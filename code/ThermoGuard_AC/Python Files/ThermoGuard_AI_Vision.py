"""
=================================================================================
ThermoGuard AI Vision — Laptop-side Python Script
=================================================================================
Author  : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
Version : 2.1 — Improved Detection Accuracy
Purpose : Captures webcam frames, detects people and sends serial commands
          to the Arduino for fan speed boost and servo zone targeting.

WHAT CHANGED IN v2.1:
  - Added face detection (Haar Cascade) as a secondary detector
    because HOG+SVM needs full standing bodies but your camera
    sees seated people close-up — faces are far more reliable here
  - Added a voting buffer (last 5 frames) to smooth out flickering
    counts — no more jumping between 0 and 1 every frame
  - Added Non-Maximum Suppression (NMS) to eliminate duplicate boxes
    when HOG detects the same person twice
  - Resizes frame before detection for better speed + accuracy balance
  - HOG confidence threshold added to ignore weak detections
=================================================================================
"""

import cv2
import serial
import time
import sys
import numpy as np
from collections import deque

# -----------------------------------------------------------------------
# CONFIGURATION 
# -----------------------------------------------------------------------
SERIAL_PORT       = 'COM7'      # Your Arduino COM port
BAUD_RATE         = 9600        # Must match AI_BAUD_RATE in Hardware_Defs.h
CAMERA_INDEX      = 0           # 0 = built-in webcam, 1 = external USB
SEND_INTERVAL_SEC = 0.5         # How often to send data to Arduino (seconds)
MAX_PERSONS       = 6           # Clamp to match AI_MAX_PERSONS on Arduino

# --- Detection mode ---
# 'face'  -> uses face detector only (best for close-up seated people)
# 'body'  -> uses HOG body detector only (best for full standing bodies)
# 'both'  -> uses both and takes the higher count (most robust, default)
DETECTION_MODE    = 'both'

# --- HOG tuning (body detector) ---
HOG_WIN_STRIDE    = (8, 8)
HOG_PADDING       = (8, 8)
HOG_SCALE         = 1.03
HOG_THRESHOLD     = 0.3

# --- Face detector tuning ---
FACE_SCALE_FACTOR = 1.1
FACE_MIN_NEIGHBORS= 4
FACE_MIN_SIZE     = (60, 60)

# --- Stability buffer ---
BUFFER_SIZE       = 5
# -----------------------------------------------------------------------


def apply_nms(boxes, weights, overlap_threshold=0.65):
    if len(boxes) == 0:
        return []
    boxes_arr = np.array(boxes)
    scores    = np.array(weights).flatten().tolist()
    indices   = cv2.dnn.NMSBoxes(
        boxes_arr.tolist(), scores,
        score_threshold=HOG_THRESHOLD,
        nms_threshold=overlap_threshold
    )
    if len(indices) == 0:
        return []
    return [boxes[i] for i in indices.flatten()]


def detect_persons(frame, hog, face_cascade):
    target_w    = 640
    h, w        = frame.shape[:2]
    scale_ratio = target_w / w
    small       = cv2.resize(frame, (target_w, int(h * scale_ratio)))

    detected_boxes = []
    face_boxes     = []

    # HOG body detection
    if DETECTION_MODE in ('body', 'both'):
        hog_boxes, hog_weights = hog.detectMultiScale(
            small, winStride=HOG_WIN_STRIDE,
            padding=HOG_PADDING, scale=HOG_SCALE
        )
        if len(hog_boxes) > 0:
            strong = [(b, wt) for b, wt in zip(hog_boxes, hog_weights.flatten())
                      if wt > HOG_THRESHOLD]
            if strong:
                fb = [s[0] for s in strong]
                fw = [s[1] for s in strong]
                for (x, y, bw, bh) in apply_nms(fb, fw):
                    detected_boxes.append((
                        int(x / scale_ratio), int(y / scale_ratio),
                        int(bw / scale_ratio), int(bh / scale_ratio)
                    ))

    # Haar face detection
    if DETECTION_MODE in ('face', 'both'):
        gray = cv2.cvtColor(small, cv2.COLOR_BGR2GRAY)
        gray = cv2.equalizeHist(gray)
        faces = face_cascade.detectMultiScale(
            gray, scaleFactor=FACE_SCALE_FACTOR,
            minNeighbors=FACE_MIN_NEIGHBORS, minSize=FACE_MIN_SIZE
        )
        if len(faces) > 0:
            for (x, y, fw, fh) in faces:
                face_boxes.append((
                    int(x / scale_ratio), int(y / scale_ratio),
                    int(fw / scale_ratio), int(fh / scale_ratio)
                ))

    if DETECTION_MODE == 'face':
        return face_boxes, face_boxes
    elif DETECTION_MODE == 'body':
        return detected_boxes, detected_boxes
    else:
        if len(face_boxes) >= len(detected_boxes):
            return face_boxes, face_boxes
        else:
            return detected_boxes, detected_boxes


