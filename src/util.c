/**
 * Various utility functions for use by device drivers
 *
 * Copyright (C) 2015 Tommy Vestermark
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

uint8_t reverse8(uint8_t x)
{
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    return x;
}


uint8_t crc4(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    unsigned remainder = init << 4; // LSBs are unused
    unsigned poly = polynomial << 4;
    unsigned bit;

    while (nBytes--) {
        remainder ^= *message++;
        for (bit = 0; bit < 8; bit++) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ poly;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder >> 4 & 0x0f; // discard the LSBs
}


uint8_t crc7(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    unsigned remainder = init << 1; // LSB is unused
    unsigned poly = polynomial << 1;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
        remainder ^= message[byte];
        for (bit = 0; bit < 8; ++bit) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ poly;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder >> 1 & 0x7f; // discard the LSB
}


uint8_t crc8(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init) {
    uint8_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
        remainder ^= message[byte];
        for (bit = 0; bit < 8; ++bit) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ polynomial;
            } else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder;
}


uint8_t crc8le(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    uint8_t crc = init, i;
    unsigned byte;
    uint8_t bit;

    for (byte = 0; byte < nBytes; ++byte) {
    for (i = 0x01; i & 0xff; i <<= 1) {
        bit = (crc & 0x80) == 0x80;
        if (message[byte] & i) {
        bit = !bit;
        }
        crc <<= 1;
        if (bit) {
        crc ^= polynomial;
        }
    }
    crc &= 0xff;
    }

    return reverse8(crc);
}

uint16_t crc16(uint8_t const message[], unsigned nBytes, uint16_t polynomial, uint16_t init)
{
    uint16_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
        remainder ^= message[byte];
        for (bit = 0; bit < 8; ++bit) {
            if (remainder & 1) {
                remainder = (remainder >> 1) ^ polynomial;
            }
            else {
                remainder = (remainder >> 1);
            }
        }
    }
    return remainder;
}

uint16_t crc16_ccitt(uint8_t const message[], unsigned nBytes, uint16_t polynomial, uint16_t init)
{
    uint16_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
        remainder ^= message[byte] << 8;
        for (bit = 0; bit < 8; ++bit) {
            if (remainder & 0x8000) {
                remainder = (remainder << 1) ^ polynomial;
            }
            else {
                remainder = (remainder << 1);
            }
        }
    }
    return remainder;
}


uint8_t lfsr_digest8(uint8_t const message[], unsigned bytes, uint8_t gen, uint8_t key)
{
    uint8_t sum = 0;
    for (unsigned k = 0; k < bytes; ++k) {
        uint8_t data = message[k];
        for (int i = 7; i >= 0; --i) {
            // fprintf(stderr, "key is %02x\n", key);
            // XOR key into sum if data bit is set
            if ((data >> i) & 1)
                sum ^= key;

            // roll the key right (actually the lsb is dropped here)
            // and apply the gen (needs to include the dropped lsb as msb)
            if (key & 1)
                key = (key >> 1) ^ gen;
            else
                key = (key >> 1);
        }
    }
    return sum;
}

uint16_t lfsr_digest16(uint32_t data, int bits, uint16_t gen, uint16_t key)
{
    uint16_t sum = 0;
    for (int bit = bits - 1; bit >= 0; --bit) {
        // fprintf(stderr, "key at bit %d : %04x\n", bit, key);
        // if data bit is set then xor with key
        if ((data >> bit) & 1)
            sum ^= key;

        // roll the key right (actually the lsb is dropped here)
        // and apply the gen (needs to include the dropped lsb as msb)
        if (key & 1)
            key = (key >> 1) ^ gen;
        else
            key = (key >> 1);
    }
    return sum;
}

/*
void lfsr_keys_fwd16(int rounds, uint16_t gen, uint16_t key)
{
    for (int i = 0; i <= rounds; ++i) {
        fprintf(stderr, "key at bit %d : %04x\n", i, key);

        // roll the key right (actually the lsb is dropped here)
        // and apply the gen (needs to include the dropped lsb as msb)
        if (key & 1)
            key = (key >> 1) ^ gen;
        else
            key = (key >> 1);
    }
}

void lfsr_keys_rwd16(int rounds, uint16_t gen, uint16_t key)
{
    for (int i = 0; i <= rounds; ++i) {
        fprintf(stderr, "key at bit -%d : %04x\n", i, key);

        // roll the key left (actually the msb is dropped here)
        // and apply the gen (needs to include the dropped msb as lsb)
        if (key & (1 << 15))
            key = (key << 1) ^ gen;
        else
            key = (key << 1);
    }
}
*/


