/**
*********************************************************************************
* @file              : ThermoGuard_AC.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Main application logic for the Automotive AC Control Panel.
* :                    Handles user inputs, temperature regulation, servo swing
* :                    mechanism, and system safety monitoring.
* @version           : 2.0 — ThermoGuard AI: Person Detection Integration
*                      - UART Serial Link to laptop Python/OpenCV script
*                      - Metabolic Multiplier: +15% PWM per detected person
*                      - Servo Zone Targeting: fan directed at crowded area
*                      - AI timeout failsafe: reverts to normal if link drops
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
#include "Serial_AI.h"        // NEW v2.0 — AI Serial Link driver
/* ****************** Include Section End   ******************** */


/* ****************** Global Variables Section Start *********** */
int Target_Temp        = 21;  // Default target temperature setpoint
int Current_Temp       = 0;   // Current reading from LM35 sensor
int Turbo_Mode         = 0;   // Flag: 0 = Normal, 1 = Max Cooling
int Current_Speed_Pwm  = 0;   // Current PWM speed value sent to motor

int Idle_Counter       = 0;   // Timer to track user inactivity
int Eco_Mode_Active    = 0;   // Flag: 0 = Active, 1 = Power Saving Mode
int Fan_Frame          = 0;   // Animation index for the LCD fan icon
int Animation_Tick     = 0;   // Time base for animation speed control
char Display_Buffer[17];      // General purpose buffer for LCD strings

// --- Servo Swing Logic Variables ---
int Swing_Mode         = 0;         // Flag: 0 = Fixed, 1 = Swinging
int Servo_Pulse        = RIGHT_LIMIT; // Current pulse width in microseconds
int Servo_State        = 0;         // Swing state machine (0–3)
int Servo_Wait_Counter = 0;         // Timer for holding at limits

// ---------------------------------------------------------------
// --- AI SERIAL LINK VARIABLES (NEW v2.0) ---
// ---------------------------------------------------------------
int  AI_Person_Count    = 0;
char AI_Servo_Zone      = 'C';
char AI_Servo_Zone_End  = 'C';
int  AI_Timeout_Ctr     = 0;
int  AI_Active          = 0;
uint8_t Voice_Cmd       = VOICE_CMD_NONE;  // Last voice command received
uint8_t Voice_Temp      = 0;               // Temperature value from voice (18-40)
/* ****************** Global Variables Section End ************* */


/* ****************** Sub-Program Declaration Section Start **** */
void System_Init(void);
void Handle_AI_Serial(void);
void Apply_AI_Servo_Zone(void);
void Apply_AI_Servo_Sweep(void);
/* ****************** Sub-Program Declaration Section End ****** */


