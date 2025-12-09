/**
*********************************************************************************
* @file              : Keypad.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Header file for the 5-button analog keypad driver.
*********************************************************************************
*/

#ifndef KEYPAD_H
#define KEYPAD_H

/* ****************** Macro Section Start ********************** */
// Key press definitions (Returned by Get_Keypad_Press)
enum {
    KEY_NONE,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_SELECT
};
/* ****************** Macro Section End ************************ */


/* ****************** Sup-Program Decleration Section Start **** */
/**
* @brief Reads the analog voltage from the keypad's ADC channel
* : and maps it to a specific key.
* @return The corresponding key value (e.g., KEY_UP) or KEY_NONE.
*/
int Get_Keypad_Press(void);
/* ****************** Sup-Program Decleration Section End ****** */

#endif // KEYPAD_H
