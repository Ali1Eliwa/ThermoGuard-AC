"""
=================================================================================
ThermoGuard Voice Control — Laptop-side Python Script
=================================================================================
Author  : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
Version : 1.1 — Added temperature set-point voice commands (18-40)
Mic     : BOYA BY-M1 Lavalier (3.5mm jack into laptop mic port)

COMMANDS:
  "turbo mode on"     → sends 'T'       → Arduino turns Turbo ON
  "turbo mode off"    → sends 't'       → Arduino turns Turbo OFF
  "swing on"          → sends 'S'       → Arduino enables swing
  "swing off"         → sends 's'       → Arduino disables swing
  "temperature 18"    → sends 'W' + 18  → Arduino sets Target_Temp = 18
  "temperature 19"    → sends 'W' + 19  → Arduino sets Target_Temp = 19
  ... (any number from 18 to 40)
  "temperature 40"    → sends 'W' + 40  → Arduino sets Target_Temp = 40

HOW TO RUN:
  python ThermoGuard_Voice.py

REQUIREMENTS:
  pip install SpeechRecognition pyaudio opencv-python pyserial
=================================================================================
"""

import cv2
import serial
import speech_recognition as sr
import threading
import time
import sys
import numpy as np
from collections import deque

# -----------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------
SERIAL_PORT    = 'COM7'
BAUD_RATE      = 9600
MIC_INDEX      = None        # None = system default mic
ENERGY_THRESH  = 300         # Lower = more sensitive
PAUSE_THRESH   = 0.6
TEMP_MIN       = 18
TEMP_MAX       = 40
# -----------------------------------------------------------------------


def build_command_map():
    """
    Builds the full command dictionary including all temperature values.
    Temperature commands accept multiple natural phrasings.
    """
    cmds = {}

    # --- Mode commands ---
    cmds["turbo mode on"]  = ("TURBO ON",  lambda a: a.write(b'T'))
    cmds["turbo on"]       = ("TURBO ON",  lambda a: a.write(b'T'))
    cmds["turbo mode off"] = ("TURBO OFF", lambda a: a.write(b't'))
    cmds["turbo off"]      = ("TURBO OFF", lambda a: a.write(b't'))
    cmds["turbo of"]      = ("TURBO OFF", lambda a: a.write(b't'))
    cmds["swing on"]       = ("SWING ON",  lambda a: a.write(b'S'))
    cmds["swing mode on"]  = ("SWING ON",  lambda a: a.write(b'S'))
    cmds["swing off"]      = ("SWING OFF", lambda a: a.write(b's'))
    cmds["swing of"]      = ("SWING OFF", lambda a: a.write(b's'))
    cmds["swing mode off"] = ("SWING OFF", lambda a: a.write(b's'))

    # --- Temperature commands (18–40) ---
    # Multiple phrasings per temperature so recognition is flexible
    for t in range(TEMP_MIN, TEMP_MAX + 1):
        label  = f"TEMP {t}C"
        # Capture t in closure correctly with default argument
        action = (lambda temp: lambda a: (
            a.write(b'W'),
            time.sleep(0.03),
            a.write(bytes([temp]))
        ))(t)

        cmds[f"temperature {t}"] = (label, action)
        cmds[f"set temperature {t}"] = (label, action)
        cmds[f"set temp {t}"]    = (label, action)
        cmds[f"temp {t}"]        = (label, action)

    return cmds

COMMANDS = build_command_map()


# -----------------------------------------------------------------------
# Shared state between listener thread and display thread
# -----------------------------------------------------------------------
class VoiceState:
    def __init__(self):
        self.status       = "Initialising..."
        self.last_heard   = ""
        self.last_command = "—"
        self.history      = deque(maxlen=6)
        self.serial_ok    = False
        self.lock         = threading.Lock()

state = VoiceState()


def find_mic_index():
    mic_list = sr.Microphone.list_microphone_names()
    print("\nAvailable microphones:")
    for i, name in enumerate(mic_list):
        print(f"  [{i}] {name}")

    if MIC_INDEX is not None:
        print(f"\nUsing manually set mic index: {MIC_INDEX}")
        return MIC_INDEX

    for i, name in enumerate(mic_list):
        if any(k in name.lower() for k in ['boya', 'by-m1', 'lavalier', 'external']):
            print(f"\nFound BOYA mic at index {i}: {name}")
            return i

    print("\nBOYA not found by name — using system default mic.")
    return None


