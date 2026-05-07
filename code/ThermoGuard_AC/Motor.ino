/**
*********************************************************************************
* @file              : Motor.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Implementation file for the Motor and Servo driver.
* : Handles Timer2 PWM for DC motor and Bit-Banging for Servo.
*********************************************************************************
*/

/* ****************** Include Section Start ******************** */
#include <avr/io.h>
#include <util/delay.h>
#include "Motor.h" 
#include "Hardware_Defs.h"
/* ****************** Include Section End   ******************** */

/* ****************** DC Motor Section Start ******************* */
/**
* @brief Initializes Timer2 in Fast PWM mode for motor speed control.
*/
void Motor_Init(void) {
    // Configure pins as Outputs
    MOTOR_PWM_DDR |= (1 << MOTOR_PWM_PIN);
    MOTOR_DIR_DDR |= (1 << MOTOR_IN3_PIN) | (1 << MOTOR_IN4_PIN);

    // Timer2 Configuration:
    // WGM20 | WGM21 = Fast PWM Mode
    // COM2B1        = Clear OC2B on Compare Match (Non-Inverting Mode)
    TCCR2A = (1 << COM2B1) | (1 << WGM20) | (1 << WGM21);
    
    // CS22          = Prescaler 64 (Frequency adjustment)
    TCCR2B = (1 << CS22);
    
    OCR2B = 0;  // Start with 0% Duty Cycle (Motor Stopped)
}

/**
* @brief Sets the motor direction and speed using H-Bridge and PWM.
* @param Speed_Pwm: 0-255 duty cycle.
* @param Direction: MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_OFF.
*/
void Motor_Set_Control(int Speed_Pwm, int Direction) {
    OCR2B = Speed_Pwm;  // Set PWM duty cycle
    
    // Handle Stop Condition
    if (Speed_Pwm == 0 || Direction == MOTOR_OFF) {
        MOTOR_DIR_PORT &= ~((1 << MOTOR_IN3_PIN) | (1 << MOTOR_IN4_PIN)); // IN3=0, IN4=0
        OCR2B = 0;
    } 
    // Handle Forward
    else if (Direction == MOTOR_FORWARD) {
        MOTOR_DIR_PORT |= (1 << MOTOR_IN3_PIN);     // IN3 = 1
        MOTOR_DIR_PORT &= ~(1 << MOTOR_IN4_PIN);    // IN4 = 0
    } 
    // Handle Reverse
    else { 
        MOTOR_DIR_PORT &= ~(1 << MOTOR_IN3_PIN);    // IN3 = 0
        MOTOR_DIR_PORT |= (1 << MOTOR_IN4_PIN);     // IN4 = 1
    }
}
/* ****************** DC Motor Section End ********************* */


/* ****************** Servo Section Start ********************** */
/**
* @brief Initializes the servo pin to the center position.
*/
void Servo_Init(void) {
    SERVO_DDR |= (1 << SERVO_PIN);  // Set Pin Direction
    
    // Send initial center pulse to position servo
    SERVO_PORT |= (1 << SERVO_PIN);
    _delay_us(RIGHT_LIMIT);         // Wait High Time
    SERVO_PORT &= ~(1 << SERVO_PIN);
    _delay_us(SERVO_PERIOD - RIGHT_LIMIT); // Wait Low Time
}

/**
* @brief Generates a specific PWM pulse manually (Bit-Banging).
* @param high_time_us: The duration of the HIGH signal in microseconds.
*/
void Servo_Send_Pulse(int high_time_us) {
    // 1. Set Pin High (Start Pulse)
    SERVO_PORT |= (1 << SERVO_PIN);
    
    // 2. Hold High for specific duration
    for(int i=0; i<high_time_us; i+=10) {
        _delay_us(10);  // Small delay increments for resolution
    }

    // 3. Set Pin Low (End Pulse)
    SERVO_PORT &= ~(1 << SERVO_PIN);
    
    // 4. Wait for the rest of the 20ms period (Period - HighTime)
    int low_time = SERVO_PERIOD - high_time_us;
    for(int i=0; i<low_time; i+=10) {
        _delay_us(10);
    }
}
/* ****************** Servo Section End ************************ */