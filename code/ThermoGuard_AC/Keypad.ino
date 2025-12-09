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
    // Ensure AVcc Reference for Keypad (Since Temp sensor switches it to 1.1V)
    ADMUX = (1 << REFS0) | KEYPAD_ADC_CHANNEL;
    _delay_ms(5);

    uint16_t Adc_Val = Adc_Read(KEYPAD_ADC_CHANNEL);
    
    if (Adc_Val > 1000)           return KEY_NONE;
    if (Adc_Val < KEY_ADC_RIGHT)  return KEY_RIGHT; 
    if (Adc_Val < KEY_ADC_UP)     return KEY_UP;
    if (Adc_Val < KEY_ADC_DOWN)   return KEY_DOWN;
    if (Adc_Val < KEY_ADC_LEFT)   return KEY_LEFT;
    if (Adc_Val < KEY_ADC_SELECT) return KEY_SELECT;
    
    return KEY_NONE;
}
/* ****************** Sup-Program Section End ****************** */