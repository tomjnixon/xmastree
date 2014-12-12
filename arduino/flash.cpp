#include <stdint.h>
#include "FastLED.h"
#include "leds.h"

// How many leds in your strip?
#define NUM_LEDS 50
#define DATA_PIN 3

uint8_t brightness = 255;

CRGB leds[NUM_LEDS];

void update_brightness() {
    brightness = constrain(map(analogRead(A0), 50, 1000, 0, 255), 0, 255);
}

void random_wipe(CRGB from, CRGB to, unsigned long delay_time) {
    update_brightness();
    uint8_t axis = random(4);
    int8_t direction = random(0, 2) == 1 ? 1 : -1;
    
    for (int a = -128; a < 128; a++) {
        for (int i = 0; i < NUM_LEDS; i++) {
            int8_t led_pos = direction * led_positions[i].axes[axis];
            leds[i] = led_pos <= a ? to : from;
        }
        FastLED.show(brightness);
        delay(delay_time);
    }
}

void setup() {
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
}

void loop() {
    random_wipe(CRGB::Red, CRGB::Green, 0);
    random_wipe(CRGB::Green, CRGB::Blue, 0);
    random_wipe(CRGB::Blue, CRGB::Red, 0);
}
