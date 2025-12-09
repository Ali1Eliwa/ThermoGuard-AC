/**
*********************************************************************************
* @file              : ThermoGuard_AC.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Main application logic for the Automotive AC Control Panel.
* : Handles user inputs, temperature regulation, servo swing
* : mechanism, and system safety monitoring.
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
int Target_Temp = 24;           // Default target temperature setpoint
int Current_Temp = 0;           // Current reading from LM35 sensor
int Turbo_Mode = 0;             // Flag: 0 = Normal Operation, 1 = Max Cooling
int Current_Speed_Pwm = 0;      // Current PWM speed value sent to motor

int Idle_Counter = 0;           // Timer to track user inactivity
int Eco_Mode_Active = 0;        // Flag: 0 = Active, 1 = Power Saving Mode
int Fan_Frame = 0;              // Animation index for the LCD fan icon
int Animation_Tick = 0;         // Time base for animation speed control
char Display_Buffer[17];        // General purpose buffer for LCD strings

// --- Servo Swing Logic Variables ---
int Swing_Mode = 0;             // Flag: 0 = Fixed position, 1 = Swinging
int Servo_Pulse = RIGHT_LIMIT;  // Current pulse width in microseconds
int Servo_State = 0;            // State Machine Tracker:
                                // 0: Moving Right to Left
                                // 1: Holding at Left Limit
                                // 2: Moving Left to Right
                                // 3: Holding at Right Limit
int Servo_Wait_Counter = 0;     // Timer for holding position at limits
/* ****************** Global Variables Section End ************* */


/* ****************** Sub-Program Decleration Section Start **** */
void System_Init(void);
/* ****************** Sub-Program Decleration Section End ****** */


