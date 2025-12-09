/**
*********************************************************************************
* @file              : Motor.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Header file for the Motor and Servo driver.
*********************************************************************************
*/

#ifndef MOTOR_H
#define MOTOR_H

/* ****************** Sup-Program Decleration Section Start **** */
/**
* @brief Initializes PWM on Timer 2 for motor speed control.
*/
void Motor_Init(void);

/**
* @brief Sets the fan speed and direction.
* @param Speed_Pwm: PWM duty cycle (0-255).
* @param Direction: MOTOR_FORWARD, MOTOR_REVERSE, or MOTOR_OFF.
*/
void Motor_Set_Control(int Speed_Pwm, int Direction);

/**
* @brief Initializes Servo pin (A1).
*/
void Servo_Init(void);

/**
* @brief Sends a manual PWM pulse to the servo.
* @param high_time_us: Microseconds to hold the pin HIGH.
*/
void Servo_Send_Pulse(int high_time_us);

/* ****************** Sup-Program Decleration Section End ****** */
#endif // MOTOR_H