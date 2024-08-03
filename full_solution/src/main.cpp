#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "core_pins.h"
#include <cstdint>

// teensy40 pins
#define INTERNAL_LED_PIN 13
#define PIXEL_PIN 14

#define ANTENNA_PIN 20
#include "DCF77.h"

// RGBW PIXEL MATRIX
const int N_ROWS = 8;
const int N_COLS = 8;
const int N_PIXELS = N_COLS * N_ROWS;
Adafruit_NeoPixel pixels(N_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);


DCF77 dcf77;



void set2d(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    pixels.setPixelColor(N_COLS * row + col, r, g, b);
}

void setup() {
    pinMode(ANTENNA_PIN, INPUT);
    dcf77.reset();

    pinMode(INTERNAL_LED_PIN, OUTPUT);
    pixels.begin();
    pixels.clear();

    Serial.begin(9600);
    while (!Serial && millis() < 15000 ) {
        delay(100);
    }
    Serial.println("Hello from Teensy Clock");
}


void loop() {
    int value = digitalRead(ANTENNA_PIN);
    digitalWrite(INTERNAL_LED_PIN, value);
    dcf77.loop(value == HIGH);

    /*Serial.print(millis());*/
    /*Serial.print(" ");*/
    /*Serial.println(value);*/
    delay(1);
}
