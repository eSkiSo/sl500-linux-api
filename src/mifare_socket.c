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

#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTO_VER "1.0"
#define PIPEBUF_SIZE 50

enum cmds {
    CMD_WAIT_FOR_CARD = 0x10,
    CMD_CARD_ACK,

    CMD_CARD_DETECTED
};

enum rfid_states {
    STATE_IDLE = 0x00,
    STATE_WAIT_FOR_CARD
};

volatile int card_found = 0;
unsigned int card_no;
volatile unsigned int flash_on_found = 1;
int rf_fd;
volatile enum rfid_states rfid_state = STATE_IDLE;

void poll_loop()
{
    static unsigned int count = 0;
    static int flash_state = 0;
    static int acked = 1;

    if (rfid_state == STATE_IDLE) {
        acked = 1;
    }

    if (count % 2 == 0) {
        /* Look for card */
        rf_request(rf_fd);
        rf_anticoll(rf_fd, &card_no);

        if ((card_no) && (rfid_state == STATE_WAIT_FOR_CARD) && (acked)) {
            card_found = 1;
            acked = 0;

            if (flash_on_found) {
                flash_state = 3;
                rf_beep(rf_fd, 10);
            }
        }
    }

    if (flash_state == 0) {
        /* Blink green LED 200 ms every 2 s */
        if (count % 20 == 0) {
            rf_light(rf_fd, LED_GREEN);
        }
        if (count % 20 == 2) {
            rf_light(rf_fd, LED_OFF);
        }
    } else {
        /* Quick flash 5 times when card found */
        rf_light(rf_fd, LED_GREEN);
        usleep(50000);
        rf_light(rf_fd, LED_OFF);
        flash_state--;
        printf("Flashed!\n");
    }

    count++;
}

int pipe_send(int pipefd, enum cmds cmd, uint8_t bufsize, void *buf)
{
    int i;
    int status;

    if (bufsize > 0 && buf == NULL) {
        return -1;
    }

    status = write(pipefd, &cmd, sizeof(cmd));

    status = write(pipefd, &bufsize, sizeof(bufsize));

    for (i=0; i<bufsize; i++) {
        write(pipefd, buf++, 1);
    }

    return 0;
}

int pipe_rcv(int pipefd, enum cmds *cmd, uint8_t bufsize, void *buf)
{
    uint8_t len;
    uint8_t trash;
    int i = 0;
    int num;

    while ((num = read(pipefd, cmd, sizeof(*cmd))) == -1);
    if (num == 0) {
        fprintf(stderr, "pipe: EOF\n");
        exit(EXIT_FAILURE);
    }

    while ((num = read(pipefd, &len, sizeof(len))) == -1);
    assert(num > 0);

    if (buf != NULL) {
        for (i=0; i<min(len, bufsize); i++) {
            while ((num = read(pipefd, buf++, 1)) == -1);
            assert(num > 0);
        }
    }

    for (; i<len; i++) {
        while ((num = read(pipefd, &trash, 1)) == -1);
        assert(num > 0);
    }

    return len;
}

void rfid_process(int net_rfid[], int rfid_net[], int rfid_fd)
{
    enum cmds cmd;
    struct itimerval loop_ival;
    struct sigaction alrm_action;

    /* Close unused pipes */
    close(net_rfid[1]);
    close(rfid_net[0]);

    rf_fd = rfid_fd;

    /* Set timer to 100 ms */
    loop_ival.it_interval.tv_sec = 0;
    loop_ival.it_interval.tv_usec = 100000;
    loop_ival.it_value.tv_sec = 5;
    loop_ival.it_value.tv_usec = 10;

    alrm_action.sa_handler = poll_loop;
    sigemptyset (&alrm_action.sa_mask);
    alrm_action.sa_flags = 0;
    sigaction(SIGALRM, &alrm_action, NULL);

    setitimer(ITIMER_REAL, &loop_ival, NULL);

    while (1) {
        /* poll_loop is running until card is found */

        /* Wait for command */
        printf("RFID: Waiting for command.\n");
        pipe_rcv(net_rfid[0], &cmd, 0, NULL);

        printf("RFID: got command\n");
        switch (cmd) {
            case CMD_WAIT_FOR_CARD:
                printf("RFID: Received CMD_WAIT_FOR_CARD.\n");
                card_found = 0;
                rfid_state = STATE_WAIT_FOR_CARD;
                while (!card_found) {
                    usleep(10000000);
                }
                rfid_state = STATE_IDLE;

                printf("RFID: Sending CMD_CARD_DETECTED.\n");
                pipe_send(rfid_net[1], CMD_CARD_DETECTED, sizeof(card_no), &card_no);
                break;
            default:
                break;
        }
    }

    rf_light(rfid_fd, LED_RED);
}

