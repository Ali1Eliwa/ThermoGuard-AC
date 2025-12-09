/**
*********************************************************************************
* @file              : Hardware_Defs.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Central file for all hardware pin definitions and 
* : wiring constants.
*********************************************************************************
*/

#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

/* ****************** Macro Section Start ********************** */

// --- LCD Connections (16x2 Display) ---
#define LCD_RS_PORT PORTB 
#define LCD_RS_DDR  DDRB
#define LCD_RS_PIN  0         // Register Select

#define LCD_E_PORT  PORTB 
#define LCD_E_DDR   DDRB
#define LCD_E_PIN   1         // Enable Pin

#define LCD_BL_PORT PORTB 
#define LCD_BL_DDR  DDRB
#define LCD_BL_PIN  2         // Backlight Control

#define LCD_DATA_PORT PORTD   // 4-bit Data bus (Using Upper Nibble D4-D7)
#define LCD_DATA_DDR  DDRD

// --- DC Motor Connections (L298N) ---
#define MOTOR_PWM_PORT PORTD
#define MOTOR_PWM_DDR  DDRD
#define MOTOR_PWM_PIN  3      // OC2B for PWM

#define MOTOR_DIR_PORT PORTB
#define MOTOR_DIR_DDR  DDRB
#define MOTOR_IN3_PIN  4      // Direction Pin 1
#define MOTOR_IN4_PIN  5      // Direction Pin 2

// --- Emergency & Feedback Indicators ---
#define LED_PORT    PORTD
#define LED_DDR     DDRD
#define LED_PIN     2         // Red Alert LED

#define BUZZER1_PORT PORTB
#define BUZZER1_DDR  DDRB
#define BUZZER1_PIN  3        // Piezo Buzzer 1

#define BUZZER2_PORT PORTC
#define BUZZER2_DDR  DDRC
#define BUZZER2_PIN  3        // Piezo Buzzer 2

// --- Servo Connections (SG90) ---
#define SERVO_PORT PORTC
#define SERVO_DDR  DDRC
#define SERVO_PIN  1          // PC1 (A1)

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

/* ****************** Macro Section End ************************ */

#endif // HARDWARE_DEFS_H
