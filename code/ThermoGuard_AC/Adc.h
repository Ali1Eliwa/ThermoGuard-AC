/**
*********************************************************************************
* @file              : Adc.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El- Adgham
* @brief             : Header file for the ADC (Analog-to-Digital Converter)
* : driver.
*********************************************************************************
*/
 
#ifndef ADC_H
#define ADC_H


/* ****************** Include Section Start ******************** */
#include <stdint.h>         // For uint8_t and uint16_t
/* ****************** Include Section End   ******************** */


/* ****************** Sup-Program Decleration Section Start **** */
/**
* @brief Initializes the ADC hardware.
*/
void Adc_Init(void);


/**
* @brief Reads a 10-bit value from the specified ADC channel.
* @param Channel: The ADC channel to read (0-7).
* @return The 10-bit digital result (0-1023).
*/
uint16_t Adc_Read(uint8_t Channel);


/**
* @brief Reads the temperature from LM35 and converts to Celsius.
* @return Temperature in degrees Celsius.
*/
int Get_Temperature(void);

/* ****************** Sup-Program Decleration Section End ****** */
#endif // ADC_H