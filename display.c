#include "LPC23xx.h"
#include "type.h"
#include "string.h"

#include "display.h"

/* Framebuffer */
uint8_t frame[FRAMESIZE];

static void waste_time(uint32_t i)
{
    while(i--)
        asm volatile("nop");
}

static void send_command(unsigned char comm)
{
    FIO1CLR = (1 << PIN_ID);
    FIO1SET = (1 << PIN_R);
    FIO1CLR = (1 << PIN_CS);
    FIO1CLR = (1 << PIN_W);
    waste_time(100);
    FIO1PIN3 = comm;
    waste_time(100);
    FIO1SET = (1 << PIN_W);
    waste_time(100);
    FIO1SET = (1 << PIN_CS);
    waste_time(100);
}

static void send_data(unsigned char data)
{
    FIO1PIN3 = data;

    FIO1SET = (1 << PIN_ID);

    FIO1SET = (1 << PIN_R);
    FIO1CLR = (1 << PIN_CS);
    FIO1CLR = (1 << PIN_W);

    FIO1SET = (1 << PIN_W);
    FIO1SET = (1 << PIN_CS);
}

static void display_brightness(uint8_t b)
{
    if(b > 100)
        b = 100;

    PWM1MR1 = b;
    PWM1LER = 0x02;     // Apply the new MR1 value
}

static void display_update(void)
{
    uint16_t i;
    uint8_t data;

    send_command(0x2C);
    for(i = 0; i < FRAMESIZE; i++) {
        data = (frame[i] & 0x80) >> 3;
        data |= (frame[i] & 0x40) << 1;
        data |= data >> 1;
        send_data(data);

        data = (frame[i] & 0x20) >> 1;
        data |= (frame[i] & 0x10) << 3;
        data |= data >> 1;
        send_data(data);

        data = (frame[i] & 0x08) << 1;
        data |= (frame[i] & 0x04) << 5;
        data |= data >> 1;
        send_data(data);

        data = (frame[i] & 0x02) << 3;
        data |= (frame[i] & 0x01) << 7;
        data |= data >> 1;
        send_data(data);
    }
}

static void display_decompress(uint8_t *in)
{
    register uint8_t *input = in;
    uint32_t written = 0, i = 0;
    while(written < FRAMESIZE) {
        if(*(input) & 0x80) {   // Sequence of unique bytes
            i = ( (*(input++) & ~0x80) << 8);
            i |= *(input++);
            while(i--)
                frame[written++] = *(input++);
        }
        else {  // Sequence of equal bytes
            i = (*(input++) << 8);
            i |= *(input++);
            while(i--)
                frame[written++] = *input;
            input++;
        }
    }
    return;
}

extern uint8_t frame_logo[];
void display_init(void)
{
    SCS |= 0x00000001;  // Enable FIO on Port 1
    FIO1DIR |= (1 << PIN_RESET) | (1 << PIN_ID) | (1 << PIN_W) | (1 << PIN_R) | (1 << PIN_CS) | (0xFF << PORT_DATA);

    FIO1SET = (1 << PIN_RESET);     // Reset the display
    waste_time(100000UL);
    FIO1CLR = (1 << PIN_RESET);
    waste_time(100000UL);
    FIO1SET = (1 << PIN_RESET);
    waste_time(1000000UL);

    memset(frame, 0x00, FRAMESIZE);

    send_command(0x11); // Sleep out
    send_command(0x28); // Display off
    waste_time(100000UL);
    send_command(0xB3); // FOSC divider
    send_data(0x00);
    send_command(0xC0); // Vop = 0xB9
    send_data(0x30);
    send_data(0x01);
    send_command(0xC3); // BIAS = 1/14
    send_data(0x00);// 0x00
    send_command(0xC4); // Booster = x8
    send_data(0x07);
    send_command(0xD0); // Enable analog circuit
    send_data(0x1D);
    send_command(0xB5); // N-Line = 0
    send_data(0x00);
    send_command(0x39); // Monochrome mode
    send_command(0x3A); // Enable DDRAM interface
    send_data(0x02);
    send_command(0x36); // Scan direction
    send_data(0xC0);    // C160-C1 SEG384-SEG1
    send_command(0xB0); // Duty setting
    send_data(0x9F);

    send_command(0x20); // Inversion off

    send_command(0xB1); // First output COM
    send_data(0x00);
    send_command(0x2A); // Column address settings
    send_data(0x00);
    send_data(0x08);    // 0x08
    send_data(0x00);
    send_data(0x7F); // 7F
    send_command(0x2B); // Row address settings
    send_data(0x00);
    send_data(0x00);
    send_data(0x00);
    send_data(0x9F);


    send_command(0x29); // Display on

    PINSEL4 |= 0x01;    // PWM on pin 2.0
    PWM1PR = 125;       // Prescaler
    PWM1MR0 = 100;      // Reset counter at 100
    PWM1MR1 = 100;      // Turn off backlight
    PWM1MCR = 0x02;     // Reset TC on match of MR0
    PWM1PCR = (1 << 9); // Enable PWM1.1
    PWM1TCR = 0x09;     // Enable timer and PWM

    display_brightness(50);

    display_decompress(frame_logo);

    display_update();
}


