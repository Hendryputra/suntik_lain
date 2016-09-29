#include "LPC23xx.h"
#include "type.h"
#include "ecdsa.h"
#include "sha2.h"
#include "target.h"
#include "display.h"
#include "string.h"
#include <stdio.h>
// CRP Configuration
#define CRP0 0x23456789 // No CRP
#define CRP1 0x12345678 // CRP level 1
#define CRP2 0x87654321 // CRP level 2
#define CRP3 0x43218765 // CRP level 3

#ifndef BIT
#define BIT(x)	(1 << (x))

#endif

// IAP Commands
#define IAP_PREPARE     50
#define IAP_COPY        51
#define IAP_ERASE       52
#define IAP_BLANK       53
#define IAP_READID      54
#define IAP_READVERSION 55
#define IAP_COMPARE     56
#define IAP_ISP         57

/* Constants to setup and access the UART. */
#define serDLAB							( ( unsigned char ) 0x80 )
#define serENABLE_INTERRUPTS			( ( unsigned char ) 0x03 )
#define serNO_PARITY					( ( unsigned char ) 0x00 )
#define ser1_STOP_BIT					( ( unsigned char ) 0x00 )
#define ser8_BIT_CHARS					( ( unsigned char ) 0x03 )
#define serFIFO_ON						( ( unsigned char ) 0x01 )
#define serCLEAR_FIFO					( ( unsigned char ) 0x06 )
#define serWANTED_CLOCK_SCALING			( ( unsigned long ) 16 )

#define LED_UTAMA	BIT(18)
#define RLY_1	BIT(0)			/* P1 */
#define RLY_2	BIT(1)
#define RLY_3	BIT(4)			
#define RLY_4	BIT(8)	
#define cRelay1()		FIO1CLR = RLY_1
#define cRelay2()		FIO1CLR = RLY_2
#define cRelay3()		FIO1CLR = RLY_3
#define cRelay4()		FIO1CLR = RLY_4
#define sRelay1()		FIO1SET = RLY_1
#define sRelay2()		FIO1SET = RLY_2
#define sRelay3()		FIO1SET = RLY_3
#define sRelay4()		FIO1SET = RLY_4

__attribute__((section(".crp"))) const uint32_t crp = CRP0;


typedef struct	{
  unsigned int ReturnCode;
  unsigned int Result[4];
} IAP_return_t;

typedef enum IAP_STATUS_t {
   CMD_SUCCESS = 0,         // Command was executed successfully.
   INVALID_COMMAND = 1,     // Invalid command.
   SRC_ADDR_ERROR = 2,      // Source address is not on a word boundary.
   DST_ADDR_ERROR = 3,      // Destination address is not on a correct boundary.

   SRC_ADDR_NOT_MAPPED = 4, // Source address is not mapped in the memory map.
                            // Count value is taken in to consideration where
                            // applicable.

   DST_ADDR_NOT_MAPPED = 5, // Destination address is not mapped in the memory
                            // map. Count value is taken in to consideration
                            // where applicable.

   COUNT_ERROR = 6,         // Byte count is not multiple of 4 or is not a
                            // permitted value.

   INVALID_SECTOR = 7,      // Sector number is invalid.
   SECTOR_NOT_BLANK = 8,    // Sector is not blank.

   SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION = 9, // Command to prepare sector for
                                                // write operation was not
                                                // executed.

   COMPARE_ERROR = 10,      // Source and destination data is not same.
   BUSY = 11,               // Flash programming hardware interface is busy.
} IAP_STATUS_t;

// IAP Configuration
#define IAP_LOCATION 0x7ffffff1
unsigned long iap_command[5];
unsigned long iap_result[3];
uint32_t ctr = 0;
uint32_t codelen = 0;
uint8_t buf[2];


void iap_entry(unsigned int param_tab[], unsigned int result_tab[])		{
	IAP_return_t iap_return;
	void (*iap)(unsigned int[], unsigned int[]);
	
	iap = (void (*)(unsigned int[], unsigned int[]))IAP_LOCATION;
	iap(param_tab,result_tab);
	
}

