// vim: ts=4 expandtab ai

#ifndef SL500_H
#define SL500_H

/*
 * Copyright (c) 2007, Henrik Torstensson <laban@kryo.se>
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

#include <stdint.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define BAUD_4800 0x00
#define BAUD_9600 0x01
#define BAUD_14400 0x02
#define BAUD_19200 0x03
#define BAUD_28800 0x04
#define BAUD_38400 0x05
#define BAUD_57600 0x06
#define BAUD_115200 0x07

/*
 * Use (LED_RED|LED_GREEN) to turn both red and green lights on.
 * Documented as "yellow", but doesn't look like it.
 */
#define LED_OFF (0x00)
#define LED_RED (0x01)
#define LED_GREEN (0x02)

#define TYPE_A 'A'
#define TYPE_B 'B'
#define ISO15693 '1'

#define RF_OFF (0x00)
#define RF_ON (0x01)

#define REQ_STD (0x26)
#define REQ_ALL (0x52)

#define KEY_A (0x60)
#define KEY_B (0x61)

void comm_error();

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(void);

uint8_t get_byte(int fd);

void expect(int fd, uint8_t expected);

void send_command(int fd, uint8_t dev_id[2], uint8_t cmd_code[2],
                  uint8_t param_len, uint8_t *param);

int receive_response(int fd, uint8_t *dev_id, uint8_t *cmd_code,
                     uint8_t *status, int data_len, uint8_t *data);

uint8_t rf_init_com(int fd, uint8_t rate);

uint8_t rf_get_model(int fd, int data_len, uint8_t *data);

uint8_t rf_init_device_number(int fd, uint8_t dev_id[2]);

uint8_t rf_get_device_number(int fd, uint8_t *dev_id);

uint8_t rf_beep(int fd, uint8_t time);

uint8_t rf_light(int fd, uint8_t color);

uint8_t rf_init_type(int fd, uint8_t mode);

uint8_t rf_antenna_sta(int fd, uint8_t state);

uint8_t rf_request(int fd);

uint8_t rf_anticoll(int fd, unsigned int *card_no);

uint8_t rf_select(int fd, int cardnbr_size, uint8_t *cardnbr);

uint8_t rf_halt(int fd);

uint8_t rf_M1_authentication2(int fd, uint8_t key_type, uint8_t block, uint8_t key[6]);

uint8_t rf_M1_read(int fd, uint8_t block, uint8_t *content);

void rf_M1_write(int fd);

#endif

