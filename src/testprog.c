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

int main()
{
    int fd;
    uint8_t dev_id[2], buf[100];
    uint8_t new_dev_id[] = {0x13, 0x1a};

    /* Set up serial port */
    fd = open_port();

    printf("\nrf_get_model\n");
    rf_get_model(fd, sizeof(buf), buf);
    printf("Model: %s\n", buf);

    printf("\nrf_get_device_number\n");
    rf_get_device_number(fd, dev_id);
    printf("Device ID: %02hhx %02hhx\n", dev_id[0], dev_id[1]);

    printf("\nrf_init_device_number\n");
    rf_init_device_number(fd, new_dev_id);

    printf("\nrf_get_device_number\n");
    rf_get_device_number(fd, dev_id);
    printf("Device ID: %02hhx %02hhx\n", dev_id[0], dev_id[1]);

    printf("\nrf_light\n");
    rf_light(fd, LED_OFF);

    printf("\nrf_request\n");
    rf_request(fd);

    printf("\nrf_anticoll\n");
    unsigned int card_no;
    rf_anticoll(fd, &card_no);
    printf("Card number: %u\n", card_no);

    printf("\nrf_select\n");
    rf_select(fd, sizeof(card_no), &card_no);

/*    printf("\nrf_M1_authentication2\n");
    rf_M1_authentication2(fd);

    printf("\nrf_M1_write\n");
    rf_M1_write(fd);

    printf("\nrf_M1_read\n");
    rf_M1_read(fd);*/

    /*int i;
    for (i=0; i<3; i++) {
        rf_light(fd, LED_RED);
        usleep(300000);
        rf_light(fd, LED_GREEN);
        usleep(300000);
        rf_light(fd, LED_OFF);
        usleep(300000);
        rf_beep(fd, 2);
        usleep(300000);
    }*/

    return 0;
}

