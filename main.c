#include "LPC23xx.h"
#include "type.h"
#include "ecdsa.h"
#include "sha2.h"
#include "target.h"
#include "display.h"
#include "string.h"

// CRP Configuration
#define CRP0 0x23456789 // No CRP
#define CRP1 0x12345678 // CRP level 1
#define CRP2 0x87654321 // CRP level 2
#define CRP3 0x43218765 // CRP level 3
__attribute__((section(".crp"))) const uint32_t crp = CRP0;

// IAP Configuration
#define IAP_LOCATION 0x7ffffff1
uint32_t iap_command[5];
uint32_t iap_result[3];
typedef void (*IAP)(uint32_t [],uint32_t []);
static const IAP iap_entry = (IAP)IAP_LOCATION;
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

// IAP Commands
#define IAP_PREPARE     50
#define IAP_COPY        51
#define IAP_ERASE       52
#define IAP_BLANK       53
#define IAP_READID      54
#define IAP_READVERSION 55
#define IAP_COMPARE     56
#define IAP_ISP         57

uint8_t codebuffer[0x1000];

static uint8_t hex2bin(const uint8_t *s)
{
    uint8_t ret=0;
    uint8_t i;
    for(i = 0; i < 2; i++) {
        char c = *s++;
        int n = 0;
        if('0' <= c && c <= '9')
            n = c - '0';
        else if('a' <= c && c <= 'f')
            n = 10 + c - 'a';
        else if('A' <= c && c <= 'F')
            n = 10 + c - 'A';
        ret = n + ret * 16;
    }
    return ret;
}

int main(void)
{
    uint32_t i;
    FIO2DIR = (3 << 3);
    display_init();
    if(FIO2PIN & (1 << 1)) { // S0 pressed, start update.
        // Configure UART0
        PCLKSEL0 = (PCLKSEL0 & ~(0x03 << 6)) | (0x01 << 6);
        PINSEL0 &= ~((0x3<<6) | (0x3<<4)); // For safety, clear necessary bits in PINSEL0
        PINSEL0 |= (1<<6) | (1<<4);        // Configure TXD and RXD
        U0LCR |= (1<<7);  // Enable access to divisor latches
        U0DLL = 22; // Configure the baud rate to 115200
        U0DLM = 0x00;
        U0FDR = ((13 << 4) | 3);
        U0LCR &= ~(1<<7); // Disable access to divisor latches
        U0LCR = 0x03; // 8 bits, no parity, 1 stop bit
        U0FCR = 0x07; //Enable FIFO

        FIO2SET = (1 << 4); // Indicate start of boot loader.

        uint32_t ctr = 0;
        uint32_t codelen = 0;
        uint8_t buf[2];
        for(i = 0; i < 4; i++) {
            while(!(U0LSR & 0x01)) ; // Wait for incoming data.
            buf[0] = U0RBR;
            while(!(U0LSR & 0x01)) ;
            buf[1] = U0RBR;
            codelen = (codelen << 8) | hex2bin(buf);
            codebuffer[ctr] = codelen & 0xFF;
            ctr++;
        }

        uint32_t writeptr = 0x10000;

        while(codelen--) {
            while(!(U0LSR & 0x01)) ; // Wait for incoming data.
            buf[0] = U0RBR;
            while(!(U0LSR & 0x01)) ;
            buf[1] = U0RBR;
            codebuffer[ctr++] = hex2bin(buf);
            if(ctr == 0x1000 || codelen == 0) { // Transmission complete, write sector.
                if(writeptr == 0x10000) { // Before the first write, we have to erase the flash memory.
                    uint8_t startsector = 0, endsector = 0;

                    while(writeptr > sector_boundaries[startsector])
                        startsector++;

                    while(writeptr + codelen > sector_boundaries[endsector])
                        endsector++;

                    iap_command[0] = IAP_PREPARE;
                    iap_command[1] = startsector;
                    iap_command[2] = endsector;
                    iap_entry(iap_command, iap_result);

                    iap_command[0] = IAP_ERASE;
                    iap_command[1] = startsector;
                    iap_command[2] = endsector;
                    iap_command[3] = Fcclk / 1000UL;
                    iap_entry(iap_command, iap_result);
                }
                uint8_t sector = 0;
                while(writeptr > sector_boundaries[sector])
                    sector++; // Calculate sector to write to.

                iap_command[0] = IAP_PREPARE;
                iap_command[1] = sector;
                iap_command[2] = sector;
                iap_entry(iap_command, iap_result); // Prepare the sector(s).

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
                iap_command[0] = IAP_COPY; // Copy RAM to flash.
                iap_command[1] = writeptr; // Destination in flash.
                iap_command[2] = (uint32_t)codebuffer; // Source in RAM.
                iap_command[3] = ctr; // Size in bytes.
                iap_command[4] = Fcclk / 1000UL; // System clock in kHz.
                iap_entry(iap_command, iap_result); // Copy RAM to flash.

                writeptr += ctr; // Increase destination address for next write cycle.
                ctr = 0;
                if(codelen > 0) {
                    while(!(U0LSR & (1<<5)));
                    U0THR = 0x11; // Tell host to continue sending.
                }
            }
        }
        FIO2CLR = (1 << 4); // Indicate end of boot loader.
    }

    uint8_t *usercode_len_loc = (uint8_t*)0x00010000; // Location of length of usercode in flash.
    uint8_t *flashsig = (uint8_t*)0x00010004; // Location of the signature in flash.
    uint8_t *usercode = (uint8_t*)0x00010044; // Location of the usercode in flash.
    uint32_t usercode_len = (usercode_len_loc[0] << 24 | usercode_len_loc[1] << 16 | usercode_len_loc[2] << 8 | usercode_len_loc[3]) - 0x40; // Subtract the length of the signature.

    const uint8_t key[65] = {0x04, 0x6E, 0xAF, 0x09, 0x68, 0xAA, 0x89, 0x5A, 0xDD, 0xFE, 0xE5, 0x99, 0x56, 0x6F, 0x0B, 0x88,
                             0x02, 0x42, 0x46, 0x1D, 0x13, 0x77, 0xF4, 0x88, 0x7C, 0x9B, 0x84, 0x63, 0x1E, 0x13, 0x06, 0x7B,
                             0x96, 0xDB, 0x18, 0xC4, 0x1E, 0x0C, 0x20, 0x8F, 0x8D, 0x12, 0xEB, 0xCC, 0x3F, 0x99, 0xF2, 0x52,
                             0x29, 0x03, 0xAF, 0x61, 0x05, 0x83, 0x3E, 0x4C, 0xBA, 0xDE, 0x9D, 0x6A, 0x1D, 0x0F, 0x03, 0x91,
                             0x87}; // 1DVqmTy9Ux3NqkWURZZfygqvnxac2FjH66, http://gobittest.appspot.com/Address

    if(usercode_len != 0 && usercode_len < 0x7dfff) { // Check for invalid code length.
        if(ecdsa_verify(key, flashsig, usercode, usercode_len) == 0) { // Verification successfull, branch to user code.
            asm volatile("mov r0, #0x00010000"); // Start of sector 9.
            asm volatile("add r1, r0, #0x44"); // Skip length and 64 byte signature.
            asm volatile("bx r1"); // Branch to user program.
        }
    }

    while(1) { // Loop forever if verification failed.
        FIO2PIN ^= (1 << 3);
        for(i=0;i<1000000UL;i++)
            asm volatile("nop");
    }


    return 0;
}
