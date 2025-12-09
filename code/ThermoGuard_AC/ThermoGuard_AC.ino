/**
*********************************************************************************
* @file              : ThermoGuard_AC.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Main application logic for the Automotive AC Control Panel.
*********************************************************************************
*/

/* ****************** Include Section Start ******************** */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h> 
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include "Hardware_Defs.h" 
#include "Adc.h"           
#include "Lcd.h"           
#include "Keypad.h"        
#include "Motor.h"         
/* ****************** Include Section End   ******************** */


/* ****************** Global Variables Section Start *********** */
int Target_Temp = 24;           // Target temperature setpoint
int Current_Temp = 0;           // Current reading from sensor
int Turbo_Mode = 0;             // Turbo mode status flag
int Current_Speed_Pwm = 0;      // Current PWM speed

int Idle_Counter = 0;           // Counter for inactivity
int Eco_Mode_Active = 0;        // Eco mode status flag
int Fan_Frame = 0;              // Frame index for fan animation
int Animation_Tick = 0;         // Tick counter for animation timing
char Display_Buffer[17];        // Global buffer for string operations

// --- Servo Logic Vars ---
int Swing_Mode = 0;             // 0 = Off, 1 = On
int Servo_Pulse = RIGHT_LIMIT;
int Servo_State = 0;            // 0: R to L,  1: Wait L,  2: L to R,  3: Wait R
int Servo_Wait_Counter = 0;
/* ****************** Global Variables Section End ************* */


/* ****************** Sub-Program Decleration Section Start **** */
void System_Init(void);
/* ****************** Sub-Program Decleration Section End ****** */


