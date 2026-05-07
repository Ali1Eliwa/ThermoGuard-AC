/**
*********************************************************************************
* @file              : Adc.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Implementation file for the ADC driver.
*********************************************************************************
*/

/* ****************** Include Section Start ******************** */
#include <avr/io.h>
#include <util/delay.h>
#include "Adc.h" 
#include "Hardware_Defs.h"
/* ****************** Include Section End   ******************** */


/* ****************** Sup-Program Section Start **************** */
/**
* @brief Internal function to configure ADC registers.
*/
static void Adc_Init_Internal(void) {
    ADMUX = (1<<REFS0); // Use AVcc as reference voltage
    
    // Enable ADC, Set Prescaler to 128 (16MHz/128 = 125kHz)
    // 125kHz is within the optimal 50-200kHz range for AVR ADC
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}


/**
* @brief Internal function to select channel and perform one conversion.
*/
static unsigned short Adc_ReadChannel_Internal(unsigned char ch) {

  ch &= 0b00000111;             // Ensure channel is restricted to 0-7
  ADMUX = (ADMUX & 0xF8)|ch;    // Clear bottom 3 bits, Set channel
  
  ADCSRA |= (1<<ADSC);          // Start a single conversion
  
  while(ADCSRA & (1<<ADSC));    // Wait for the conversion to complete
  
  return (ADC);                 // Return the 10-bit result
}


/**
* @brief Public function: Initializes the ADC module.
*/
void Adc_Init(void) {
    Adc_Init_Internal();        // Call the private implementation
}


/**
* @brief Public function: Reads a 10-bit value from the specified channel.
*/
uint16_t Adc_Read(uint8_t Channel) {
    // Note: We only set the channel here, Reference is assumed AVcc
    // unless changed by Get_Temperature.
    ADMUX = (ADMUX & 0xF0) | (Channel & 0x0F); 
    return Adc_ReadChannel_Internal(Channel);
}


/**
* @brief Public function: Reads temperature with high precision settings.
*/
int Get_Temperature(void) {
    // Set Internal 1.1V Reference for better precision on small signals (LM35)
    ADMUX = (1 << REFS0) | (1 << REFS1) | TEMP_ADC_CHANNEL;
    _delay_ms(5);               // Stabilize reference voltage

    // Take 20 samples and average them to reduce noise
    uint32_t Sum = 0;
    for (int i = 0; i < 20; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        Sum += ADC;
        _delay_ms(1);
    }
    uint16_t Avg = Sum / 20;
    
    // Conversion Formula: 
    // Vout = (ADC / 1024) * 1.1V
    // Temp = Vout * 100 (since 10mV/C)
    // Temp = (ADC * 110) / 1024
    return (int)((Avg * 110UL) / 1024UL);
}
/* ****************** Sup-Program Section End ****************** */