/* ****************** Main Program Section Start *************** */
/**
* @brief Main entry point and super-loop of the AC Controller.
* Coordinates input reading, thermal logic, safety checks,
* and actuator control (Motor/Servo).
*/
int main(void) {
    
    // --- 1. Initialization ---
    System_Init();              // Setup all peripherals (ADC, LCD, PWM, etc.)
    
    int Last_Key = KEY_NONE;    // Previous key state for edge detection
    int Key_Press = KEY_NONE;   // Current key state

    // --- 2. Main Loop ---
    while (1) {
        
        // --- A. Read Inputs ---
        Key_Press = Get_Keypad_Press();     // Poll the analog keypad 
        Current_Temp = Get_Temperature();   // Read LM35 sensor via ADC 

        // --- B. Process Feedback & Wake-up Logic ---
        // Check for new button press (Rising Edge detection)
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {
            
            // Trigger feedback sound (Dual Buzzer Pulse)
            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);
            _delay_ms(10);                  // Short beep duration
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
            
            // Reset Idle Timer on any interaction
            Idle_Counter = 0;
            
            // If we were in ECO mode, wake up now
            if (Eco_Mode_Active) {
                Eco_Mode_Active = 0;
                LCD_BL_PORT |= (1 << LCD_BL_PIN);   // Turn Backlight ON
                Lcd_Clear();                        // Refresh display
            }
        } 
        // Logic for entering ECO mode logic if no buttons pressed
        else if (!Eco_Mode_Active && Key_Press == KEY_NONE) {
            Idle_Counter++;
            
            // Check if timeout threshold reached
            if (Idle_Counter >= IDLE_TIMEOUT) {
                Eco_Mode_Active = 1;                // Enable Power Saving
                LCD_BL_PORT &= ~(1 << LCD_BL_PIN);  // Turn Backlight OFF
                Lcd_Clear();
                Lcd_Set_Cursor(0, 4);
                Lcd_Send_String("ECO MODE");        // Show status
            }
        }

        // --- C. Process Configuration Logic ---
        // Handle button commands (only on new press)
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {
            
            if (Key_Press == KEY_RIGHT) { 
                Turbo_Mode = !Turbo_Mode;           // Toggle Turbo
                Target_Temp = Turbo_Mode ? 16 : 24; // Override Temp if Turbo
            }
            if (Key_Press == KEY_UP) { 
                Target_Temp++;                      // Increment Setpoint
                Turbo_Mode = 0;                     // Manual change disables Turbo
            }
            if (Key_Press == KEY_DOWN) { 
                Target_Temp--;                      // Decrement Setpoint
                Turbo_Mode = 0;                     // Manual change disables Turbo
            }
            if (Key_Press == KEY_LEFT) {
                Swing_Mode = !Swing_Mode;           // Toggle Air Swing
            }
            
            // Clamp Target Temperature between 16C and 30C
            if (Target_Temp < 16) Target_Temp = 16;
            if (Target_Temp > 30) Target_Temp = 30;
        }
        Last_Key = Key_Press;   // Save state for next loop iteration

        // --- D. Process Emergency Logic ---
        // Critical safety check for overheating
        if (Current_Temp > 50) {
            // Force system awake
            Eco_Mode_Active = 0;
            Idle_Counter = 0;
            LCD_BL_PORT |= (1 << LCD_BL_PIN);
            
            // Activate all visual and audio alarms
            LED_PORT |= (1 << LED_PIN);
            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);
            
            // Maximize cooling fan to dissipate heat
            Motor_Set_Control(255, MOTOR_FORWARD);
            
            // Display Emergency Message
            Lcd_Set_Cursor(0, 0);
            Lcd_Send_String("!! OVERHEAT !! ");
            Lcd_Set_Cursor(1, 0);
            sprintf(Display_Buffer, "Temp:%2d%cC SHUTDWN", Current_Temp, 2);
            Lcd_Send_String(Display_Buffer);
            
            // Hold alarm state for 5 seconds
             for (int i = 0; i < 50; i++) {
                _delay_ms(100);
            }

            // --- SYSTEM LOCKDOWN ---
            Motor_Set_Control(0, MOTOR_OFF);        // Kill Motor
            LED_PORT &= ~(1 << LED_PIN);            // Kill LED
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);    // Kill Buzzers
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
            LCD_BL_PORT &= ~(1 << LCD_BL_PIN);      // Kill Display
            Lcd_Clear();
            
            while(1) { /* Infinite Loop: Requires Hard Reset */ }
        } else {
            // Normal state: Ensure alarms are off
            LED_PORT &= ~(1 << LED_PIN);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
        }

        // --- E. Process Fan Logic ---
        // Calculate error between Current and Target Temp
        int Diff = Current_Temp - Target_Temp;
        int Speed_Pwm = 0;
        int Req_Direction = MOTOR_OFF;
        char Mode_Str[5];
        
        if (Turbo_Mode) {
            Speed_Pwm = 255;                        // Max Speed
            Req_Direction = MOTOR_FORWARD;
            sprintf(Mode_Str, "TURB");
        } else {
            // Determine cooling or heating needs
            if (Diff >= 1) { // Current > Target -> Cooling needed
                Req_Direction = MOTOR_FORWARD;
                sprintf(Mode_Str, "COOL");
                
                // Proportional-ish control logic
                if (Diff >= 5) Speed_Pwm = 255;     // High Speed
                else if (Diff >= 3) Speed_Pwm = 170;// Medium Speed
                else Speed_Pwm = 100;               // Low Speed
            }
            else if (Diff <= -1) { // Current < Target -> Heating needed
                Req_Direction = MOTOR_REVERSE;
                sprintf(Mode_Str, "HEAT");
                
                int Abs_Diff = abs(Diff);
                if (Abs_Diff >= 5) Speed_Pwm = 255;
                else if (Abs_Diff >= 3) Speed_Pwm = 170;
                else Speed_Pwm = 100;
            }
            else { // Temperature reached
                Req_Direction = MOTOR_OFF;
                sprintf(Mode_Str, "IDLE");
                Speed_Pwm = 0;
            }
        }
        
        Current_Speed_Pwm = Speed_Pwm;
        Motor_Set_Control(Speed_Pwm, Req_Direction); // Update H-Bridge

        // --- F. Update Display ---
        if (!Eco_Mode_Active) {
            int Percent = (Current_Speed_Pwm * 100) / 255; // Calc duty %
            
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

            // --- Fan Animation Logic ---
            Lcd_Set_Cursor(1, 15);
            if (Req_Direction != MOTOR_OFF) {
                Animation_Tick++;
                int Update_Needed = 0;
                
                // Animate faster if fan speed is higher
                if (Current_Speed_Pwm > 200) Update_Needed = 1;         // Every Loop
                else if (Current_Speed_Pwm > 150) {
                     if (Animation_Tick % 2 == 0) Update_Needed = 1;    // Every 2nd Loop
                } else {
                     if (Animation_Tick % 4 == 0) Update_Needed = 1;    // Every 4th Loop
                }

                if (Update_Needed) Fan_Frame = !Fan_Frame; // Toggle Frame
                Lcd_Send_Data(Fan_Frame);
            } else {
                Lcd_Send_Data(' '); // Clear animation if idle
            }
        }

        // --- G. SERVO & LOOP TIMING ---
        // We use a sliced loop here to generate Servo PWM signals (Software PWM)
        // Total Delay = 10 slices * 20ms = 200ms
        for (int k = 0; k < 10; k++) {
            
            if (Swing_Mode) {
                // --- Servo State Machine ---
                
                // Case 0: Move Blade Right to Left
                if (Servo_State == 0) {
                    Servo_Pulse -= 10;              // Decrement Pulse Width
                    if (Servo_Pulse <= LEFT_LIMIT) {
                        Servo_Pulse = LEFT_LIMIT;   // Clamp
                        Servo_State = 1;            // Transition to Wait
                        Servo_Wait_Counter = 0;
                    }
                }
                // Case 1: Hold at Left Limit
                else if (Servo_State == 1) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) { // Wait approx 500ms
                        Servo_State = 2;            // Transition to Return
                    }
                }
                // Case 2: Move Blade Left to Right
                else if (Servo_State == 2) {
                    Servo_Pulse += 10;              // Increment Pulse Width
                    if (Servo_Pulse >= RIGHT_LIMIT) {
                        Servo_Pulse = RIGHT_LIMIT;  // Clamp
                        Servo_State = 3;            // Transition to Wait
                        Servo_Wait_Counter = 0;
                    }
                }
                // Case 3: Hold at Right Limit
                else if (Servo_State == 3) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) { // Wait approx 500ms
                        Servo_State = 0;            // Restart Cycle
                    }
                }
                
                Servo_Send_Pulse(Servo_Pulse);      // Send signal
            } 
            else {
                // If swing OFF, simply wait to keep loop timing consistent
                _delay_ms(20);
            }
        }
    }
    return 0; // Unreachable
}
/* ****************** Main Program Section End ***************** */


/* ****************** Sub-Program Section Start **************** */
/**
* @brief Internal helper to initialize all system modules.
*/
void System_Init(void) {
    // 1. Configure I/O Directions
    LED_DDR |= (1 << LED_PIN);
    BUZZER1_DDR |= (1 << BUZZER1_PIN);
    BUZZER2_DDR |= (1 << BUZZER2_PIN);
    
    // 2. Initialize Drivers
    Adc_Init();     // ADC for Sensors/Keypad
    Motor_Init();   // PWM for DC Motor
    Lcd_Init();     // 16x2 Display
    Servo_Init();   // PWM for Servo
    
    // 3. Show Boot Screen
    Lcd_Set_Cursor(0, 0);
    Lcd_Send_String(" SYSTEM LOADING ");
    Lcd_Set_Cursor(1, 0);
    
    // Simple progress bar animation
    for(int i=0; i<16; i++) {
        Lcd_Send_Data(1);   // Custom Block Char
        _delay_ms(50);
    }
    Lcd_Clear();
}
/* ****************** Sub-Program Section End ****************** */