/* ****************** Main Program Section Start *************** */
/**
* @brief Main entry point and super-loop of the AC Controller.
*        Coordinates input reading, thermal logic, safety checks,
*        actuator control (Motor/Servo), and AI serial processing.
*/
int main(void) {

    // --- 1. Initialization ---
    System_Init();

    int Last_Key  = KEY_NONE;
    int Key_Press = KEY_NONE;

    // --- 2. Main Loop ---
    while (1) {

        // --- A. Read Inputs ---
        Key_Press    = Get_Keypad_Press();
        Current_Temp = Get_Temperature();

        // -------------------------------------------------------
        // --- A2. Read AI Serial Link (NEW v2.0) ---
        // Non-blocking: reads one byte if available, updates
        // AI_Person_Count, AI_Servo_Zone, and AI_Timeout_Ctr.
        // -------------------------------------------------------
        Handle_AI_Serial();

        // --- B. Process Feedback & Wake-up Logic ---
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {

            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);
            _delay_ms(10);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);

            Idle_Counter = 0;

            if (Eco_Mode_Active) {
                Eco_Mode_Active = 0;
                LCD_BL_PORT |= (1 << LCD_BL_PIN);
                Lcd_Clear();
            }
        }
        else if (!Eco_Mode_Active && Key_Press == KEY_NONE) {
            Idle_Counter++;
            if (Idle_Counter >= IDLE_TIMEOUT) {
                Eco_Mode_Active = 1;
                LCD_BL_PORT &= ~(1 << LCD_BL_PIN);
                Lcd_Clear();
                Lcd_Set_Cursor(0, 4);
                Lcd_Send_String("ECO MODE");
            }
        }

        // --- C. Process Configuration Logic ---
        if (Key_Press != KEY_NONE && Last_Key == KEY_NONE) {

            if (Key_Press == KEY_RIGHT) {
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
            if (Key_Press == KEY_LEFT) {
                Swing_Mode = !Swing_Mode;
            }

            if (Target_Temp < 16) Target_Temp = 16;
            if (Target_Temp > 30) Target_Temp = 30;
        }

        // -------------------------------------------------------
        // --- C2. VOICE COMMAND HANDLING (v3.1) ---
        // Processed every loop — independent of keypad presses.
        // Has identical effect to pressing the physical buttons.
        // -------------------------------------------------------
        if (Voice_Cmd == VOICE_CMD_TURBO_ON) {
            Turbo_Mode  = 1;
            Target_Temp = 16;
        }
        else if (Voice_Cmd == VOICE_CMD_TURBO_OFF) {
            Turbo_Mode  = 0;
            Target_Temp = 24;
        }
        else if (Voice_Cmd == VOICE_CMD_SWING_ON) {
            Swing_Mode = 1;
        }
        else if (Voice_Cmd == VOICE_CMD_SWING_OFF) {
            Swing_Mode = 0;
        }
        else if (Voice_Cmd == VOICE_CMD_SET_TEMP) {
            // Set temperature directly from voice (range 18-40)
            // Turbo is cancelled — user is manually setting a target
            Turbo_Mode  = 0;
            Target_Temp = (int)Voice_Temp;
            // Clamp to system range just in case
            if (Target_Temp < 16) Target_Temp = 16;
            if (Target_Temp > 40) Target_Temp = 40;
        }
        // Clear after handling — ready for next command
        Voice_Cmd  = VOICE_CMD_NONE;
        Voice_Temp = 0;
        Last_Key = Key_Press;

        // --- D. Process Emergency Logic ---
        if (Current_Temp > 50) {
            Eco_Mode_Active = 0;
            Idle_Counter    = 0;
            LCD_BL_PORT |= (1 << LCD_BL_PIN);

            LED_PORT     |= (1 << LED_PIN);
            BUZZER1_PORT |= (1 << BUZZER1_PIN);
            BUZZER2_PORT |= (1 << BUZZER2_PIN);

            Motor_Set_Control(0, MOTOR_FORWARD);

            Lcd_Set_Cursor(0, 0);
            Lcd_Send_String("!! OVERHEAT !! ");
            Lcd_Set_Cursor(1, 0);
            sprintf(Display_Buffer, "Temp:%2d%cC SHUTDWN", Current_Temp, 2);
            Lcd_Send_String(Display_Buffer);

            for (int i = 0; i < 50; i++) {
                _delay_ms(100);
            }

            Motor_Set_Control(0, MOTOR_OFF);
            LED_PORT     &= ~(1 << LED_PIN);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
            LCD_BL_PORT  &= ~(1 << LCD_BL_PIN);
            Lcd_Clear();

            while (1) { /* SYSTEM LOCKDOWN — requires hard reset */ }

        } else {
            LED_PORT     &= ~(1 << LED_PIN);
            BUZZER1_PORT &= ~(1 << BUZZER1_PIN);
            BUZZER2_PORT &= ~(1 << BUZZER2_PIN);
        }

        // --- E. Process Fan Logic ---
        int Diff        = Current_Temp - Target_Temp;
        int Speed_Pwm   = 0;
        int Req_Direction = MOTOR_OFF;
        char Mode_Str[6];

        if (Turbo_Mode) {
            Speed_Pwm     = 255;
            Req_Direction = MOTOR_FORWARD;
            sprintf(Mode_Str, "TURB");

        } else {
            if (Diff >= 1) {
                Req_Direction = MOTOR_FORWARD;
                sprintf(Mode_Str, "COOL");

                if (Diff >= 5)      Speed_Pwm = 255;
                else if (Diff >= 3) Speed_Pwm = 170;
                else                Speed_Pwm = 100;
            }
            else if (Diff <= -1) {
                Req_Direction = MOTOR_REVERSE;
                sprintf(Mode_Str, "HEAT");

                int Abs_Diff = abs(Diff);
                if (Abs_Diff >= 5)      Speed_Pwm = 255;
                else if (Abs_Diff >= 3) Speed_Pwm = 170;
                else                    Speed_Pwm = 100;
            }
            else {
                Req_Direction = MOTOR_OFF;
                sprintf(Mode_Str, "IDLE");
                Speed_Pwm = 0;
            }
        }

        // -----------------------------------------------------------
        // --- E2. AI METABOLIC MULTIPLIER (NEW v2.0) ---
        //
        // If the laptop AI link is alive and has detected people,
        // boost the base PWM by AI_PWM_PER_PERSON (~15%) per person.
        // This only applies when the fan is already running (COOL/HEAT).
        // It does NOT activate the fan from IDLE — thermal logic governs that.
        // Turbo Mode already runs at 255 so no boost is needed there.
        // -----------------------------------------------------------
        if (AI_Active && AI_Person_Count > 0 && Speed_Pwm > 0 && !Turbo_Mode) {
            int Boost = AI_Person_Count * AI_PWM_PER_PERSON;
            Speed_Pwm = Speed_Pwm + Boost;
            if (Speed_Pwm > 255) Speed_Pwm = 255; // Hard clamp
            sprintf(Mode_Str, "AI%d", AI_Person_Count); // e.g. "AI3" on LCD
        }

        Current_Speed_Pwm = Speed_Pwm;
        Motor_Set_Control(Speed_Pwm, Req_Direction);

        // --- F. Update Display ---
        if (!Eco_Mode_Active) {
            int Percent = (Current_Speed_Pwm * 100) / 255;

            Lcd_Set_Cursor(0, 0);
            if (Turbo_Mode) {
                Lcd_Send_String(" !! TURBO MODE !!");
            } else {
                // Show AI indicator on display when link is active
                if (AI_Active && AI_Person_Count > 0) {
                    sprintf(Display_Buffer, "Set:%2d%cC [%s]  ", Target_Temp, 2, Mode_Str);
                } else {
                    sprintf(Display_Buffer, "Set:%2d%cC  [%s]", Target_Temp, 2, Mode_Str);
                }
                Lcd_Send_String(Display_Buffer);
            }

            Lcd_Set_Cursor(1, 0);
            sprintf(Display_Buffer, "Rm :%2d%cC  F:%3d%%", Current_Temp, 2, Percent);
            Lcd_Send_String(Display_Buffer);

            // Fan Animation
            Lcd_Set_Cursor(1, 15);
            if (Req_Direction != MOTOR_OFF) {
                Animation_Tick++;
                int Update_Needed = 0;

                if      (Current_Speed_Pwm > 200)                       Update_Needed = 1;
                else if (Current_Speed_Pwm > 150 && Animation_Tick % 2 == 0) Update_Needed = 1;
                else if (Animation_Tick % 4 == 0)                        Update_Needed = 1;

                if (Update_Needed) Fan_Frame = !Fan_Frame;
                Lcd_Send_Data(Fan_Frame);
            } else {
                Lcd_Send_Data(' ');
            }
        }

        // --- G. SERVO & LOOP TIMING ---
        // 10 slices × 20ms = 200ms total loop period
        //
        // SERVO PRIORITY:
        //
        //   PRIORITY 1 — Swing button ON (LEFT key)
        //     Full left-to-right sweep ALWAYS.
        //     Ignores AI completely. User has manual control.
        //
        //   PRIORITY 2 — AI Active + persons detected
        //     Sweeps ONLY across the zones where people are.
        //     If people span L and R  → sweeps full L↔R
        //     If people only in L     → holds on L
        //     If people in L and C    → sweeps L↔C only
        //     No button press needed.
        //
        //   PRIORITY 3 — AI Active, no persons
        //     Returns servo smoothly to center and holds.
        //
        //   PRIORITY 4 — Idle
        //     No AI, no swing. Hold current position, no pulse.
        //
        for (int k = 0; k < 10; k++) {

            if (Swing_Mode) {
                // -----------------------------------------------
                // PRIORITY 1 — MANUAL FULL SWEEP
                // Swing button is ON. Full L-to-R sweep always.
                // AI is completely ignored here by design.
                // -----------------------------------------------
                if (Servo_State == 0) {
                    Servo_Pulse -= 10;
                    if (Servo_Pulse <= LEFT_LIMIT) {
                        Servo_Pulse        = LEFT_LIMIT;
                        Servo_State        = 1;
                        Servo_Wait_Counter = 0;
                    }
                }
                else if (Servo_State == 1) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) Servo_State = 2;
                }
                else if (Servo_State == 2) {
                    Servo_Pulse += 10;
                    if (Servo_Pulse >= RIGHT_LIMIT) {
                        Servo_Pulse        = RIGHT_LIMIT;
                        Servo_State        = 3;
                        Servo_Wait_Counter = 0;
                    }
                }
                else if (Servo_State == 3) {
                    Servo_Wait_Counter++;
                    if (Servo_Wait_Counter >= 25) Servo_State = 0;
                }
                Servo_Send_Pulse(Servo_Pulse);
            }
            else if (AI_Active && AI_Person_Count > 0) {
                // -----------------------------------------------
                // PRIORITY 2 — AI ZONE RANGE SWEEP
                // Automatically sweeps between the leftmost and
                // rightmost zones where people are detected.
                // Apply_AI_Servo_Sweep() runs a mini state machine
                // that bounces between AI_Servo_Zone and
                // AI_Servo_Zone_End target pulse positions.
                // -----------------------------------------------
                Apply_AI_Servo_Sweep();
                Servo_Send_Pulse(Servo_Pulse);
            }
            else if (AI_Active && AI_Person_Count == 0) {
                // -----------------------------------------------
                // PRIORITY 3 — AI ACTIVE, NO PERSONS
                // Smoothly return to center and hold.
                // -----------------------------------------------
                AI_Servo_Zone     = 'C';
                AI_Servo_Zone_End = 'C';
                Apply_AI_Servo_Zone();
                Servo_Send_Pulse(Servo_Pulse);
            }
            else {
                // -----------------------------------------------
                // PRIORITY 4 — IDLE
                // -----------------------------------------------
                _delay_ms(20);
            }
        }
    }

    return 0;
}
/* ****************** Main Program Section End ***************** */


