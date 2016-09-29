#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include "LPC23xx.h"
#include "type.h"
#include "target.h"

#define PIN_RESET 19
#define PIN_ID 20
#define PIN_W 21
#define PIN_R 22
#define PIN_CS 23
#define PORT_DATA 24

#define LED_DISPLAY (1<<0)

#define FRAMESIZE   4800    // 240 * 160 bits


void display_init(void);
void display_clear(void);


#endif // DISPLAY_H_INCLUDED
