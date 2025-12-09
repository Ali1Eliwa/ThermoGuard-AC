/**
*********************************************************************************
* @file              : Hardware_Defs.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Central file for all hardware pin definitions.
*********************************************************************************
*/

#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

/* ****************** Macro Section Start ********************** */
// --- LCD Connections ---
#define LCD_RS_PORT PORTB 
#define LCD_RS_DDR  DDRB
#define LCD_RS_PIN  0     

#define LCD_E_PORT  PORTB 
#define LCD_E_DDR   DDRB
#define LCD_E_PIN   1     

#define LCD_BL_PORT PORTB 
#define LCD_BL_DDR  DDRB
#define LCD_BL_PIN  2     

#define LCD_DATA_PORT PORTD 
#define LCD_DATA_DDR  DDRD

// --- Motor Connections ---
#define MOTOR_PWM_PORT PORTD
#define MOTOR_PWM_DDR  DDRD
#define MOTOR_PWM_PIN  3      

#define MOTOR_DIR_PORT PORTB
#define MOTOR_DIR_DDR  DDRB
#define MOTOR_IN3_PIN  4      
#define MOTOR_IN4_PIN  5      

// --- Emergency & Feedback ---
#define LED_PORT    PORTD
#define LED_DDR     DDRD
#define LED_PIN     2         

#define BUZZER1_PORT PORTB
#define BUZZER1_DDR  DDRB
#define BUZZER1_PIN  3        

#define BUZZER2_PORT PORTC
#define BUZZER2_DDR  DDRC
#define BUZZER2_PIN  3        

// --- Servo Connections ---
#define SERVO_PORT PORTC
#define SERVO_DDR  DDRC
#define SERVO_PIN  1          // PC1 (A1)

// --- ADC Channels ---
#define KEYPAD_ADC_CHANNEL 0      
#define TEMP_ADC_CHANNEL   2      

// --- Keypad Thresholds ---
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
#define IDLE_TIMEOUT    50 

// --- Servo Constants ---
#define SERVO_PERIOD    20000 // 20ms
#define RIGHT_LIMIT     1500  // Center (90 deg)
#define LEFT_LIMIT      1000  // Left (0 deg)

/* ****************** Macro Section End ************************ */

#endif // HARDWARE_DEFS_H