def match_command(text):
    """
    Matches recognised text to a command.
    Returns (label, action_fn) or (None, None).
    Uses substring matching — partial phrases still work.
    """
    text_lower = text.lower().strip()

    # Try exact substring match first (most reliable)
    for phrase, (label, action) in COMMANDS.items():
        if phrase in text_lower:
            return label, action

    # Fallback: try to extract a number after "temperature" or "temp"
    import re
    match = re.search(r'\b(?:temperature|temp(?:erature)?)\s+(\d+)\b', text_lower)
    if match:
        num = int(match.group(1))
        if TEMP_MIN <= num <= TEMP_MAX:
            label = f"TEMP {num}C"
            t = num
            action = lambda a, temp=t: (
                a.write(b'W'),
                time.sleep(0.03),
                a.write(bytes([temp]))
            )
            return label, action

    return None, None


def listen_loop(arduino, mic_idx):
    recognizer = sr.Recognizer()
    recognizer.energy_threshold = ENERGY_THRESH
    recognizer.pause_threshold  = PAUSE_THRESH
    recognizer.dynamic_energy_threshold = True

    mic = sr.Microphone(device_index=mic_idx)

    with state.lock:
        state.status = "Calibrating mic (1 sec)..."
    with mic as source:
        recognizer.adjust_for_ambient_noise(source, duration=1)
    with state.lock:
        state.status = "Listening..."

    print("[OK]  Microphone ready")

    while True:
        try:
            with mic as source:
                audio = recognizer.listen(source, timeout=5, phrase_time_limit=5)

            with state.lock:
                state.status    = "Processing..."
                state.last_heard = "..."

            try:
                text = recognizer.recognize_google(audio, language='en-US')
            except sr.UnknownValueError:
                with state.lock:
                    state.status    = "Listening..."
                    state.last_heard = "(not understood)"
                continue
            except sr.RequestError as e:
                with state.lock:
                    state.status    = "Listening..."
                    state.last_heard = f"API error: {e}"
                continue

            with state.lock:
                state.last_heard = text

            print(f"[HEARD] '{text}'")

            label, action = match_command(text)

            if label and action:
                sent_ok = False
                if arduino:
                    try:
                        action(arduino)
                        sent_ok = True
                    except serial.SerialException as e:
                        print(f"[WARN] Serial write failed: {e}")
                        with state.lock:
                            state.serial_ok = False

                with state.lock:
                    state.last_command = label
                    state.history.appendleft(label)
                    state.status = "Listening..."

                print(f"[MATCH] '{label}' → {'sent' if sent_ok else 'no serial'}")
            else:
                with state.lock:
                    state.status = "Listening..."
                print(f"[NO MATCH] '{text}'")

        except sr.WaitTimeoutError:
            with state.lock:
                state.status = "Listening..."
        except Exception as e:
            print(f"[ERR] {e}")
            with state.lock:
                state.status = "Error — retrying..."
            time.sleep(1)


