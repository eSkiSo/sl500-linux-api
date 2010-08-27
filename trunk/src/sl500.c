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

void comm_error()
{
    fprintf(stderr, "Communication error, aborting!\n");
    exit(1);
}

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port(void)
{
    int fd; /* File descriptor for the port */


    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        /*
         * Could not open the port.
         */

        perror("open_port: Unable to open /dev/ttyUSB0 - ");
    }
    else
    {
        fcntl(fd, F_SETFL, 0);
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B19200);
    cfsetospeed(&options, B19200);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSTOPB;
    cfmakeraw(&options);
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

uint8_t get_byte(int fd)
{
    uint8_t res;
    if (read(fd, &res, 1) != 1)
        comm_error();
    return res;
}

void expect(int fd, uint8_t expected)
{
    uint8_t res = get_byte(fd);
    if (res != expected) {
        fprintf(stderr, "Expected 0x%02hhx, but got 0x%02hhx.\n", expected, res);
        exit(1);
    }
}

void send_command(int fd, uint8_t dev_id[2], uint8_t cmd_code[2],
                  uint8_t param_len, uint8_t *param)
{
    uint8_t buf[8];
    uint8_t ver = 0x00;
    const uint8_t zero = 0x00;
    int i;
#ifdef DEBUG_LOW_LEVEL
    char printbuf[100 + param_len * 10];
    int pos = 0;
#endif

    buf[0] = 0xaa;                      // Command head
    buf[1] = 0xbb;
    buf[2] = 5 + param_len;             // Length
    buf[3] = 0x00;
    buf[4] = dev_id[0];                 // Device ID
    buf[5] = dev_id[1];
    buf[6] = cmd_code[0];               // Command code
    buf[7] = cmd_code[1];

#ifdef DEBUG_LOW_LEVEL
    fprintf(stderr, "¤¤¤ COMMAND   Length: %2d, Command code: %02hhx %02hhx, Parameter: ", buf[2], cmd_code[0], cmd_code[1]);
    for (i=0; i<param_len; i++)
    {
        pos += sprintf(&printbuf[pos], "%02hhx ", param[i]);
    }
    pos += sprintf(&printbuf[pos], "\nSent bytes: ");
    for (i=0; i<8; i++)
    {
        pos += sprintf(&printbuf[pos], "%02hhx ", buf[i]);
    }
#endif

    write(fd, buf, 8);

    /* Calculate verification of sent bytes */
    for (i=4; i<8; i++)
        ver ^= buf[i];

    for (i=0; i<param_len; i++) {
        ver ^= param[i];
        write(fd, &param[i], 1);
#ifdef DEBUG_LOW_LEVEL
        pos += sprintf(&printbuf[pos], "%02hhx ", param[i]);
#endif

        /* Avoid writing the command head 0xaabb */
        if (param[i] == 0xaa) {
#ifdef DEBUG_LOW_LEVEL
            pos += sprintf(&printbuf[pos], "0x00 ");
#endif
            write(fd, &zero, 1);
        }
    }

    /* Write verification */
    write(fd, &ver, 1);
#ifdef DEBUG_LOW_LEVEL
    pos += sprintf(&printbuf[pos], "%02hhx", ver);
    fprintf(stderr, "%s\n", printbuf);
#endif
}