int byteParity(uint8_t inByte)
{
    inByte ^= inByte >> 4;
    inByte &= 0xf;
    return (0x6996 >> inByte) & 1;
}


char* local_time_str(time_t time_secs, char *buf)
{
    time_t etime;
    struct tm *tm_info;

    if (time_secs == 0) {
        extern float sample_file_pos;
        if (sample_file_pos != -1.0) {
            snprintf(buf, LOCAL_TIME_BUFLEN, "@%fs", sample_file_pos);
            return buf;
        }
        time(&etime);
    } else {
        etime = time_secs;
    }

    tm_info = localtime(&etime); // note: win32 doesn't have localtime_r()

    strftime(buf, LOCAL_TIME_BUFLEN, "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

float celsius2fahrenheit(float celsius)
{
  return celsius * 9 / 5 + 32;
}


float fahrenheit2celsius(float fahrenheit)
{
    return (fahrenheit - 32) / 1.8;
}


float kmph2mph(float kmph)
{
    return kmph / 1.609344;
}

float mph2kmph(float mph)
{
    return mph * 1.609344;
}


float mm2inch(float mm)
{
    return mm * 0.039370;
}

float inch2mm(float inch)
{
    return inch / 0.039370;
}


float kpa2psi(float kpa)
{
    return kpa / 6.89475729;
}

float psi2kpa(float psi)
{
    return psi * 6.89475729;
}


float hpa2inhg(float hpa)
{
    return hpa / 33.8639;
}

float inhg2hpa(float inhg)
{
    return inhg * 33.8639;
}


bool str_endswith(const char *restrict str, const char *restrict suffix)
{
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    return (str_len >= suffix_len) &&
           (0 == strcmp(str + (str_len - suffix_len), suffix));
}

// Original string replacement function was found here:
// https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c/779960#779960
//
// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with)
{
    char *result;  // the return string
    char *ins;     // the next insert point
    char *tmp;     // varies
    int len_rep;   // length of rep (the string to remove)
    int len_with;  // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;     // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variables are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

// Make a more readable string for a frequency.
const char *nice_freq (double freq)
{
  static char buf[30];

  if (freq >= 1E9)
     snprintf (buf, sizeof(buf), "%.3fGHz", freq/1E9);
  else if (freq >= 1E6)
     snprintf (buf, sizeof(buf), "%.3fMHz", freq/1E6);
  else if (freq >= 1E3)
     snprintf (buf, sizeof(buf), "%.3fkHz", freq/1E3);
  else
     snprintf (buf, sizeof(buf), "%f", freq);
  return (buf);
}

// Test code
// gcc -I include/ -std=gnu99 -D _TEST src/util.c
#ifdef _TEST
int main(int argc, char **argv) {
    fprintf(stderr, "util:: test\n");

    uint8_t msg[] = {0x08, 0x0a, 0xe8, 0x80};

    fprintf(stderr, "util::crc8(): odd parity:  %02X\n", crc8(msg, 3, 0x80));
    fprintf(stderr, "util::crc8(): even parity: %02X\n", crc8(msg, 4, 0x80));

    return 0;
}
#endif /* _TEST */
