/**
*********************************************************************************
* @file              : Lcd.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El- Adgham
* @brief             : Implementation file for the 16x2 LCD driver.
*********************************************************************************
*/


/* ****************** Include Section Start ******************** */
#define F_CPU 16000000UL 
#include <avr/io.h>
#include <util/delay.h>
#include "Lcd.h" 
#include "Hardware_Defs.h"
/* ****************** Include Section End   ******************** */


/* ****************** Sup-Program Section Start **************** */
#define LCD_Port    LCD_DATA_PORT
#define RS_EN_Port  LCD_RS_PORT 
#define RS          LCD_RS_PIN
#define EN          LCD_E_PIN

/**
* @brief Internal function: Sends a command byte (in 4-bit mode).
*/
static void LCD_Command_Internal( unsigned char cmnd ) {
    PORTB &= ~(1 << LCD_RS_PIN);                            // RS=0 (command)
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);           // Send upper nibble
	RS_EN_Port |= (1<<EN);   								// Pulse Enable
	_delay_us(1);
	RS_EN_Port &= ~ (1<<EN);
	_delay_us(200);
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  			// Send lower nibble
	RS_EN_Port |= (1<<EN);   								// Pulse Enable
	_delay_us(1);
	RS_EN_Port &= ~ (1<<EN);
	_delay_ms(2);
}


/**
* @brief Internal function: Sends a data/character byte (in 4-bit mode).
*/
static void LCD_Char_Internal( unsigned char data ) {
    PORTB |= (1 << LCD_RS_PIN);                                                 // RS=1 (data)
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0);           // Send upper nibble
	RS_EN_Port |= (1<<RS);   								// RS=1 (data)
	RS_EN_Port|= (1<<EN);    								// Pulse Enable
	_delay_us(1);
	RS_EN_Port &= ~ (1<<EN);
	_delay_us(200);
	LCD_Port = (LCD_Port & 0x0F) | (data << 4);  			// Send lower nibble
	RS_EN_Port |= (1<<EN);   								// Pulse Enable
	_delay_us(1);
	RS_EN_Port &= ~ (1<<EN);
	_delay_ms(2);
}


/**
* @brief Internal function: Sends a null-terminated string.
*/
static void LCD_String_Internal (char *str) {
	int i;
	for(i=0;str[i]!=0;i++) 											// Loop until null terminator
	{
		LCD_Char_Internal(str[i]);
	}
}


/**
* @brief Public function: Initializes the LCD (4-bit mode sequence).
*/
void Lcd_Init(void) {
	LCD_DATA_DDR |= 0xF0; 									// Set LCD data port upper nibble as output
	LCD_RS_DDR |= (1 << LCD_RS_PIN);
    LCD_E_DDR  |= (1 << LCD_E_PIN);
    LCD_BL_DDR |= (1 << LCD_BL_PIN);

    LCD_BL_PORT |= (1 << LCD_BL_PIN);                       // Backlight ON
    
	_delay_ms(50); 											// Wait for LCD to power on
    
    // Reset sequence
    PORTB &= ~(1 << LCD_RS_PIN);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
    _delay_ms(5);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
    _delay_us(150);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
	
	LCD_Command_Internal(0x02); 								// Return home (part of 4-bit init)
	LCD_Command_Internal(0x28);
                                                                // 4-bit mode, 2 lines, 5x8 font
	LCD_Command_Internal(0x0c); 								// Display ON, Cursor OFF
	LCD_Command_Internal(0x06); 								// Entry mode: increment cursor, no shift
	LCD_Command_Internal(0x01);
                                                                // Clear Display
    Lcd_Load_Custom_Chars();
	_delay_ms(2);
}


/**
* @brief Public function: Sends a command to the LCD.
*/
void Lcd_Send_Command(uint8_t cmd) {
    LCD_Command_Internal(cmd);
}


/**
* @brief Public function: Sends data (a character) to the LCD.
*/
void Lcd_Send_Data(uint8_t data) {
    LCD_Char_Internal(data);
}


/**
* @brief Public function: Sends a string to the LCD.
*/
void Lcd_Send_String(const char* str) {
    LCD_String_Internal((char*)str); 								// Cast const away for the internal function
}


/**
* @brief Public function: Clears the LCD display.
*/
void Lcd_Clear(void) {
    LCD_Command_Internal(0x01); 										// Send "Clear Display" command
    _delay_ms(2);
                                                                // This command takes longer to execute
}


/**
* @brief Public function: Sets the cursor to a specific row and column.
*/
void Lcd_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t address;
    if (row == 0)
	  address = (col & 0x0F) | 0x80;
                                                                // DDRAM address for row 0
    else // row == 1
	  address = (col & 0x0F) |
                                                                0xC0; 					// DDRAM address for row 1
    
    LCD_Command_Internal(address);
                                                                // Send the "Set DDRAM Address" command
}


/**
* @brief Public function: Loads custom characters into CGRAM.
*/
void Lcd_Load_Custom_Chars(void) {
    LCD_Command_Internal(0x40); // Address 0
    
    // Fan Blade +
    uint8_t Fan1[] = {0x04, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x04, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Fan1[i]);

    // Fan Blade X
    uint8_t Fan2[] = {0x11, 0x0A, 0x04, 0x0E, 0x04, 0x0A, 0x11, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Fan2[i]);
    
    // Degree Symbol
    uint8_t Deg[]  = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Deg[i]);

    LCD_Command_Internal(0x80); // Return home
}
/* ****************** Sup-Program Section End ****************** */