IAP_return_t iapReadSerialNumber(void)	{
	IAP_return_t iap_return;
	// ToDo: Why does IAP sometime cause the application to halt when read???
	iap_command[0] = IAP_READID;

	iap_entry(iap_command,(unsigned int*)(&iap_return));
	
	return iap_return;
}

IAP_return_t iapEraseSector(unsigned char awal, unsigned char akhir)	{
	IAP_return_t iap_return;
	// ToDo: Why does IAP sometime cause the application to halt when read???
	iap_command[0] = IAP_ERASE;
	iap_command[1] = awal;
	iap_command[2] = akhir;
	iap_command[3] = (60000000/1000);
	
	iap_entry(iap_command,(unsigned int*)(&iap_return));
	
	return iap_return;
}

IAP_return_t iapPrepareSector(unsigned char awal, unsigned char akhir)		{
	IAP_return_t iap_return;
	// ToDo: Why does IAP sometime cause the application to halt when read???
	iap_command[0] = IAP_PREPARE;
	iap_command[1] = awal;
	iap_command[2] = akhir;
	
	iap_entry(iap_command,(unsigned int*)(&iap_return));
	
	return iap_return;
}


IAP_return_t iapCopyMemorySector(unsigned int addr, unsigned short * data, int pjg)		{
	IAP_return_t iap_return;
	// ToDo: Why does IAP sometime cause the application to halt when read???
	
	//printf("\r\n%s cmd: %d, addr: 0x%08X, pjg: %d\r\n", __FUNCTION__, IAP_CMD_COPYRAMTOFLASH, addr, pjg);
	
	iap_command[0] = IAP_COPY;
	iap_command[1] = addr;
	iap_command[2] = (unsigned short*) &data[0];
	iap_command[3] = pjg;
	iap_command[4] = (60000000/1000);
	
	iap_entry(iap_command,(unsigned int*)(&iap_return));
	
	return iap_return;
}

void ser0_print(const signed char * const str){
	signed char *pxNext;
	pxNext = ( signed char * ) str;
			/* We wrote the character directly to the UART, so was 
			successful. */
			//*plTHREEmpty = pdFALSE;
			while( *pxNext ){
				while(!(U0LSR & (1<<5)));
                 U0THR= *pxNext	;
				pxNext++;
			}
			//xReturn = pdPASS;
	
}

struct t_str{
	unsigned long int data;
};
const uint32_t sector_boundaries[] = {0x00000FFF, // Sector 0
                                      0x00001FFF, // Sector 1
                                      0x00002FFF, // Sector 2
                                      0x00003FFF, // Sector 3
                                      0x00004FFF, // Sector 4
                                      0x00005FFF, // Sector 5
                                      0x00006FFF, // Sector 6
                                      0x00007FFF, // Sector 7
                                      0x0000FFFF, // Sector 8
                                      0x00017FFF, // Sector 9
                                      0x0001FFFF, // Sector 10
                                      0x00027FFF, // Sector 11
                                      0x0002FFFF, // Sector 12
                                      0x00037FFF, // Sector 13
                                      0x0003FFFF, // Sector 14
                                      0x00047FFF, // Sector 15
                                      0x0004FFFF, // Sector 16
                                      0x00057FFF, // Sector 17
                                      0x0005FFFF, // Sector 18
                                      0x00067FFF, // Sector 19
                                      0x0006FFFF, // Sector 20
                                      0x00077FFF, // Sector 21
                                      0x00078FFF, // Sector 22
                                      0x00079FFF, // Sector 23
                                      0x0007AFFF, // Sector 24
                                      0x0007BFFF, // Sector 25
                                      0x0007CFFF, // Sector 26
                                      0x0007DFFF};  // Sector 27

// Usercode locations
#define USERCODE_LOCATION   0x10000
#define USERCODE_SIZE_SIZE  0x04
#define USERCODE_SIG_SIZE   0x40
#define USERCODE_FREE_SPACE 0x7dfff
IAP_return_t iap_return;
uint8_t codebuffer[0x1000];
  int dd;
        char buff[20];
