/**
*********************************************************************************
* @file              : Serial_AI.h
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : Header for the AI Serial Link driver.
* @version           : 3.1 — Added temperature set-point voice command
*********************************************************************************
*/

#ifndef SERIAL_AI_H
#define SERIAL_AI_H

#include <stdint.h>

// --- Voice command codes returned via out_voice_cmd ---
#define VOICE_CMD_NONE        0
#define VOICE_CMD_TURBO_ON    1
#define VOICE_CMD_TURBO_OFF   2
#define VOICE_CMD_SWING_ON    3
#define VOICE_CMD_SWING_OFF   4
#define VOICE_CMD_SET_TEMP    5   // Temperature setpoint — read out_voice_temp

void Serial_AI_Init(void);

/**
* Full protocol:
*
*   CAMERA — 3-byte sequence:
*     Byte 1 = person count   '0'–'6'
*     Byte 2 = leftmost zone  'L','C','R'
*     Byte 3 = rightmost zone 'L','C','R'
*
*   VOICE — single-byte commands:
*     'T' = Turbo ON    't' = Turbo OFF
*     'S' = Swing ON    's' = Swing OFF
*
*   VOICE TEMP — 2-byte sequence:
*     Byte 1 = 'W'  (set-point Write marker)
*     Byte 2 = temperature value as raw uint8 (18–40)
*     Sets out_voice_cmd = VOICE_CMD_SET_TEMP
*     Sets out_voice_temp = the temperature value
*
*   SHARED:
*     'H' = heartbeat
*
* @param out_voice_cmd   Set to VOICE_CMD_* constant each call
* @param out_voice_temp  Temperature value when cmd == VOICE_CMD_SET_TEMP
*/
uint8_t Serial_AI_Read(int     *out_person_count,
                        char    *out_servo_zone_start,
                        char    *out_servo_zone_end,
                        int     *out_timeout_ctr,
                        uint8_t *out_voice_cmd,
                        uint8_t *out_voice_temp);

#endif
