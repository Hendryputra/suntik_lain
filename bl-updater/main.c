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

static unsigned char Ascii2Hex(unsigned char c)
{
    if (c >= '0' && c <= '9')
    {
        return (unsigned char)(c - '0');
    }

    if (c >= 'A' && c <= 'F')
    {
        return (unsigned char)(c - 'A' + 10);
    }

    if (c >= 'a' && c <= 'f')
    {
        return (unsigned char)(c - 'a' + 10);
    }

    printf("Wrong Hex-Nibble %c (%02X)\n", c, c);
    
    return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
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
      
        //tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ISIG;
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 0;           // 0.5 seconds read timeout

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
	//int fd;
	#if 1
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

#endif
	//unsigned long FL;
    FILE *fp;
    fp = fopen(argv[2], "rb"); // Open file to be sent.
    if(fp == NULL) {
        printf("Error opening file %s!\n", argv[2]);
        return -1;
    }
    
   // FL= lseek(fp, 0L, 2); 
    //    printf("size %ld!\n", FL);
    
    //return 0;
    
    unsigned char c;
    volatile long int last_len=0;
    unsigned char buf[1024*512*2];
    
    long int sent = 0, sentchunks = 0,xc=0,skip=0,len=0;
	unsigned char buf2[2];
	buf2[1]='\0';
	int rectype=99;
    while(1) {
       
         
        	buf2[0]=getc(fp);
				  
				if (buf2[0] == '\r' ){
					  continue;
				}
				
				if (buf2[0] == '\n' ){
					continue;
				}
				  
				if (buf2[0] != ':' && skip==0){
					 printf("Missing start of record (':') wrong byte %c / %02X\n",buf2[xc], buf2[xc]);
					 break;
				}else if(skip<=8){
					//:,1,0,0,0,0,0,0,0,  
					
					if(skip==1){
					len=Ascii2Hex(buf2[0])*16;
					//	printf("len H: %d/%c,",len,buf2[0]);
						skip++;
					}else if(skip==2){
						
						len=len+Ascii2Hex(buf2[0]);
				//		printf("baca file len: %d\r\n",len);
						printf("\r\nlengthnya: %ld\r\n",last_len);	
								
						last_len=len+last_len;	
					skip++;
					}else if(skip==7 || skip==8){
						
						if(skip==8){
							
							rectype=rectype+Ascii2Hex(buf2[0]);
							printf("type: %d,",rectype);
							if(rectype==1 ||rectype==3 ){
								printf("end of file");
								break;
							}
						}else{
							
							rectype=Ascii2Hex(buf2[0])*16;
								
						}
						
						
						
						skip++;
					}else{
						skip++;
					}
					
					
				}else if(skip==(len*2)+8+1){ //crc 1 byte
 					  
 					  skip++;
					  
				}else if(skip>=(len*2)+8+2){ //crc 1 byte
 					  
 					  skip=0;
					  
				}else{
						//delay(5000);
						buf[sent] = Ascii2Hex(buf2[0]); // Fill buffer with data.
						
					printf("skip : %ld,addr: %ld ,data: %c\r\n",skip,sent/2,buf2[0]);
					
						sent++;
						skip++;
				}  
         
         
        if(feof(fp)){ // Abort on EOF.
            break;
        
		}
         
      
      
    }
    int bc=0,i=0;
    unsigned long int sent2;
    sent2=sent/2;
    
     for(i = 0; i < 8; i++) {
		buf2[0]=(sent2 >> i*4) & 0x0f;
					write(fd, buf2, 1); // Sent to bootloader.
						//printf("1[%d]-[%s]",xc,buf2);
						while(c != buf2[0]){ // Wait for response from bootloader.
							read(fd, &c, 1);
						printf("%01X",c);
						}
						printf("rec: %01X",c);
						//usleep ((1 + 0) * 1000);
					
		printf("length sent : %ld\r\n",sent2);
	 }
	c='\0'; 
	 
	while(c != 0x12){ // Wait for response from bootloader.
		read(fd, &c, 1);
		printf("%01X",c);
	}
	printf("rec: %01X",c);
	usleep ((1 + 0) * 1000);
	 
	float x1,x2; 
	 
    while(sent>0){
		
		#if 1  
		
		if(sent>4096*2){
            for(xc=0;xc<(4096*2);xc++){
			 buf2[0]=buf[xc+(bc*4096*2)];
			  if(xc % 8==0){
				  x1=((sent2*2)-sent);
				  x2=(sent2*2);
				printf("\r\n%0.0f%%:%6ld",(x1/x2)*100,xc);
				}
			  printf("-[%01X]:",buf2[0]);
			 
					write(fd, buf2, 1); // Sent to bootloader.
						//printf("1[%d]-[%s]",xc,buf2);
						while(c != buf2[0]){ // Wait for response from bootloader.
							read(fd, &c, 1);
						//printf("%01X",c);
						}
						printf("%01X",c);
					//	usleep (1 + (0 * 10000));
				
			sent--;
			}
			
			
			
			
			c='\0';
           while(c != 0x12) // Wait for response from bootloader.
                read(fd, &c, 1);
            
            bc++;
            printf("\r\nreceived %c\r\n",c);
			c = '\0';
			printf("\r\nChunks sent1: %li, last chunk: %li.\n", ++sentchunks, sent);
           
					
		}else{
			//printf("\r\nlengthnya: %d\r\n",last_len);
			xc=0;
			//sent=40;
			//char cc[2];
			for(xc=0;xc<sent;xc++){
				 buf2[0] =buf[xc+(bc*4096*2)];
				
				 
				 if(xc % 8==0){
					x1=((sent2*2)-sent);
					x2=(sent2*2);
					printf("\r\n%0.0f%%:%6ld",(x1/x2)*100,xc);
				 }
				 
				 printf("-[%01X]:",buf2[0]);
				 write(fd, buf2, 1); // Sent to bootloader.
							
				//usleep (1 +(20*1000));				
				while(c != buf2[0]){ // Wait for response from bootloader.
					read(fd, &c, 1);
					
				}
				printf("%01X",c);
			
				//usleep (1 +(0*1000));
					
				
			}
			sent=0;
			
			printf("\r\nChunks sent1: %li, last chunk: %li.\n", ++sentchunks, sent);
            //sent = 0;
			c='\0';
			while(c != 0x12){ // Wait for response from bootloader.
                read(fd, &c, 1);
               // printf("%x",c);
                usleep ((1 + 0) * 1000);
			}
			
			printf("received END Char 0x12, Jump to User Code\r\n");
			
		}
			
			//sent=sent-
			
        #endif
	}
    
    if(sent >= 1) { // Send last chunk of data.
         for(xc=0;xc<=sent;xc++){
				buf2[0]=buf[xc];
				
				write(fd, buf2, 1); // Sent to bootloader.
				printf("2[%ld]-[%s]",xc,buf2);
				usleep ((1 + 7) * 1000);
				//delay(1000);
				
				}
				write(fd, buf, sent);
        printf("Chunks sent2: %li, last chunk: %li.\n", ++sentchunks, sent);
    }
	close(fd);
    return 0;
}
