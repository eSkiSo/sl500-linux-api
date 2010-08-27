// vim: ts=4 expandtab ai

/*
 * Copyright (c) 2008, Henrik Torstensson <laban@kryo.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "sl500.h"

#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

void shutdown(int fd, int errorcode)
{
    printf("\nResetting communication speed to 19200 baud...\n");
    rf_init_com(fd, BAUD_19200);
    exit(errorcode);
}

int main()
{
    int fd;
    uint8_t dev_id[2], buf[100];
    uint8_t new_dev_id[] = {0x13, 0x1a};
    uint8_t key[6];
    uint8_t status;
    char printbuf[100];
    int block, i, pos, authed;
    unsigned int card_no;

    /* Set up serial port */
    fd = open_port();

    printf("\nSpeeding up communication to 115200 baud...\n");
    rf_init_com(fd, BAUD_115200);

    rf_get_model(fd, sizeof(buf), buf);
    printf("Model: %s\n", buf);

    rf_light(fd, LED_OFF);

    /* START, MIFARE COMMANDS */

    printf("Request all\n");
    status = rf_request(fd);
    if (status == 20)
    {
        printf("No card - exiting...\n");
        shutdown(fd, status);
    }
    else if (status != 0)
    {
        printf("ERROR %d\n", status);
        shutdown(fd, status);
    }

    printf("Anticollision\n");
    status = rf_anticoll(fd, &card_no);
    if (status != 0)
    {
        printf("ERROR %d\n", status);
        shutdown(fd, status);
    }
    printf("Card number: %u (0x%08x)\n", card_no, card_no);

    printf("Selecting card\n");
    status = rf_select(fd, sizeof(card_no), (uint8_t*)&card_no);
    if (status != 0)
    {
        printf("ERROR %d\n", status);
        shutdown(fd, status);
    }

    printf("\nDumping card contents...\n");

    memset(key, 0xff, 6);
    for (block=0; block<256; block++)
    {
        if (block % 4 == 0)
        {
            status = rf_M1_authentication2(fd, KEY_A, block, key);
            authed = (status == 0);
        }
        if (authed)
        {
            rf_M1_read(fd, block, buf);
            printf("Block %3d (0x%02hhx):", block, block);
            int i;
            pos = 0;
            for (i=0; i<16; i++)
            {
                pos += sprintf(&printbuf[pos], " %02hhx", buf[i]);
            }
            printf("%s\n", printbuf);
        }
        else
        {
            printf("Access denied to block %d (%02hhx)\n", block, block);
        }
    }

    shutdown(fd, 0);
}

