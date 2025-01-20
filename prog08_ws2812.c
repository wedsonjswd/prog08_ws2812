#include <stdio.h>
#include "pico/stdlib.h"
#include <hardware/pio.h>
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7

struct pixel_t {uint8_t G, R, B;};

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

void npInit(uint pin) 
{
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);

    if (sm < 0) 
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    for (uint i = 0; i < LED_COUNT; ++i) 
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) 
{
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear() 
{
    for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

void npWrite() 
{
    for (uint i = 0; i < LED_COUNT; ++i) 
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

int main() 
{
    stdio_init_all();
    npInit(LED_PIN);
    npClear();
    npWrite();

    while (true) 
    {
        for(int i = 0; i<25; i++) 
        {   
            npSetLED(i, 0, 0, 255);
            npWrite();
            sleep_ms(100);
            npClear();
        }    
     }
}