/* ****************** Sub-Program Section Start **************** */

/**
* @brief Calls the Serial_AI driver, updates AI_Active flag,
*        and captures any incoming voice command byte.
*        Called once per main loop iteration (non-blocking).
*/
void Handle_AI_Serial(void) {
    Serial_AI_Read(&AI_Person_Count,
                   &AI_Servo_Zone,
                   &AI_Servo_Zone_End,
                   &AI_Timeout_Ctr,
                   &Voice_Cmd,
                   &Voice_Temp);

    AI_Active = (AI_Timeout_Ctr < AI_TIMEOUT_LOOPS) ? 1 : 0;
}


/**
* @brief Converts a zone char ('L','C','R') to its servo pulse value.
*/
static int Zone_To_Pulse(char zone) {
    if (zone == 'L') return SERVO_ZONE_LEFT;
    if (zone == 'R') return SERVO_ZONE_RIGHT;
    return SERVO_ZONE_CENTER;
}


/**
* @brief AI zone RANGE sweep state machine.
*
*        Bounces Servo_Pulse between the pulse values of
*        AI_Servo_Zone (leftmost) and AI_Servo_Zone_End (rightmost).
*
*        If both zones are equal (e.g. 'L'+'L') → holds at that position.
*        If zones differ (e.g. 'L'+'R') → sweeps back and forth between them
*        at 10us steps per call, identical feel to the manual swing.
*
*        Uses Servo_State and Servo_Wait_Counter shared variables
*        (same ones as the manual swing — safe because the two modes
*        never run at the same time due to the priority structure).
*/
void Apply_AI_Servo_Sweep(void) {
    int Pulse_Start = Zone_To_Pulse(AI_Servo_Zone);
    int Pulse_End   = Zone_To_Pulse(AI_Servo_Zone_End);

    // Same zone — just hold position smoothly (no sweep needed)
    if (Pulse_Start == Pulse_End) {
        // Step toward the target
        if (Servo_Pulse < Pulse_Start) {
            Servo_Pulse += 10;
            if (Servo_Pulse > Pulse_Start) Servo_Pulse = Pulse_Start;
        } else if (Servo_Pulse > Pulse_Start) {
            Servo_Pulse -= 10;
            if (Servo_Pulse < Pulse_Start) Servo_Pulse = Pulse_Start;
        }
        return;
    }

    // Different zones — bounce sweep between Pulse_Start and Pulse_End
    // State 0: moving toward Pulse_Start (leftmost)
    // State 1: holding at Pulse_Start
    // State 2: moving toward Pulse_End (rightmost)
    // State 3: holding at Pulse_End
    if (Servo_State == 0) {
        Servo_Pulse -= 10;
        if (Servo_Pulse <= Pulse_Start) {
            Servo_Pulse        = Pulse_Start;
            Servo_State        = 1;
            Servo_Wait_Counter = 0;
        }
    }
    else if (Servo_State == 1) {
        Servo_Wait_Counter++;
        if (Servo_Wait_Counter >= 15) Servo_State = 2; // Shorter hold for AI mode
    }
    else if (Servo_State == 2) {
        Servo_Pulse += 10;
        if (Servo_Pulse >= Pulse_End) {
            Servo_Pulse        = Pulse_End;
            Servo_State        = 3;
            Servo_Wait_Counter = 0;
        }
    }
    else if (Servo_State == 3) {
        Servo_Wait_Counter++;
        if (Servo_Wait_Counter >= 15) Servo_State = 0;
    }
}


