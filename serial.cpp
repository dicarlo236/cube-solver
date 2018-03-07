#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "serial.h"

int serial_fd = -1;

void send_sequence(uint8_t* seq, int size)
{
    if(serial_fd <= 0)
    {
        printf("[SERIAL] error bad serial.\n");
        printf("\tserial_fd: 0x%x\n",serial_fd);
        return;
    }

    printf("[SERIAL] sending buffer of length %d\n",size);

    write(serial_fd,"\n",1);
    write(serial_fd,(void*)seq,size);
    write(serial_fd,"\r",1);
    
    printf("[SERIAL] Done sending!\n");
}

int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tcgetattr: %s\n", errno,strerror(errno));
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
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
        printf("error %d from tcgetattr: %s\n", errno,strerror(errno));
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
        printf("error %d from tcgetattr: %s\n", errno,strerror(errno));
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf("error %d setting term attributes\n", errno);
}

void init_serial(const char* name)
{
    serial_fd = open(name,O_RDWR|O_NOCTTY|O_SYNC);
    if(serial_fd < 0)
    {
        printf("[SERIAL] error %d opening %s: %s\n",errno,name,strerror(errno));
        return;
    }
    printf("serial_fd: 0x%x\n",serial_fd);

    set_interface_attribs(serial_fd,B115200,0);
    set_blocking(serial_fd,1);
    printf("[SERIAL] Configuration done.\n");
}

void set_rts_pin(uint8_t on)
{
    int rts_pin = TIOCM_DTR;
    if(on)
    {
        //printf("[SERIAL] turning ON rts pin\n");
        ioctl(serial_fd,TIOCMBIC,&rts_pin);
    }
    else
    {
        //printf("[SERIAL] turning OFF rts pin\n");
        ioctl(serial_fd,TIOCMBIS,&rts_pin);
    }
}
