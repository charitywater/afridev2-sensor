

/** 
 * @file minmea.c
 * \n Source File
 * \n Afridev MSP430 Firmware
 * 
 * \brief Routines to parse GPS data.  Developed by a third party (see license below)
 */

/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The F*** You Want To Public License, Version 2, as
 * published by Sam Hocevar. 
 */

#include "minmea.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

static inline bool minmea_isfield(char c)
{
    return (isprint((unsigned char)c) && c != ',' && c != '*');
}

bool minmea_scan(const char *sentence, const char *format, ...)
{
    bool result = false;
    bool optional = false;
    va_list ap;

    va_start(ap, format);
    int f;

    const char *field = sentence;
#define next_field() \
    do { \
        /* Progress to the next field. */ \
        while (minmea_isfield(*sentence)) \
            sentence++; \
        /* Make sure there is a field there. */ \
        if (*sentence == ',') { \
            sentence++; \
            field = sentence; \
        } else { \
            field = NULL; \
        } \
    } while (0)

    while (*format)
    {
        char type = *format++;

        if (type == ';')
        {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!field && !optional)
        {
            // Field requested but we ran out if input. Bail out.
            goto parse_error;
        }

        switch (type)
        {
            case 'c':
            {                                              // Single character field (char).
                char value = '\0';

                if (field && minmea_isfield(*field))
                    value = *field;

                *va_arg(ap, char *) = value;
            }
                break;

            case 'd':
            {                                              // Single character direction field (int).
                int value = 0;

                if (field && minmea_isfield(*field))
                {
                    switch (*field)
                    {
                        case 'N':
                        case 'E':
                            value = 1;
                            break;
                        case 'S':
                        case 'W':
                            value = -1;
                            break;
                        default:
                            goto parse_error;
                    }
                }

                *va_arg(ap, int *) = value;
            }
                break;

            case 'f':
            {                                              // Fractional value with scale (struct minmea_float).
                int sign = 0;
                int_least32_t value = -1;
                int_least32_t scale = 0;

                if (field)
                {
                    while (minmea_isfield(*field))
                    {
                        if (*field == '+' && !sign && value == -1)
                        {
                            sign = 1;
                        }
                        else if (*field == '-' && !sign && value == -1)
                        {
                            sign = -1;
                        }
                        else if (isdigit((unsigned char)*field))
                        {
                            int digit = *field - '0';

                            if (value == -1)
                                value = 0;
                            if (value > (INT_LEAST32_MAX - digit) / 10)
                            {
                                /* we ran out of bits, what do we do? */
                                if (scale)
                                {
                                    /* truncate extra precision */
                                    break;
                                }
                                else
                                {
                                    /* integer overflow. bail out. */
                                    goto parse_error;
                                }
                            }
                            value = (10 * value) + digit;
                            if (scale)
                                scale *= 10;
                        }
                        else if (*field == '.' && scale == 0)
                        {
                            scale = 1;
                        }
                        else if (*field == ' ')
                        {
                            /* Allow spaces at the start of the field. Not NMEA
                             * conformant, but some modules do this. */
                            if (sign != 0 || value != -1 || scale != 0)
                                goto parse_error;
                        }
                        else
                        {
                            goto parse_error;
                        }
                        field++;
                    }
                }

                if ((sign || scale) && value == -1)
                    goto parse_error;

                if (value == -1)
                {
                    /* No digits were scanned. */
                    value = 0;
                    scale = 0;
                }
                else if (scale == 0)
                {
                    /* No decimal point. */
                    scale = 1;
                }
                if (sign)
                    value *= sign;

                *va_arg(ap, struct minmea_float *) = (struct minmea_float){ value, scale };
            }
                break;

            case 'i':
            {                                              // Integer value, default 0 (int).
                int value = 0;

                if (field)
                {
                    char *endptr;

                    value = strtol(field, &endptr, 10);
                    if (minmea_isfield(*endptr))
                        goto parse_error;
                }

                *va_arg(ap, int *) = value;
            }
                break;

            case 's':
            {                                              // String value (char *).
                char *buf = va_arg(ap, char *);

                if (field)
                {
                    while (minmea_isfield(*field)) *buf++ = *field++;
                }

                *buf = '\0';
            }
                break;

            case 't':
            {                                              // NMEA talker+sentence identifier (char *).
                                                           // This field is always mandatory.
                if (!field)
                    goto parse_error;

                if (field[0] != '$')
                {
                    goto parse_error;
                }
                for (f = 0; f < 5; f++)
                {
                    if (!minmea_isfield(field[1 + f]))
                    {
                        goto parse_error;
                    }
                }

                char *buf = va_arg(ap, char *);

                memcpy(buf, field + 1, 5);
                buf[5] = '\0';
            }
                break;

            case 'D':
            {                                              // Date (int, int, int), -1 if empty.
                struct minmea_date *date = va_arg(ap, struct minmea_date *);

                int d = -1, m = -1, y = -1;

                if (field && minmea_isfield(*field))
                {
                    int f;

                    // Always six digits.
                    for (f = 0; f < 6; f++) if (!isdigit((unsigned char)field[f]))
                        goto parse_error;

                    char dArr[] = { field[0], field[1], '\0' };
                    char mArr[] = { field[2], field[3], '\0' };
                    char yArr[] = { field[4], field[5], '\0' };

                    d = strtol(dArr, NULL, 10);
                    m = strtol(mArr, NULL, 10);
                    y = strtol(yArr, NULL, 10);
                }

                date->day = d;
                date->month = m;
                date->year = y;
            }
                break;

            case 'T':
            {                                              // Time (int, int, int, int), -1 if empty.
                struct minmea_time *time_ = va_arg(ap, struct minmea_time *);

                int h = -1, i = -1, s = -1, u = -1;

                if (field && minmea_isfield(*field))
                {
                    int f;

                    // Minimum required: integer time.
                    for (f = 0; f < 6; f++) if (!isdigit((unsigned char)field[f]))
                        goto parse_error;

                    char hArr[] = { field[0], field[1], '\0' };
                    char iArr[] = { field[2], field[3], '\0' };
                    char sArr[] = { field[4], field[5], '\0' };

                    h = strtol(hArr, NULL, 10);
                    i = strtol(iArr, NULL, 10);
                    s = strtol(sArr, NULL, 10);
                    field += 6;

                    // Extra: fractional time. Saved as microseconds.
                    if (*field++ == '.')
                    {
                        uint32_t value = 0;
                        uint32_t scale = 1000000LU;

                        while (isdigit((unsigned char)*field) && scale > 1)
                        {
                            value = (value * 10) + (*field++ - '0');
                            scale /= 10;
                        }
                        u = value * scale;
                    }
                    else
                    {
                        u = 0;
                    }
                }

                time_->hours = h;
                time_->minutes = i;
                time_->seconds = s;
                time_->microseconds = u;
            }
                break;

            case '_':
            {                                              // Ignore the field.
            }
                break;

            default:
            {                                              // Unknown.
                goto parse_error;
            }
        }

        next_field();
    }

    result = true;

parse_error:
    va_end(ap);
    return (result);
}

bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence)
{
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!minmea_scan(sentence, "tTfdfdiiffcfci_",
                     type,
                     &frame->time,
                     &frame->latitude, &latitude_direction,
                     &frame->longitude, &longitude_direction,
                     &frame->fix_quality,
                     &frame->satellites_tracked,
                     &frame->hdop,
                     &frame->altitude, &frame->altitude_units,
                     &frame->height, &frame->height_units,
                     &frame->dgps_age))
    {
        return (false);
    }

    if (strcmp(type + 2, "GGA"))
    {
        return (false);
    }

    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;

    return (true);
}