void network_process(int rfid_net[], int net_rfid[])
{
    char buf[50];
    char ch;
    char client_protocol[10];
    int bufpos = 0;
    int handshake;
    int net_fd;
    int sock;
    int status;
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    socklen_t client_size;

    client_size = sizeof(client_addr);

    /* Close unused pipes */
    close(net_rfid[0]);
    close(rfid_net[1]);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(3333);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    for (;;) {

        if (listen(sock, 1) == -1) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        if ((net_fd = accept(sock, (struct sockaddr *)&client_addr, &client_size)) == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        handshake = 0;
        bufpos = 0;

        while (read(net_fd, &ch, 1)) {
            if (bufpos < sizeof(buf)) {
                if (ch == '\r') {
                    buf[bufpos++] = '\0';
                } else if (ch == '\n') {
                } else {
                    buf[bufpos++] = ch;
                }
            } else {
                /* Buffer overrun. Reset buffer. */
                bufpos = 0;
            }

            if (ch == '\r' && bufpos > 0) {
                /* Parse command */

                if ((strncmp(buf, "client_protocol ", 16) == 0) &&
                        (strlen(&buf[16])) > 0 &&
                        (strlen(&buf[16])) < 10) {
                    strncpy(client_protocol, &buf[9], 9);
                    client_protocol[9] = '\0';
                    handshake = 1;
                    sprintf(buf, "server_protocol %s\n", PROTO_VER);
                    write(net_fd, buf, strlen(buf));
                } else if (strcmp(buf, "exit") == 0) {
                    break;
                } else {

                    if (handshake) {
                        enum cmds cmd;
                        int status;
                        unsigned int card_no;

                        if (strcmp(buf, "wait_for_card") == 0) {
                            printf("NET: Sending CMD_WAIT_FOR_CARD.\n");
                            pipe_send(net_rfid[1], CMD_WAIT_FOR_CARD, 0, NULL);

                            printf("NET: Waiting for CMD_CARD_DETECTED.\n");
                            status = pipe_rcv(rfid_net[0], &cmd, sizeof(card_no), &card_no);
                            if (status == -1) {
                                printf("NET: ERROR.\n");
                            } else if (status == 0) {
                                printf("NET: Nothing received.\n");
                            }
                            printf("NET: Got cmd: %u.\n", cmd);
                            if (cmd == CMD_CARD_DETECTED) {
                                printf("NET: Received CMD_CARD_DETECTED\n");
                                sprintf(buf, "card_detected %u\n", card_no);
                                write(net_fd, buf, strlen(buf));
                            }
                        } else {
                            sprintf(buf, "Syntax error\n");
                            write(net_fd, buf, strlen(buf));
                        }
                    } else {
                        /* Protocol version not received */
                        sprintf(buf, "Please provide protocol version.\n");
                        write(net_fd, buf, strlen(buf));
                    }
                }

                bufpos = 0;
            } // Parse command
        } // while(read())
        if (status == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("NET: Kill client.\n");

        if (close(net_fd) == -1) {
            perror("close net_fd");
            exit(EXIT_FAILURE);
        }

    } // for(;;)

    if (close(sock) == -1) {
        perror("close sock");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    int net_rfid[2];
    int rfid_fd;
    int rfid_net[2];
    pid_t cpid;

    /* Set up serial port */
    rfid_fd = open_port();

    /* Turn off LED */
    rf_light(rfid_fd, LED_OFF);

    if (pipe(net_rfid) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(rfid_net) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    cpid = fork();

    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {
        network_process(rfid_net, net_rfid);
    } else {
        rfid_process(net_rfid, rfid_net, rfid_fd);
    }

    return 0;
}

