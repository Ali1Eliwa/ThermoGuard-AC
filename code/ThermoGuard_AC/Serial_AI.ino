/**
*********************************************************************************
* @file              : Serial_AI.ino
* @author            : Ziad Khalil, Ali Akram Ali, Mahmoud Ahmed El-Adgham
* @brief             : AI Serial Link driver — Native C UART, no Arduino libs.
* @version           : 3.1 — Added temperature set-point 2-byte protocol
*
* FULL PROTOCOL:
*
*   CAMERA (3-byte sequence, sent every ~500ms):
*     Byte 1 = person count digit  '0'-'6'
*     Byte 2 = leftmost  zone char 'L','C','R'
*     Byte 3 = rightmost zone char 'L','C','R'
*
*   VOICE — single-byte commands:
*     'T' = Turbo ON   't' = Turbo OFF
*     'S' = Swing ON   's' = Swing OFF
*
*   VOICE TEMP — 2-byte sequence:
*     Byte 1 = 'W'  (Write marker, safe — never used by camera protocol)
*     Byte 2 = temperature value as raw uint8 (18–40)
*
*   SHARED:
*     'H' = heartbeat
*
* parse_state values:
*   0 = idle       — waiting for any first byte
*   1 = cam_zone1  — got count digit, waiting for zone_start
*   2 = cam_zone2  — got zone_start, waiting for zone_end
*   3 = temp_val   — got 'W', waiting for temperature byte
*********************************************************************************
*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include "Serial_AI.h"
#include "Hardware_Defs.h"

static uint8_t parse_state   = 0;
static int     pending_count = 0;
static char    pending_start = 'C';

void Serial_AI_Init(void) {
    uint16_t ubrr = (uint16_t)((F_CPU / (16UL * AI_BAUD_RATE)) - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);
    UCSR0B = (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

uint8_t Serial_AI_Read(int     *out_person_count,
                        char    *out_servo_zone_start,
                        char    *out_servo_zone_end,
                        int     *out_timeout_ctr,
                        uint8_t *out_voice_cmd,
                        uint8_t *out_voice_temp)
{
    *out_voice_cmd  = VOICE_CMD_NONE;
    *out_voice_temp = 0;

    (*out_timeout_ctr)++;

    if (!(UCSR0A & (1 << RXC0))) {
        if (*out_timeout_ctr >= AI_TIMEOUT_LOOPS) {
            *out_person_count     = 0;
            *out_servo_zone_start = 'C';
            *out_servo_zone_end   = 'C';
            parse_state = 0;
        }
        return 0;
    }

    uint8_t byte = UDR0;
    *out_timeout_ctr = 0;

    // --- HEARTBEAT ---
    if (byte == 'H') {
        parse_state = 0;
        return 1;
    }

    // --- STATE 3: waiting for temperature value byte ---
    if (parse_state == 3) {
        if (byte >= 18 && byte <= 40) {
            *out_voice_cmd  = VOICE_CMD_SET_TEMP;
            *out_voice_temp = byte;
        }
        parse_state = 0;
        return 1;
    }

    // --- IDLE STATE ---
    if (parse_state == 0) {

        // Single-byte voice commands
        if (byte == 'T') { *out_voice_cmd = VOICE_CMD_TURBO_ON;  return 1; }
        if (byte == 't') { *out_voice_cmd = VOICE_CMD_TURBO_OFF; return 1; }
        if (byte == 'S') { *out_voice_cmd = VOICE_CMD_SWING_ON;  return 1; }
        if (byte == 's') { *out_voice_cmd = VOICE_CMD_SWING_OFF; return 1; }

        // 2-byte temperature command marker
        if (byte == 'W') {
            parse_state = 3;
            return 1;
        }

        // Camera 3-byte — count digit
        if (byte >= '0' && byte <= '0' + AI_MAX_PERSONS) {
            pending_count = byte - '0';
            if (pending_count == 0) {
                *out_person_count     = 0;
                *out_servo_zone_start = 'C';
                *out_servo_zone_end   = 'C';
                parse_state = 0;
            } else {
                parse_state = 1;
            }
            return 1;
        }
    }

    // --- STATE 1: waiting for zone_start ---
    else if (parse_state == 1) {
        if (byte == 'L' || byte == 'C' || byte == 'R') {
            pending_start = (char)byte;
            parse_state   = 2;
            return 1;
        }
        parse_state = 0;
    }

    // --- STATE 2: waiting for zone_end ---
    else if (parse_state == 2) {
        if (byte == 'L' || byte == 'C' || byte == 'R') {
            *out_person_count     = pending_count;
            *out_servo_zone_start = pending_start;
            *out_servo_zone_end   = (char)byte;
            parse_state = 0;
            return 1;
        }
        parse_state = 0;
    }

    return 0;
}