int receive_response(int fd, uint8_t *dev_id, uint8_t *cmd_code,
                     uint8_t *status, int data_len, uint8_t *data)
{
    uint8_t len, tmp[2];
    uint8_t ver = 0x00;
    uint8_t act_ver;
    int i;

#ifdef DEBUG_LOW_LEVEL
    int pos = 0;
    char printbuf[200];

    pos += sprintf(&printbuf[pos], "¤¤¤ RESPONSE  ");
#endif

    expect(fd, 0xaa);                   // Command head
    expect(fd, 0xbb);
    len = get_byte(fd);                 // Length
    expect(fd, 0x00);

#ifdef DEBUG_LOW_LEVEL
    pos += sprintf(&printbuf[pos], "Length: %02hhx, ", len);
#endif

    tmp[0] = get_byte(fd);              // Device ID
    tmp[1] = get_byte(fd);
    ver ^= tmp[0];
    ver ^= tmp[1];
#ifdef DEBUG_LOW_LEVEL
    pos += sprintf(&printbuf[pos], "Device ID: %02hhx %02hhx, ", tmp[0], tmp[1]);
#endif
    if (dev_id != NULL) {
        *dev_id = tmp[0];
    }

    tmp[0] = get_byte(fd);              // Command code
    tmp[1] = get_byte(fd);
    ver ^= tmp[0];
    ver ^= tmp[1];
    if (cmd_code != NULL) {
        cmd_code[0] = tmp[0];
        cmd_code[1] = tmp[1];
    }
#ifdef DEBUG_LOW_LEVEL
    pos += sprintf(&printbuf[pos], "Command code: %02hhx %02hhx, ", tmp[0], tmp[1]);
#endif

    tmp[0] = get_byte(fd);              // Status
    ver ^= tmp[0];
    if (status != NULL) {
        *status = tmp[0];
    }
#ifdef DEBUG_LOW_LEVEL
    pos += sprintf(&printbuf[pos], "Status: %02hhx\nData: ", tmp[0]);
#endif

    for (i=0; i<len-6; i++) {
        tmp[0] = get_byte(fd);          // Data range
        ver ^= tmp[0];
        if (i<data_len && data != NULL) {
            data[i] = tmp[0];
        }
#ifdef DEBUG_LOW_LEVEL
        pos += sprintf(&printbuf[pos], "%02hhx ", data[i]);
#endif
        if (tmp[0] == 0xaa) {
            expect(fd, 0x00);
        }
    }
#ifdef DEBUG_LOW_LEVEL
    fprintf(stderr, "%s\n", printbuf);
#endif

    //expect(fd, ver);                    // Verification
    act_ver = get_byte(fd); // Just consume verification for now

    if (act_ver != ver) {
        printf("WARNING: Verification should be %02hhx but was %02hhx.\n", ver, act_ver);
    }

    return len-6;
}

