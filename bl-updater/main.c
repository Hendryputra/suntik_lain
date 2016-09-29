#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

void delay(int delay_t)
{
   int c = 1, d = 1;
 
   for ( c = 1 ; c <= delay_t ; c++ )
       for ( d = 1 ; d <= delay_t*2 ; d++ )
       {}
 
   
}


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
    char buf[4096*2];
    
    int sent = 0, sentchunks = 0,xc=0,skip=0,len=0;
	char buf2[2];
	buf2[1]='\0';
    while(1) {
       
         
        	buf2[0]=getc(fp);
				  
				if (buf2[0] == '\r' ){
					  continue;
				}if (buf2[0] == '\n' ){
					continue;
				}
				  
				if (buf2[0] != ':' && skip==0){
					 printf("Missing start of record (':') wrong byte %c / %02X\n",buf2[xc], buf2[xc]);
					 break;
				}else if(skip<=8){
					//:,1,0,0,0,0,0,0,0,  
					
					if(skip==1){
					len=(buf2[0]-'0')*16;
					
					}
					
					if(skip==2){
					len=len+buf2[0]-'0' ;
					printf("\r\nlength: %d\r\n",len);	
					}
					
					skip++;
					
				}else if(skip>(len*2)+8+1){ //crc 1 byte
 					  skip=0;
					  
				}else{
						//delay(1000);
						buf[sent] = buf2[0]; // Fill buffer with data.
						skip++;
						sent++;
						
				}  
         
         
        if(feof(fp)) // Abort on EOF.
            break;
        
         
         
      
        
        if(sent >= 4096*2) { // 4k chunk read.
            for(xc=0;xc<sent;xc++){
			 buf2[0]=buf[xc];
						write(fd, buf2, 1); // Sent to bootloader.
						printf("1[%d]-[%s]",xc,buf2);
						usleep ((1 + 7) * 1);
				
				}
			printf("Chunks sent1: %i, last chunk: %i.\n", ++sentchunks, sent);
            sent = 0;
            while(c != 0x11) // Wait for response from bootloader.
                read(fd, &c, 1);
            
            printf("received %c",c);
			c = 0x00;
        }
    }
    if(sent >= 1) { // Send last chunk of data.
         for(xc=0;xc<=sent;xc++){
				buf2[0]=buf[xc];
				
				write(fd, buf2, 1); // Sent to bootloader.
				printf("2[%d]-[%s]",xc,buf2);
				usleep ((1 + 7) * 1000);
				//delay(1000);
				
				}write(fd, buf, sent);
        printf("Chunks sent2: %i, last chunk: %i.\n", ++sentchunks, sent);
    }
	close(fd);
    return 0;
}
