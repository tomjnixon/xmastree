#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 50
#define DATA_PIN 3

// Define the array of leds
CRGB leds[NUM_LEDS];

const unsigned long flash_time = 5*(1000/25);

void setup() { 
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    
    delay(3000);
    
    // White flash
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = CRGB::White;
    FastLED.show();
    delay(flash_time);
    
    // Black pause
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = CRGB::Black;
    FastLED.show();
    delay(flash_time);
    
    // each LED in turn
    for (int i = 0; i < NUM_LEDS; i++) {
        for (int j = 0; j < NUM_LEDS; j++)
            leds[j] = CRGB::Black;
        leds[i] = CRGB::White;
        FastLED.show();
        delay(flash_time);
    }
    
    // clear
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = CRGB::Black;
    FastLED.show();
}

void loop() { 
}
