/**
*********************************************************************************
* @file              : Adc.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El- Adgham
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
    ADMUX = (1<<REFS0);
                                    // Use AVcc as reference voltage
                                    // Enable ADC, Set Prescaler to 128 (16MHz/128 = 125kHz)
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}


/**
* @brief Internal function to select channel and perform one conversion.
*/
static unsigned short Adc_ReadChannel_Internal(unsigned char ch) {

  ch &= 0b00000111;
                                // Ensure channel is 0-7
  ADMUX = (ADMUX & 0xF8)|ch;    // Select the channel
  ADCSRA |= (1<<ADSC);
                                // Start a single conversion
  while(ADCSRA & (1<<ADSC));    // Wait for the conversion to complete
  return (ADC);
                                // Return the 10-bit result
}


/**
* @brief Public function: Initializes the ADC module.
*/
void Adc_Init(void) {
    Adc_Init_Internal();
                                // Call the private implementation
}


/**
* @brief Public function: Reads a 10-bit value from the specified channel.
*/
uint16_t Adc_Read(uint8_t Channel) {
    ADMUX = (ADMUX & 0xF0) | (Channel & 0x0F);          // Select channel, keep Ref
    return Adc_ReadChannel_Internal(Channel);           // Call the private implementation
}


/**
* @brief Public function: Reads temperature.
*/
int Get_Temperature(void) {
    // Set Internal 1.1V Reference
    ADMUX = (1 << REFS0) | (1 << REFS1) | TEMP_ADC_CHANNEL;
    _delay_ms(5);               // Stabilize

    uint32_t Sum = 0;
    for (int i = 0; i < 20; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        Sum += ADC;
        _delay_ms(1);
    }
    uint16_t Avg = Sum / 20;
                                // Conversion: Temp = (ADC * 110) / 1024
    return (int)((Avg * 110UL) / 1024UL);
}
/* ****************** Sup-Program Section End ****************** */