/**
* @brief Snaps the servo toward a single zone (used for center return).
*/
void Apply_AI_Servo_Zone(void) {
    int Target_Pulse = Zone_To_Pulse(AI_Servo_Zone);

    if (Servo_Pulse < Target_Pulse) {
        Servo_Pulse += 10;
        if (Servo_Pulse > Target_Pulse) Servo_Pulse = Target_Pulse;
    }
    else if (Servo_Pulse > Target_Pulse) {
        Servo_Pulse -= 10;
        if (Servo_Pulse < Target_Pulse) Servo_Pulse = Target_Pulse;
    }
}


/**
* @brief Initializes all system peripherals.
*/
void System_Init(void) {
    // 1. Configure I/O Directions
    LED_DDR     |= (1 << LED_PIN);
    BUZZER1_DDR |= (1 << BUZZER1_PIN);
    BUZZER2_DDR |= (1 << BUZZER2_PIN);

    // 2. Initialize Drivers
    Adc_Init();          // ADC for Sensors/Keypad
    Motor_Init();        // PWM for DC Motor
    Lcd_Init();          // 16x2 Display
    Servo_Init();        // PWM for Servo
    Serial_AI_Init();    // UART for AI Serial Link (NEW v2.0)

    // 3. Show Boot Screen
    Lcd_Set_Cursor(0, 0);
    Lcd_Send_String(" THERMOGUARD AI ");
    Lcd_Set_Cursor(1, 0);
    for (int i = 0; i < 16; i++) {
        Lcd_Send_Data(1);  // Custom Block Char progress bar
        _delay_ms(50);
    }
    Lcd_Clear();
}

/* ****************** Sub-Program Section End ****************** */
