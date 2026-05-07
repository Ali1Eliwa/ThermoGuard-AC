/**
*********************************************************************************
* @file              : Lcd.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Header file for the 16x2 LCD driver (4-bit mode).
*********************************************************************************
*/

#ifndef LCD_H
#define LCD_H

/* ****************** Include Section Start ******************** */
#include <stdint.h>
/* ****************** Include Section End   ******************** */


/* ****************** Sup-Program Decleration Section Start **** */
/**
* @brief Initializes the LCD in 4-bit mode. Must be called first.
*/
void Lcd_Init(void);


/**
* @brief Sends a command byte to the LCD (e.g., clear, set cursor).
* @param cmd: The command byte to send.
*/
void Lcd_Send_Command(uint8_t cmd);


/**
* @brief Sends a data byte (a character) to be displayed on the LCD.
* @param data: The character to display.
*/
void Lcd_Send_Data(uint8_t data);


/**
* @brief Moves the LCD cursor to a specific position.
* @param row: The row to move to (0 or 1).
* @param col: The column to move to (0-15).
*/
void Lcd_Set_Cursor(uint8_t row, uint8_t col);


/**
* @brief Sends a null-terminated string to be displayed on the LCD.
* @param str: Pointer to the character string.
*/
void Lcd_Send_String(const char* str);


/**
* @brief Clears the entire display and returns the cursor to (0,0).
*/
void Lcd_Clear(void);


/**
* @brief Loads custom characters (Fan, Degree symbol) into CGRAM.
*/
void Lcd_Load_Custom_Chars(void);
/* ****************** Sup-Program Decleration Section End ****** */

#endif // LCD_H