/* ****************** Main Program Section Start *************** */
int main(void) {
    
    System_Init();
    
    int Last_Key = KEY_NONE;
    int Key_Press = KEY_NONE;

    while (1) {
        
        // ** A. Read Inputs **
        Key_Press = Get_Keypad_Press();
        Current_Temp = Get_Temperature();

        // ** B. Process Feedback & Eco Mode **
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {
            // Dual Buzzer Pulse
            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);
            _delay_ms(10);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
            // Wake up logic
            Idle_Counter = 0;
            if (Eco_Mode_Active) {
                Eco_Mode_Active = 0;
                LCD_BL_PORT |= (1 << LCD_BL_PIN);       // ON
                Lcd_Clear();
            }
        } 
        else if (!Eco_Mode_Active && Key_Press == KEY_NONE) {
            Idle_Counter++;
            if (Idle_Counter >= IDLE_TIMEOUT) {
                Eco_Mode_Active = 1;
                LCD_BL_PORT &= ~(1 << LCD_BL_PIN);      // OFF
                Lcd_Clear();
                Lcd_Set_Cursor(0, 4);
                Lcd_Send_String("ECO MODE");
            }
        }

        // ** C. Process Configuration Logic **
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {
            
            if (Key_Press == KEY_RIGHT) { // Turbo
                Turbo_Mode = !Turbo_Mode;
                Target_Temp = Turbo_Mode ? 16 : 24;
            }
            if (Key_Press == KEY_UP) { 
                Target_Temp++;
                Turbo_Mode = 0; 
            }
            if (Key_Press == KEY_DOWN) { 
                Target_Temp--;
                Turbo_Mode = 0; 
            }
            // --- LEFT KEY controls Swing Mode ---
            if (Key_Press == KEY_LEFT) {
                Swing_Mode = !Swing_Mode;
            }
            
            if (Target_Temp < 16) Target_Temp = 16;
            if (Target_Temp > 30) Target_Temp = 30;
        }
        Last_Key = Key_Press;

        // ** D. Process Emergency Logic **
        if (Current_Temp > 50) {
            Eco_Mode_Active = 0;
            Idle_Counter = 0;
            LCD_BL_PORT |= (1 << LCD_BL_PIN);
            LED_PORT |= (1 << LED_PIN);
            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);
            
            Motor_Set_Control(255, MOTOR_FORWARD);
            
            Lcd_Set_Cursor(0, 0);
            Lcd_Send_String("!! OVERHEAT !! ");
            Lcd_Set_Cursor(1, 0);
            sprintf(Display_Buffer, "Temp:%2d%cC SHUTDWN", Current_Temp, 2);
            Lcd_Send_String(Display_Buffer);
            
            // Wait 5 seconds
             for (int i = 0; i < 50; i++) {
                _delay_ms(100);
            }

            // Shutdown Sequence
            Motor_Set_Control(0, MOTOR_OFF);
            LED_PORT &= ~(1 << LED_PIN);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
            LCD_BL_PORT &= ~(1 << LCD_BL_PIN);
            Lcd_Clear();
            
            while(1) { /* System Halted */ }
        } else {
            LED_PORT &= ~(1 << LED_PIN);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
        }

        // ** E. Process Fan Logic **
        int Diff = Current_Temp - Target_Temp;
        int Speed_Pwm = 0;
        int Req_Direction = MOTOR_OFF;
        char Mode_Str[5];

        if (Turbo_Mode) {
            Speed_Pwm = 255;
            Req_Direction = MOTOR_FORWARD;
            sprintf(Mode_Str, "TURB");
        } else {
            if (Diff >= 1) { // Cooling
                Req_Direction = MOTOR_FORWARD;
                sprintf(Mode_Str, "COOL");
                if (Diff >= 5) Speed_Pwm = 255;
                else if (Diff >= 3) Speed_Pwm = 170;
                else Speed_Pwm = 100;
            }
            else if (Diff <= -1) { // Heating
                Req_Direction = MOTOR_REVERSE;
                sprintf(Mode_Str, "HEAT");
                int Abs_Diff = abs(Diff);
                if (Abs_Diff >= 5) Speed_Pwm = 255;
                else if (Abs_Diff >= 3) Speed_Pwm = 170;
                else Speed_Pwm = 100;
            }
            else {
                Req_Direction = MOTOR_OFF;
                sprintf(Mode_Str, "IDLE");
                Speed_Pwm = 0;
            }
        }
        
        Current_Speed_Pwm = Speed_Pwm;
        Motor_Set_Control(Speed_Pwm, Req_Direction);

        // ** F. Update Display **
        if (!Eco_Mode_Active) {
            int Percent = (Current_Speed_Pwm * 100) / 255;
            Lcd_Set_Cursor(0, 0);
            if (Turbo_Mode) {
                 Lcd_Send_String(" !! TURBO MODE !!");
            } else {
                 sprintf(Display_Buffer, "Set:%2d%cC  [%s]", Target_Temp, 2, Mode_Str);
                 Lcd_Send_String(Display_Buffer);
            }

            Lcd_Set_Cursor(1, 0);
            sprintf(Display_Buffer, "Rm :%2d%cC  F:%3d%%", Current_Temp, 2, Percent);
            Lcd_Send_String(Display_Buffer);

            // Animation
            Lcd_Set_Cursor(1, 15);
            if (Req_Direction != MOTOR_OFF) {
                Animation_Tick++;
                int Update_Needed = 0;
                
                if (Current_Speed_Pwm > 200) Update_Needed = 1;
                else if (Current_Speed_Pwm > 150) {
                     if (Animation_Tick % 2 == 0) Update_Needed = 1;
                } else {
                     if (Animation_Tick % 4 == 0) Update_Needed = 1;
                }

                if (Update_Needed) Fan_Frame = !Fan_Frame;
                Lcd_Send_Data(Fan_Frame);
            } else {
                Lcd_Send_Data(' ');
            }
        }

        // ** G. SERVO & LOOP TIMING (NEW) **
        // Replaces simple _delay_ms(200) to allow Servo updates
        // Loop runs 10 times x 20ms = 200ms total delay
        for (int k = 0; k < 10; k++) {
            
            if (Swing_Mode) {
                // Execute Swing Logic (State Machine)
                // 0: Move R to L
                if (Servo_State == 0) {
                    Servo_Pulse -= 10;
                    if (Servo_Pulse <= LEFT_LIMIT) {
                        Servo_Pulse = LEFT_LIMIT;
                        Servo_State = 1; // Wait Left
                        Servo_Wait_Counter = 0;
                    }
                }
                // 1: Wait at Left
                else if (Servo_State == 1) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) { // 25 * 20ms = 500ms
                        Servo_State = 2;
                    }
                }
                // 2: Move L to R
                else if (Servo_State == 2) {
                    Servo_Pulse += 10;
                    if (Servo_Pulse >= RIGHT_LIMIT) {
                        Servo_Pulse = RIGHT_LIMIT;
                        Servo_State = 3; // Wait Right
                        Servo_Wait_Counter = 0;
                    }
                }
                // 3: Wait at Right
                else if (Servo_State == 3) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) { // 25 * 20ms = 500ms
                        Servo_State = 0;
                    }
                }
                
                Servo_Send_Pulse(Servo_Pulse); // Takes ~20ms
            } 
            else {
                // If swing is off, we still need to wait to maintain loop timing
                _delay_ms(20);
            }
        }
    }
    return 0;
}
/* ****************** Main Program Section End ***************** */


/* ****************** Sup-Program Section Start **************** */
void System_Init(void) {
    LED_DDR |= (1 << LED_PIN);
    BUZZER1_DDR |= (1 << BUZZER1_PIN);
    BUZZER2_DDR |= (1 << BUZZER2_PIN);
    
    Adc_Init();
    Motor_Init();
    Lcd_Init();
    Servo_Init(); // Initialize Servo
    
    // Splash Screen
    Lcd_Set_Cursor(0, 0);
    Lcd_Send_String(" SYSTEM LOADING ");
    Lcd_Set_Cursor(1, 0);
    for(int i=0; i<16; i++) {
        Lcd_Send_Data(1);
        _delay_ms(50);
    }
    Lcd_Clear();
}
/* ****************** Sup-Program Section End ****************** */