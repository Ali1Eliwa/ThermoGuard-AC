/**
*********************************************************************************
* @file              : Hardware_Defs.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Central file for all hardware pin definitions and
* :                    wiring constants.
* @version           : 2.0 — Added AI Serial Link definitions
*********************************************************************************
*/

#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

/* ****************** Macro Section Start ********************** */

// --- LCD Connections (16x2 Display) ---
#define LCD_RS_PORT PORTB
#define LCD_RS_DDR  DDRB
#define LCD_RS_PIN  0         // Register Select → D8

#define LCD_E_PORT  PORTB
#define LCD_E_DDR   DDRB
#define LCD_E_PIN   1         // Enable Pin → D9

#define LCD_BL_PORT PORTB
#define LCD_BL_DDR  DDRB
#define LCD_BL_PIN  2         // Backlight Control → D10

#define LCD_DATA_PORT PORTD   // 4-bit Data bus (Upper Nibble D4–D7)
#define LCD_DATA_DDR  DDRD

// --- DC Motor Connections (L298N) ---
#define MOTOR_PWM_PORT PORTD
#define MOTOR_PWM_DDR  DDRD
#define MOTOR_PWM_PIN  3      // OC2B for PWM → D3

#define MOTOR_DIR_PORT PORTB
#define MOTOR_DIR_DDR  DDRB
#define MOTOR_IN3_PIN  4      // Direction Pin 1 → D12
#define MOTOR_IN4_PIN  5      // Direction Pin 2 → D13

// --- Emergency & Feedback Indicators ---
#define LED_PORT    PORTD
#define LED_DDR     DDRD
#define LED_PIN     2         // Red Alert LED → D2

#define BUZZER1_PORT PORTB
#define BUZZER1_DDR  DDRB
#define BUZZER1_PIN  3        // Piezo Buzzer 1 → D11

#define BUZZER2_PORT PORTC
#define BUZZER2_DDR  DDRC
#define BUZZER2_PIN  3        // Piezo Buzzer 2 → A3

// --- Servo Connections (SG90) ---
#define SERVO_PORT PORTC
#define SERVO_DDR  DDRC
#define SERVO_PIN  1          // PC1 → A1

// --- ADC Channels ---
#define KEYPAD_ADC_CHANNEL 0  // Analog Keypad on A0
#define TEMP_ADC_CHANNEL   2  // LM35 Sensor on A2

// --- Keypad ADC Thresholds ---
#define KEY_ADC_RIGHT  60
#define KEY_ADC_UP     200
#define KEY_ADC_DOWN   400
#define KEY_ADC_LEFT   600
#define KEY_ADC_SELECT 800

// --- Motor States ---
#define MOTOR_FORWARD   1
#define MOTOR_REVERSE   0
#define MOTOR_OFF       2

// --- System Constants ---
#define IDLE_TIMEOUT    50    // Loops before ECO mode triggers

// --- Servo Constants ---
#define SERVO_PERIOD    20000 // 20ms (50Hz) standard servo frame
#define RIGHT_LIMIT     1500  // ~1.5ms pulse (Center/Right limit)
#define LEFT_LIMIT      1000  // ~1.0ms pulse (Left limit)

// ---------------------------------------------------------------
// --- AI SERIAL LINK CONSTANTS (NEW v2.0) ---
// ---------------------------------------------------------------

// Baud rate for UART communication with laptop Python script
#define AI_BAUD_RATE    9600

// PWM boost applied per detected person (15% of 255 = ~38)
#define AI_PWM_PER_PERSON  38

// Maximum number of persons the system will respond to
// (prevents runaway PWM if detection goes wrong)
#define AI_MAX_PERSONS  6

// Servo pulse positions for zone targeting
// Python sends 'L', 'C', or 'R' to point fan at crowd zone
#define SERVO_ZONE_LEFT    1000   // Full left (LEFT_LIMIT)
#define SERVO_ZONE_CENTER  1250   // Center position
#define SERVO_ZONE_RIGHT   1500   // Full right (RIGHT_LIMIT)

// Serial timeout: if no AI data received for this many loops,
// reset person count to 0 (camera disconnected / script crashed)
#define AI_TIMEOUT_LOOPS   30

/* ****************** Macro Section End ************************ */

#endif // HARDWARE_DEFS_H