def get_servo_zone(boxes, frame_width):
    """
    Returns a zone RANGE string describing where ALL people are,
    not just the dominant zone. This lets the Arduino sweep between
    the leftmost and rightmost occupied zone.

    Examples:
      1 person on left only          -> 'L'
      1 person on right only         -> 'R'
      2 persons, one L one R         -> 'LR'   (Arduino sweeps L to R)
      2 persons, one L one C         -> 'LC'   (Arduino sweeps L to C)
      all persons in center          -> 'C'
      persons in all three zones     -> 'LR'   (full sweep)

    The Arduino reads:
      First char  = leftmost  zone to reach
      Second char = rightmost zone to reach (same as first if single zone)
    """
    if len(boxes) == 0:
        return 'CC'   # No persons -> hold center (2-char for consistency)

    zone_counts = {'L': 0, 'C': 0, 'R': 0}
    third = frame_width / 3

    for (x, y, w, h) in boxes:
        xc = x + w / 2
        if xc < third:
            zone_counts['L'] += 1
        elif xc < 2 * third:
            zone_counts['C'] += 1
        else:
            zone_counts['R'] += 1

    # Find leftmost and rightmost occupied zones
    occupied = [z for z in ('L', 'C', 'R') if zone_counts[z] > 0]

    if not occupied:
        return 'CC'

    leftmost  = occupied[0]   # First in L->C->R order
    rightmost = occupied[-1]  # Last  in L->C->R order

    if leftmost == rightmost:
        # All people in one zone — point to it and hold
        return leftmost + leftmost    # e.g. 'LL', 'CC', 'RR'
    else:
        # People span multiple zones — sweep between them
        return leftmost + rightmost   # e.g. 'LR', 'LC', 'CR'