uint8_t rf_init_com(int fd, uint8_t rate)
{
    uint8_t cmd_code[] = {0x01, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;
    struct termios options;
    speed_t new_speed;

    /* Handle unsupported baud rates */
    if (rate == BAUD_14400 || rate == BAUD_28800 || rate > BAUD_115200)
        return -1;

    send_command(fd, dev, cmd_code, 1, &rate);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    if (status == 0x00) {
        tcgetattr(fd, &options);
        switch (rate) {
            case BAUD_4800:
                new_speed = B4800;
                break;
            case BAUD_9600:
                new_speed = B9600;
                break;
            case BAUD_19200:
                new_speed = B19200;
                break;
            case BAUD_38400:
                new_speed = B38400;
                break;
            case BAUD_57600:
                new_speed = B57600;
                break;
            case BAUD_115200:
                new_speed = B115200;
                break;
            default:
                new_speed = cfgetispeed(&options);
        }
        cfsetispeed(&options, new_speed);
        cfsetospeed(&options, new_speed);
        tcsetattr(fd, TCSANOW, &options);
    }

    return status;
}

uint8_t rf_get_model(int fd, int data_len, uint8_t *data)
{
    uint8_t cmd_code[] = {0x04, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 0, NULL);
    receive_response(fd, NULL, NULL, &status, data_len, data);

    return status;
}

uint8_t rf_init_device_number(int fd, uint8_t dev_id[2])
{
    uint8_t cmd_code[] = {0x02, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 2, dev_id);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    return status;
}

uint8_t rf_get_device_number(int fd, uint8_t *dev_id)
{
    uint8_t cmd_code[] = {0x03, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 0, NULL);
    receive_response(fd, NULL, NULL, &status, 2, dev_id);

    return status;
}

uint8_t rf_beep(int fd, uint8_t time)
{
    uint8_t cmd_code[] = {0x06, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 1, &time);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    return status;
}

uint8_t rf_light(int fd, uint8_t color)
{
    uint8_t cmd_code[] = {0x07, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 1, &color);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    return status;
}

uint8_t rf_init_type(int fd, uint8_t mode)
{
    uint8_t cmd_code[] = {0x08, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 1, &mode);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    return status;
}

uint8_t rf_antenna_sta(int fd, uint8_t state)
{
    uint8_t cmd_code[] = {0x0c, 0x01};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;

    send_command(fd, dev, cmd_code, 1, &state);
    receive_response(fd, NULL, NULL, &status, 0, NULL);

    return status;
}

uint8_t rf_request(int fd)
{
    uint8_t cmd_code[] = {0x01, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t mode = REQ_ALL;
    uint8_t status;

    send_command(fd, dev, cmd_code, 1, &mode);
    receive_response(fd, NULL, NULL, &status, sizeof(buf), buf);

    return status;
}

uint8_t rf_anticoll(int fd, unsigned int *card_no)
{
    uint8_t cmd_code[] = {0x02, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t status;
    int count;

    send_command(fd, dev, cmd_code, 0, NULL);
    count = receive_response(fd, NULL, NULL, &status, sizeof(buf), buf);

    if (status == 0x00) {
        /* If the card ID is 4 bytes,
         * copy it to card_no.
         */
        if (count == 4) {
            ((uint8_t*) card_no)[0] = buf[0];
            ((uint8_t*) card_no)[1] = buf[1];
            ((uint8_t*) card_no)[2] = buf[2];
            ((uint8_t*) card_no)[3] = buf[3];
        } else {
#ifdef DEBUG_COMMANDS
            printf("ID length: %d\n", count);
#endif
            *card_no = 0;
        }

#ifdef DEBUG_COMMANDS
        printf("CARD NO: %u\n", *card_no);
#endif
    } else {
        *card_no = 0;
#ifdef DEBUG_COMMANDS
        printf("ERROR\n");
#endif
    }

    return status;
}

uint8_t rf_select(int fd, int cardnbr_size, uint8_t *cardnbr)
{
    uint8_t cmd_code[] = {0x03, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t status;

    send_command(fd, dev, cmd_code, cardnbr_size, cardnbr);
    receive_response(fd, NULL, NULL, &status, sizeof(buf), buf);

#ifdef DEBUG_COMMANDS
    if (status == 0) {
        printf("Capacity: %02hhx\n", buf[0]);
    } else {
        printf("ERROR\n");
    }
#endif

    return status;
}

uint8_t rf_halt(int fd)
{
    uint8_t cmd_code[] = {0x04, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t status;

    send_command(fd, dev, cmd_code, 0, NULL);
    receive_response(fd, NULL, NULL, &status, sizeof(buf), buf);

    return status;
}

uint8_t rf_M1_authentication2(int fd, uint8_t key_type, uint8_t block, uint8_t key[6])
{
    uint8_t cmd_code[] = {0x07, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t status;
    int i;
    uint8_t data[8] = {key_type, block};
    memcpy(&data[2], key, 6);

#ifdef DEBUG_COMMANDS
    fprintf(stderr, "Authenticating block %d (0x%02hhx) with key", block, block);
    for (i=0; i<6; i++)
    {
        fprintf(stderr, " %02hhx", key[i]);
    }

    fprintf(stderr, "...\n");
#endif

    send_command(fd, dev, cmd_code, sizeof(data), data);
    receive_response(fd, NULL, NULL, &status, sizeof(buf), buf);

    return status;
}

uint8_t rf_M1_read(int fd, uint8_t block, uint8_t *content)
{
    uint8_t cmd_code[] = {0x08, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t status;
    int i;
    uint8_t data[] = {block};
    char printbuf[100];

    send_command(fd, dev, cmd_code, sizeof(data), data);
    receive_response(fd, NULL, NULL, &status, 16, content);

#ifdef DEBUG_COMMANDS
    int pos = 0;
    if (status == 0)
    {
        fprintf(stderr, "Block %3d (0x%02hhx):", block, block);
        for (i=0; i<16; i++)
        {
            pos += sprintf(&printbuf[pos], " %02hhx", content[i]);
        }
        fprintf(stderr, "%s\n", printbuf);
    }
    else
    {
        fprintf(stderr, "Block %3d (0x%02hhx) could not be read.\n", block, block);
    }
#endif

    return status;
}










void rf_M1_write(int fd)
{
    uint8_t cmd_code[] = {0x09, 0x02};
    uint8_t dev[] = {0x00, 0x00};
    uint8_t buf[100];
    uint8_t data[] = {0x04, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

    send_command(fd, dev, cmd_code, sizeof(data), data);
}

