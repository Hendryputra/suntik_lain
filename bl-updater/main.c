#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>


int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf("error %d from tcgetattr\n", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf("error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf("error %d from tggetattr\n", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 0;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf("error %d setting term attributes\n", errno);
}


int main(int argc, char **argv)
{
    if(argc != 3) {
        printf("Usage: btct-updater [port] [file].\n");
        return -1;
    }
    int fd = open (argv[1], O_RDWR | O_NOCTTY | O_SYNC);
    if(fd < 0) {
        printf("Unable to open %s!\n", argv[1]);
        return 0;
    }

    set_interface_attribs(fd, B115200, 0); // Configure serial port to 115200 baud.
    set_blocking(fd, 1); // Don't block on read().

    FILE *fp;
    fp = fopen(argv[2], "rb"); // Open file to be sent.
    if(fp == NULL) {
        printf("Error opening file %s!\n", argv[2]);
        return -1;
    }

    unsigned char c;
    unsigned char buf[4096*2];
    int sent = 0, sentchunks = 0;

    while(1) {
        buf[sent] = getc(fp); // Fill buffer with data.
        if(feof(fp)) // Abort on EOF.
            break;
        sent++;
        if(sent >= 4096*2) { // 4k chunk read.
            write(fd, buf, sent); // Sent to bootloader.
            printf("Chunks sent: %i, last chunk: %i.\n", ++sentchunks, sent);
            sent = 0;
            while(c != 0x11) // Wait for response from bootloader.
                read(fd, &c, 1);
            c = 0x00;
        }
    }
    if(sent >= 1) { // Send last chunk of data.
        write(fd, buf, sent);
        printf("Chunks sent: %i, last chunk: %i.\n", ++sentchunks, sent);
    }

    return 0;
}