def draw_window(arduino_connected):
    frame = np.zeros((440, 500, 3), dtype=np.uint8)
    frame[:] = (30, 30, 30)

    with state.lock:
        s_status  = state.status
        s_heard   = state.last_heard
        s_command = state.last_command
        s_history = list(state.history)
        s_serial  = state.serial_ok

    # Header
    cv2.rectangle(frame, (0, 0), (500, 52), (45, 45, 45), -1)
    cv2.putText(frame, "ThermoGuard Voice Control v1.1",
                (12, 34), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    # Status dot + text
    is_listening = "Listening" in s_status
    sc = (0, 220, 80) if is_listening else (0, 180, 220)
    cv2.circle(frame, (24, 76), 8, sc, -1)
    cv2.putText(frame, s_status,
                (42, 82), cv2.FONT_HERSHEY_SIMPLEX, 0.6, sc, 1)

    cv2.line(frame, (12, 102), (488, 102), (60, 60, 60), 1)

    # Last heard
    cv2.putText(frame, "Last heard:",
                (12, 124), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (130, 130, 130), 1)
    heard = (s_heard[:52] if s_heard else "—")
    cv2.putText(frame, f'"{heard}"',
                (12, 147), cv2.FONT_HERSHEY_SIMPLEX, 0.52, (210, 210, 210), 1)

    cv2.line(frame, (12, 165), (488, 165), (60, 60, 60), 1)

    # Last command
    cv2.putText(frame, "Command executed:",
                (12, 188), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (130, 130, 130), 1)
    cc = (0, 230, 80) if s_command != "—" else (100, 100, 100)

    # Larger text for temperature commands, normal for mode commands
    font_scale = 0.65 if "TEMP" in s_command else 0.75
    cv2.putText(frame, s_command,
                (12, 216), cv2.FONT_HERSHEY_SIMPLEX, font_scale, cc, 2)

    cv2.line(frame, (12, 234), (488, 234), (60, 60, 60), 1)

    # History
    cv2.putText(frame, "Recent commands:",
                (12, 256), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (130, 130, 130), 1)
    for i, h in enumerate(s_history):
        alpha = max(70, 210 - i * 30)
        cv2.putText(frame, f"  {i+1}. {h}",
                    (12, 276 + i * 22),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45,
                    (alpha, alpha, alpha), 1)

    # Temp range hint
    cv2.line(frame, (12, 412), (488, 412), (60, 60, 60), 1)
    cv2.putText(frame, f'Say: "temperature 18" to "temperature 40"',
                (12, 428), cv2.FONT_HERSHEY_SIMPLEX, 0.40, (100, 160, 100), 1)

    # Bottom bar
    cv2.rectangle(frame, (0, 406), (500, 440), (20, 20, 20), -1)
    s_color = (0, 220, 80) if (s_serial or arduino_connected) else (60, 80, 220)
    s_text  = f"Serial: {SERIAL_PORT}  [CONNECTED]" if (s_serial or arduino_connected) \
              else f"Serial: {SERIAL_PORT}  [DISCONNECTED]"
    cv2.putText(frame, s_text,
                (12, 429), cv2.FONT_HERSHEY_SIMPLEX, 0.43, s_color, 1)
    cv2.putText(frame, "Press Q to quit",
                (360, 429), cv2.FONT_HERSHEY_SIMPLEX, 0.40, (100, 100, 100), 1)

    cv2.imshow("ThermoGuard Voice Control", frame)
    return (cv2.waitKey(100) & 0xFF) != ord('q')


def main():
    print("=" * 60)
    print("  ThermoGuard Voice Control v1.1")
    print("  Mic: BOYA BY-M1")
    print(f"  Temperature range: {TEMP_MIN}C - {TEMP_MAX}C")
    print("=" * 60)

    arduino = None
    arduino_connected = False
    try:
        arduino = serial.Serial(port=SERIAL_PORT, baudrate=BAUD_RATE, timeout=0.1)
        time.sleep(2)
        arduino_connected = True
        with state.lock:
            state.serial_ok = True
        print(f"[OK]  Serial connected → {SERIAL_PORT} @ {BAUD_RATE} baud")
    except serial.SerialException as e:
        print(f"[WARN] Serial: {e}")
        print("       Running without Arduino — display only.")

    mic_idx = find_mic_index()

    listener = threading.Thread(
        target=listen_loop,
        args=(arduino, mic_idx),
        daemon=True
    )
    listener.start()

    print("\n[OK]  Voice listener started")
    print("\nAvailable commands:")
    print("  → \"turbo mode on / off\"")
    print("  → \"swing on / off\"")
    print(f"  → \"temperature 18\" through \"temperature 40\"")
    print("\nPress Q in the window to quit.\n")

    while True:
        if not draw_window(arduino_connected):
            print("\nQ pressed — shutting down.")
            break

    cv2.destroyAllWindows()
    if arduino:
        arduino.close()
        print("[OK]  Serial closed.")
    print("[OK]  Done.")


if __name__ == '__main__':
    main()
