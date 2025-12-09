/**
*********************************************************************************
* @file              : Keypad.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El- Adgham
* @brief             : Implementation file for the 5-button analog keypad.
*********************************************************************************
*/

/* ****************** Include Section Start ******************** */
#include "Keypad.h"        
#include "Hardware_Defs.h"          // For ADC channel and thresholds
#include "Adc.h"                    // For Adc_Read()
#include <util/delay.h>
#include <avr/io.h>
/* ****************** Include Section End   ******************** */


/* ****************** Sup-Program Section Start **************** */
/**
* @brief Reads the analog keypad channel and returns the pressed key.
*/
int Get_Keypad_Press(void) {
    // --- 1. Setup ADC Reference ---
    // Ensure AVcc Reference is selected (Since Temp sensor might switch it to 1.1V)
    ADMUX = (1 << REFS0) | KEYPAD_ADC_CHANNEL;
    _delay_ms(5); // Allow reference voltage to stabilize

    // --- 2. Read Value ---
    uint16_t Adc_Val = Adc_Read(KEYPAD_ADC_CHANNEL);
    
    // --- 3. Determine Key (Ladder Logic) ---
    if (Adc_Val > 1000)           return KEY_NONE;    // No key pressed (Vcc)
    if (Adc_Val < KEY_ADC_RIGHT)  return KEY_RIGHT;   // 0V
    if (Adc_Val < KEY_ADC_UP)     return KEY_UP;      // ~140mV
    if (Adc_Val < KEY_ADC_DOWN)   return KEY_DOWN;    // ~330mV
    if (Adc_Val < KEY_ADC_LEFT)   return KEY_LEFT;    // ~500mV
    if (Adc_Val < KEY_ADC_SELECT) return KEY_SELECT;  // ~720mV
    
    return KEY_NONE;
}
/* ****************** Sup-Program Section End ****************** */
