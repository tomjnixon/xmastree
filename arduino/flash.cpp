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

int16_t dist_upper_bound(const int8_t *a, const int8_t *b) {
    return abs((int16_t)a[0] - (int16_t)b[0]) +
           abs((int16_t)a[1] - (int16_t)b[1]) +
           abs((int16_t)a[2] - (int16_t)b[2]);
}

int16_t dist_lower_bound(const int8_t *a, const int8_t *b) {
    return max(max(abs((int16_t)a[0] - (int16_t)b[0]),
                   abs((int16_t)a[1] - (int16_t)b[1])), 
               abs((int16_t)a[2] - (int16_t)b[2]));
}

void cube_at(const int8_t *p, int16_t size, CRGB fg, CRGB bg) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = dist_lower_bound(p, led_positions[i].axes) < size
                  ?fg
                  :bg;
    }
    FastLED.show(brightness);
}

void point_at(const int8_t *p) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        int16_t dist = dist_lower_bound(p, led_positions[i].axes);
        int16_t dist_norm = (dist << 2) + dist;
        if (dist_norm >= 255)
            leds[i] = CRGB(0, 0, 0);
        else {
            uint8_t brightness = 255 - dist_norm;
            leds[i] = CRGB(brightness, brightness, brightness);
        }
    }
    FastLED.show(brightness);
}

int16_t dist(const int8_t *a, const int8_t *b) {
    int32_t dist = 0;
    dist += ((int32_t)a[0] - (int32_t)b[0]) * ((int32_t)a[0] - (int32_t)b[0]);
    dist += ((int32_t)a[1] - (int32_t)b[1]) * ((int32_t)a[1] - (int32_t)b[1]);
    dist += ((int32_t)a[2] - (int32_t)b[2]) * ((int32_t)a[2] - (int32_t)b[2]);
    return sqrt(dist);
}

void pos_between(const int8_t *a, const int8_t *b, int16_t pos, int16_t max_pos, int8_t *out) {
    for (uint8_t i = 0; i < 3; i++)
        out[i] = map(pos, 0, max_pos, a[i], b[i]);
}

void cube_anim() {
    uint8_t from;
    uint8_t to = random(NUM_LEDS);
    
    while (true) {
        from = to;
        to = random(NUM_LEDS);
        
        uint16_t distance = dist(led_positions[from].axes, led_positions[to].axes) * 2;
        for (uint16_t step = 0; step < distance; step++) {
            int8_t pos[3];
            pos_between(led_positions[from].axes,
                        led_positions[to].axes,
                        step, distance, pos);
            // cube_at(pos, 35, CRGB::White, CRGB::Black);
            point_at(pos);
        }
        update_brightness();
    }
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
    // random_wipe(CRGB::Red, CRGB::Green, 0);
    // random_wipe(CRGB::Green, CRGB::Blue, 0);
    // random_wipe(CRGB::Blue, CRGB::Red, 0);
    cube_anim();
}