int main(void)
{
	
    uint32_t i;
  //  FIO2DIR = (3 << 3);
    FIO2DIR &= ~(BIT(10));
    FIO1DIR |= LED_UTAMA;
    FIO1DIR |= RLY_1;
    FIO1DIR |= RLY_2;
    FIO1DIR |= RLY_4;
    FIO1DIR |= RLY_3;
    
	FIO1CLR = LED_UTAMA;
	FIO1SET = RLY_4;
	FIO1CLR = RLY_3;
	FIO1CLR = RLY_2;
   // display_init();
    if(FIO2PIN & BIT(10)) { // S0 pressed, start update.
        // Configure UART0
        
		unsigned long ulDivisor, ulWantedClock;
        PCONP |= BIT(3);
        PCLKSEL0 = (PCLKSEL0 & ~(0x03 << 6)) | (0x01 << 6);
		PINSEL0 &= ~(BIT(5)); // For safety, clear necessary bits in PINSEL0
		PINSEL0 &= ~(BIT(7)); // For safety, clear necessary bits in PINSEL0
        PINSEL0 |= BIT(4);        // Configure TXD and RXD
        PINSEL0 |= BIT(6);        // Configure TXD and RXD
        
        ulWantedClock = 115200 * serWANTED_CLOCK_SCALING;
		ulDivisor = 60000000 / ulWantedClock;

		/* Set the DLAB bit so we can access the divisor. */
		U0LCR |= serDLAB;

		/* Setup the divisor. */
		U0DLL = ( unsigned char ) ( ulDivisor & ( unsigned long ) 0xff );
		ulDivisor >>= 8;
		U0DLM = ( unsigned char ) ( ulDivisor & ( unsigned long ) 0xff );

		/* Turn on the FIFO's and clear the buffers. */
		U0FCR = ( serFIFO_ON | serCLEAR_FIFO );

		/* Setup transmission format. */
		U0LCR = serNO_PARITY | ser1_STOP_BIT | ser8_BIT_CHARS;

		
        
       /* for(i = 0; i < 4; i++) {
            while(!(U0LSR & 0x01)) ; // Wait for incoming data.
            buf[0] = U0RBR;
            while(!(U0LSR & 0x01)) ;
            buf[1] = U0RBR;
            codelen = (codelen << 8) | hex2bin(buf);
            codebuffer[ctr] = codelen & 0xFF;
            ctr++;
        }*/
		codelen=50000;
        uint32_t writeptr = USERCODE_LOCATION;

        while(codelen--) {
			FIO1PIN ^= RLY_3;
			
            while(!(U0LSR & 0x01)) ; // Wait for incoming data.
            buf[0] = U0RBR;
            //U0THR=buf[0];
            codebuffer[ctr++] = buf[0];
            while(!(U0LSR & 0x01)) ;
            buf[1] = U0RBR;
            codebuffer[ctr++] = buf[1];
            //U0THR=buf[1];
            //ser0_print(buf);
            
            
            
            
            if(ctr == 0x1000 || codelen == 0) { // Transmission complete, write sector.
				//while(1) { // Loop forever if verification failed.
				//FIO1PIN ^= RLY_3;
				//for(i=0;i<1000000UL;i++)
				//asm volatile("nop");
				//}
				
               
                if(writeptr == USERCODE_LOCATION) { // Before the first write, we have to erase the flash memory.
                    uint8_t startsector = 0, endsector = 0;

                    while(writeptr > sector_boundaries[startsector])
                        startsector++;

                    while(writeptr + codelen > sector_boundaries[endsector])
                        endsector++;
					
					
					iap_return=iapPrepareSector(startsector,endsector);
					
					sprintf(buff,"result:%d\r\n",iap_return.ReturnCode);
					ser0_print(buff);
					iap_return=iapEraseSector(startsector,endsector);
					                    
					sprintf(buff,"result:%d\r\n",iap_return.ReturnCode);
					ser0_print(buff);
                }
                uint8_t sector = 0;
                while(writeptr > sector_boundaries[sector])
                    sector++; // Calculate sector to write to.

				iap_return=iapPrepareSector(sector,sector);
					
				sprintf(buff,"result:%d\r\n",iap_return.ReturnCode);
				ser0_print(buff);
				
					 if(ctr != 0x1000) { // Adjust ctr for last transmitted bytes. ctr has to be 256 | 512 | 1024 | 4096.
                    if(ctr <= 256)
                        ctr = 256;
                    else if(ctr <= 512)
                        ctr = 512;
                    else if(ctr <= 1024)
                        ctr = 1024;
                    else
                        ctr = 4096;
                }
                
                for(i=0;i<100;i++){
                sprintf(buff,"%c\r\n",codebuffer[i]);
				ser0_print(buff);
				}
                
                iap_return=iapCopyMemorySector(writeptr,codebuffer,ctr);
					
				sprintf(buff,"result:%d\r\n",iap_return.ReturnCode);
				ser0_print(buff);
				
                
					
                writeptr += ctr; // Increase destination address for next write cycle.
                ctr = 0;
                if(codelen > 0) {
                    while(!(U0LSR & (1<<5)));
                    U0THR = 0x11; // Tell host to continue sending.
                    FIO1PIN ^= RLY_2; 
                }
            }
        }
        //FIO2CLR = (1 << 4); // Indicate end of boot loader.
        cRelay1();
        #if 1
        struct t_str *st_str;
		st_str =  0x00010000;
		int cc;
		for(dd=0;dd<50;dd++){
			sprintf(buff,"%d:%X \r\n",dd,st_str[dd].data);
				ser0_print(buff);
			//U0THR=99;
			
			
		}
		/*
		while(1){
		 FIO1PIN ^= LED_UTAMA;
		 for(i=0;i<1000000UL;i++)
            asm volatile("nop");
		} 
		* */
		#endif
        
    }

    uint8_t *usercode_len_loc = (uint8_t*)USERCODE_LOCATION; // Location of length of usercode in flash.
    uint8_t *flashsig = (uint8_t*)USERCODE_LOCATION + USERCODE_SIZE_SIZE; // Location of the signature in flash.
    uint8_t *usercode = (uint8_t*)USERCODE_LOCATION + USERCODE_SIZE_SIZE ; // Location of the usercode in flash.
    uint32_t usercode_len = (usercode_len_loc[0] << 24 | usercode_len_loc[1] << 16 | usercode_len_loc[2] << 8 | usercode_len_loc[3]) ; // Subtract the length of the signature.

    const uint8_t key[65] = { 0xAD, 0xAA, 0x89, 0x5A, 0xDD, 0xFE, 0xE5, 0x99, 0x56, 0x6F, 0x0B, 0x88,
                             0x02, 0x42, 0x46, 0x1D, 0x13, 0x77, 0xF4, 0x88, 0x7C, 0x9B, 0x84, 0x63, 0x1E, 0x13, 0x06, 0x7B,
                             0x96, 0xDB, 0x18, 0xC4, 0x1E, 0x0C, 0x20, 0x8F, 0x8D, 0x12, 0xEB, 0xCC, 0x3F, 0x99, 0xF2, 0x52,
                             0x29, 0x03, 0xAF, 0x61, 0x05, 0x83, 0x3E, 0x4C, 0xBA, 0xDE, 0x9D, 0x6A, 0x1D, 0x0F, 0x03, 0x91,
                             0x87}; // 1DVqmTy9Ux3NqkWURZZfygqvnxac2FjH66, http://gobittest.appspot.com/Address

    if(usercode_len != 0 && usercode_len < USERCODE_FREE_SPACE) { // Check for invalid code length.
		sRelay2();
      // if(ecdsa_verify(key, flashsig, usercode, usercode_len) == 0) { // Verification successfull, branch to user codeasassasa.
            asm volatile("mov r0, #0x00010000"); // Start of sector 9.
        //    asm volatile("add r1, r0, #0x44"); // Skip length and 64 byte signature.
            asm volatile("bx r0"); // Branch to user program.
     //   }
    }

    while(1) { // Loop forever if verification failed.
        FIO1PIN ^= LED_UTAMA;
        for(i=0;i<1000000UL;i++)
            asm volatile("nop");
    }


    return 0;
}
