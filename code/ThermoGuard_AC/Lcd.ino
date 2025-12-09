/**
*********************************************************************************
* @file              : Lcd.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Implementation file for the 16x2 LCD driver.
* : Implements 4-bit mode communication protocol.
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
* Splits byte into two nibbles.
*/
static void LCD_Command_Internal( unsigned char cmnd ) {
    PORTB &= ~(1 << LCD_RS_PIN);                            // RS=0 for Command
    
    // Send Upper Nibble
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);
	RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~ (1<<EN); // Pulse Enable
	_delay_us(200);
	
    // Send Lower Nibble
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);
	RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~ (1<<EN); // Pulse Enable
	_delay_ms(2);
}


/**
* @brief Internal function: Sends a data/character byte (in 4-bit mode).
*/
static void LCD_Char_Internal( unsigned char data ) {
    PORTB |= (1 << LCD_RS_PIN);                             // RS=1 for Data
    
    // Send Upper Nibble
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0);
	RS_EN_Port |= (1<<RS);                                  // Keep RS high
	RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~ (1<<EN); // Pulse Enable
	_delay_us(200);
	
    // Send Lower Nibble
	LCD_Port = (LCD_Port & 0x0F) | (data << 4);
	RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~ (1<<EN); // Pulse Enable
	_delay_ms(2);
}


/**
* @brief Internal function: Sends a null-terminated string.
*/
static void LCD_String_Internal (char *str) {
	int i;
	for(i=0;str[i]!=0;i++) { 								// Loop until null terminator
		LCD_Char_Internal(str[i]);
	}
}


/**
* @brief Public function: Initializes the LCD (4-bit mode sequence).
*/
void Lcd_Init(void) {
    // --- 1. Set Directions ---
	LCD_DATA_DDR |= 0xF0; 									// Data port upper nibble as output
	LCD_RS_DDR |= (1 << LCD_RS_PIN);
    LCD_E_DDR  |= (1 << LCD_E_PIN);
    LCD_BL_DDR |= (1 << LCD_BL_PIN);

    LCD_BL_PORT |= (1 << LCD_BL_PIN);                       // Turn Backlight ON
	_delay_ms(50); 											// Wait for power-up
    
    // --- 2. Reset Sequence (Datasheet Requirement) ---
    PORTB &= ~(1 << LCD_RS_PIN);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
    _delay_ms(5);
    
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
    _delay_us(150);
    
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | 0x30; 
    RS_EN_Port |= (1<<EN); _delay_us(1); RS_EN_Port &= ~(1<<EN);
	
    // --- 3. Set 4-Bit Mode ---
	LCD_Command_Internal(0x02);                             // Initialize 4-bit mode
	
    // --- 4. Configuration ---
	LCD_Command_Internal(0x28);                             // 2 lines, 5x8 matrix
	LCD_Command_Internal(0x0c); 								// Display ON, Cursor OFF
	LCD_Command_Internal(0x06);                             // Increment cursor right
	LCD_Command_Internal(0x01);                             // Clear Display
    
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
    LCD_String_Internal((char*)str);
}


/**
* @brief Public function: Clears the LCD display.
*/
void Lcd_Clear(void) {
    LCD_Command_Internal(0x01); 		// Send "Clear Display" command
    _delay_ms(2);                       // Requires > 1.5ms delay
}


/**
* @brief Public function: Sets the cursor to a specific row and column.
*/
void Lcd_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t address;
    if (row == 0)
	  address = (col & 0x0F) | 0x80;    // Row 0 starts at 0x80
    else 
	  address = (col & 0x0F) | 0xC0; 	// Row 1 starts at 0xC0
    
    LCD_Command_Internal(address);
}


/**
* @brief Public function: Loads custom characters into CGRAM.
*/
void Lcd_Load_Custom_Chars(void) {
    LCD_Command_Internal(0x40); // Set CGRAM Address to 0
    
    // Fan Blade +
    uint8_t Fan1[] = {0x04, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x04, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Fan1[i]);

    // Fan Blade X
    uint8_t Fan2[] = {0x11, 0x0A, 0x04, 0x0E, 0x04, 0x0A, 0x11, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Fan2[i]);
    
    // Degree Symbol
    uint8_t Deg[]  = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};
    for(int i=0; i<8; i++) LCD_Char_Internal(Deg[i]);

    LCD_Command_Internal(0x80); // Return cursor to home
}
/* ****************** Sup-Program Section End ****************** */