/**
*********************************************************************************
* @file              : Motor.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Implementation file for the Motor and Servo driver.
*********************************************************************************
*/

/* ****************** Include Section Start ******************** */
#include <avr/io.h>
#include <util/delay.h>
#include "Motor.h" 
#include "Hardware_Defs.h"
/* ****************** Include Section End   ******************** */

/* ****************** DC Motor Section Start ******************* */
void Motor_Init(void) {
    MOTOR_PWM_DDR |= (1 << MOTOR_PWM_PIN);
    MOTOR_DIR_DDR |= (1 << MOTOR_IN3_PIN) | (1 << MOTOR_IN4_PIN);
    // Fast PWM Mode, Prescaler 256
    TCCR2A = (1 << COM2B1) | (1 << WGM20) | (1 << WGM21);
    TCCR2B = (1 << CS22);
    OCR2B = 0; 
}

void Motor_Set_Control(int Speed_Pwm, int Direction) {
    OCR2B = Speed_Pwm;
    if (Speed_Pwm == 0 || Direction == MOTOR_OFF) {
        MOTOR_DIR_PORT &= ~((1 << MOTOR_IN3_PIN) | (1 << MOTOR_IN4_PIN));
        OCR2B = 0;
    } else if (Direction == MOTOR_FORWARD) {
        MOTOR_DIR_PORT |= (1 << MOTOR_IN3_PIN);
        MOTOR_DIR_PORT &= ~(1 << MOTOR_IN4_PIN);
    } else { // REVERSE
        MOTOR_DIR_PORT &= ~(1 << MOTOR_IN3_PIN);
        MOTOR_DIR_PORT |= (1 << MOTOR_IN4_PIN);
    }
}
/* ****************** DC Motor Section End ********************* */


/* ****************** Servo Section Start ********************** */
void Servo_Init(void) {
    SERVO_DDR |= (1 << SERVO_PIN);
    // Initialize at center
    SERVO_PORT |= (1 << SERVO_PIN);
    _delay_us(RIGHT_LIMIT);
    SERVO_PORT &= ~(1 << SERVO_PIN);
    _delay_us(SERVO_PERIOD - RIGHT_LIMIT);
}

void Servo_Send_Pulse(int high_time_us) {
    // Set Pin High
    SERVO_PORT |= (1 << SERVO_PIN);
    // Hold High for Pulse Width
    for(int i=0; i<high_time_us; i+=10) {
        _delay_us(10);
    }

    // Set Pin Low
    SERVO_PORT &= ~(1 << SERVO_PIN);
    // Wait for the rest of the 20ms period
    int low_time = SERVO_PERIOD - high_time_us;
    for(int i=0; i<low_time; i+=10) {
        _delay_us(10);
    }
}
/* ****************** Servo Section End ************************ */