def draw_ui(frame, boxes, raw_count, smoothed_count, zone, port, ai_active, mode):
    h, w  = frame.shape[:2]
    third = w // 3

    cv2.line(frame, (third,   0), (third,   h), (200,200,200), 1)
    cv2.line(frame, (2*third, 0), (2*third, h), (200,200,200), 1)
    cv2.putText(frame, 'L', (third//2 - 8, 28),             cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200,200,200), 1)
    cv2.putText(frame, 'C', (third + third//2 - 8, 28),     cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200,200,200), 1)
    cv2.putText(frame, 'R', (2*third + third//2 - 8, 28),   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200,200,200), 1)

    box_color = (0, 230, 80) if smoothed_count > 0 else (100,100,100)
    for (x, y, bw, bh) in boxes:
        cv2.rectangle(frame, (x, y), (x+bw, y+bh), box_color, 2)
        cv2.putText(frame, 'Person', (x, max(y-6, 10)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45, box_color, 1)

    cv2.rectangle(frame, (0, h-95), (w, h), (20,20,20), -1)
    s_color = (0,220,80) if ai_active else (60,80,220)
    s_text  = f"Serial: {port}  [CONNECTED]" if ai_active else f"Serial: {port}  [DISCONNECTED]"

    cv2.putText(frame, f"Persons detected: {smoothed_count}  (raw: {raw_count})",
                (10, h-70), cv2.FONT_HERSHEY_SIMPLEX, 0.60, (255,255,255), 2)
    cv2.putText(frame, f"Servo zone: {zone}    Mode: {mode.upper()}",
                (10, h-45), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (255,180,50), 1)
    cv2.putText(frame, s_text,
                (10, h-18), cv2.FONT_HERSHEY_SIMPLEX, 0.45, s_color, 1)
    cv2.putText(frame, "Press Q to quit",
                (w-155, h-18), cv2.FONT_HERSHEY_SIMPLEX, 0.42, (130,130,130), 1)
    return frame


def main():
    print("=" * 60)
    print("  ThermoGuard AI Vision v2.1")
    print("=" * 60)

    arduino   = None
    ai_active = False
    try:
        arduino = serial.Serial(port=SERIAL_PORT, baudrate=BAUD_RATE, timeout=0.1)
        time.sleep(2)
        ai_active = True
        print(f"[OK]  Serial connected -> {SERIAL_PORT} @ {BAUD_RATE} baud")
    except serial.SerialException as e:
        print(f"[WARN] Serial port error: {e}")
        print("       Running in camera-only mode.")

    hog = cv2.HOGDescriptor()
    hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())
    print("[OK]  HOG+SVM body detector loaded")

    face_cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
    face_cascade      = cv2.CascadeClassifier(face_cascade_path)
    if face_cascade.empty():
        print("[WARN] Face cascade not found — using body-only mode")
        global DETECTION_MODE
        DETECTION_MODE = 'body'
    else:
        print("[OK]  Face (Haar) detector loaded")

    print(f"[OK]  Detection mode: {DETECTION_MODE.upper()}")

    cap = cv2.VideoCapture(CAMERA_INDEX)
    if not cap.isOpened():
        print(f"[ERR] Cannot open camera {CAMERA_INDEX}. Exiting.")
        if arduino: arduino.close()
        sys.exit(1)
    print(f"[OK]  Camera {CAMERA_INDEX} opened")
    print(f"\nRunning in '{DETECTION_MODE}' mode. Press Q in the window to quit.\n")

    count_buffer   = deque(maxlen=BUFFER_SIZE)
    last_send_time = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[ERR] Camera read failed.")
            break

        frame_h, frame_w = frame.shape[:2]

        all_boxes, draw_boxes = detect_persons(frame, hog, face_cascade)

        raw_count      = min(len(all_boxes), MAX_PERSONS)
        count_buffer.append(raw_count)
        smoothed_count = int(round(sum(count_buffer) / len(count_buffer)))
        smoothed_count = min(smoothed_count, MAX_PERSONS)

        # zone_range: 2-char string — leftmost+rightmost occupied zone
        # 'LL'=only left  'CC'=only center  'RR'=only right
        # 'LR'=sweep full 'LC'=sweep L-to-C 'CR'=sweep C-to-R
        zone_range = get_servo_zone(all_boxes, frame_w)

        # Display string: 'L', 'C', 'R', or 'L↔R', 'L↔C', 'C↔R'
        if zone_range[0] == zone_range[1]:
            zone_display = zone_range[0]
        else:
            zone_display = zone_range[0] + '<>' + zone_range[1]

        now = time.time()
        if arduino and (now - last_send_time) >= SEND_INTERVAL_SEC:
            last_send_time = now
            try:
                if smoothed_count > 0:
                    # 3-byte protocol:
                    #   Byte 1 = person count  ('1'-'6')
                    #   Byte 2 = leftmost zone ('L','C','R')
                    #   Byte 3 = rightmost zone('L','C','R')
                    # If both bytes are same -> hold position
                    # If different           -> Arduino sweeps between them
                    arduino.write(bytes(str(smoothed_count), 'utf-8'))
                    time.sleep(0.02)
                    arduino.write(bytes(zone_range[0], 'utf-8'))
                    time.sleep(0.02)
                    arduino.write(bytes(zone_range[1], 'utf-8'))
                else:
                    arduino.write(b'0')
                    time.sleep(0.02)
                    arduino.write(b'H')
            except serial.SerialException as e:
                print(f"[WARN] Serial write failed: {e}")
                ai_active = False

        frame = draw_ui(frame, draw_boxes, raw_count, smoothed_count,
                        zone_display, SERIAL_PORT, ai_active, DETECTION_MODE)
        cv2.imshow('ThermoGuard AI Vision', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("\nQ pressed — shutting down.")
            break

    cap.release()
    cv2.destroyAllWindows()
    if arduino:
        try:
            arduino.write(b'0')
            arduino.write(b'H')
        except:
            pass
        arduino.close()
        print("[OK]  Serial port closed.")
    print("[OK]  Done.")


if __name__ == '__main__':
    main()
