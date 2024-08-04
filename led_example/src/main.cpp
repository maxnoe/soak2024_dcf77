#include <cstdint>
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"


#define PIXEL_PIN 14

// RGBW PIXEL MATRIX
const int N_ROWS = 8;
const int N_COLS = 8;
const int N_PIXELS = N_COLS * N_ROWS;
Adafruit_NeoPixel pixels(N_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

uint32_t last_update = 0;
uint32_t update_interval = 10;

// using a power of 2 makes % much more efficient
const uint32_t period = 4096;
// divisions are costly. Compute inverse once.
const float freq = 1.0f / period; 

void setup() {
    pixels.begin();
    pixels.clear();
    pixels.show();

    Serial.begin(9600);

    // wait 10 s for a serial connection from the pc
    // if not, the pogram will start anyway
    while (!Serial && millis() < 10000 ) {
        delay(100);
    }

    Serial.println("Hello from Teensy LED example");
}

void rainbow(uint32_t t) {
    // do nothing if we just updated
    if ((t - last_update) < update_interval) return;

    // number [0.0, 1.0) inside our desired period
    float phase = freq * (t % period);

    // HSV is the simplest method to go through the rainbow
    // we move through the hue over time with offsets
    // both per row and over time
    uint16_t hue_offset = 65536 * phase;

    for (int row=0; row < N_ROWS; row++) {

        // we show a third of the rainbow on the display at any given time
        // (8 rows, all hue distributed over 24 rows)
        uint16_t hue = hue_offset + 65536 * (row / 24.0);

        uint32_t color = pixels.ColorHSV(hue, 255, 20);
        for (int col=0; col < N_COLS; col++) {
            pixels.setPixelColor(N_COLS * row + col, color);
        }
    }
    pixels.show();

    last_update = t;
}


void loop() {
    uint32_t t = millis();
    rainbow(t);
    delay(5);
}
