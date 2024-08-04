#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "TimeLib.h"
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


uint32_t last_updated = 0;


DCF77 dcf77;


void set2d(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    pixels.setPixelColor(N_COLS * row + col, r, g, b);
}

void displayTime(uint32_t t) {
    if ((t - last_updated) < 250) {
        return;
    }
    last_updated = t;

    TimeElements tm;
    breakTime(now(), tm);
    pixels.clear();

    const int year_bits = 6;
    // year is stored offset from 1970, we want the last two digits of 20yy.
    const int y = tm.Year - 30;
    for (int pixel=0; pixel < year_bits; pixel++) {
        uint8_t power = y & (1 << pixel) ? 20 : 1;
        set2d(0, year_bits - pixel - 1, power, 0, 0);
    }

    const int month_bits = 4;
    for (int pixel=0; pixel < month_bits; pixel++) {
        uint8_t power = tm.Month & (1 << pixel) ? 20 : 1;
        set2d(1, month_bits - pixel + 1, 0, power, power);
    }

    const int day_bits = 5;
    for (int pixel=0; pixel < day_bits; pixel++) {
        uint8_t power = tm.Day & (1 << pixel) ? 20 : 1;
        set2d(2, day_bits - pixel, power, power, 0);
    }

    const int hour_bits = 5;
    for (int pixel=0; pixel < hour_bits; pixel++) {
        uint8_t power = tm.Hour & (1 << pixel) ? 20 : 1;
        set2d(5, hour_bits - pixel, 0, power, 0);
    }

    const int minute_bits = 6;
    for (int pixel=0; pixel < minute_bits; pixel++) {
        uint8_t power = tm.Minute & (1 << pixel) ? 20 : 1;
        set2d(6, minute_bits - pixel - 1, 0, 0, power);
    }

    const int second_bits = 6;
    for (int pixel=0; pixel < second_bits; pixel++) {
        uint8_t power = tm.Second & (1 << pixel) ? 20 : 1;
        set2d(7, second_bits - pixel - 1,  power, 0, power);
    }

    pixels.show();
}

void displayProgress(uint32_t t) {
    if ((t - last_updated) < 100) {
        return;
    }
    last_updated = t;

    if (dcf77.current_bit == 1) {
        pixels.clear();
    }

    pixels.setPixelColor(dcf77.current_bit - 1, 10, 0, 0);
    pixels.show();
}

void setup() {
    pinMode(ANTENNA_PIN, INPUT);
    dcf77.reset();

    pinMode(INTERNAL_LED_PIN, OUTPUT);
    pixels.begin();
    pixels.clear();
    pixels.show();

    Serial.begin(9600);
    while (!Serial && millis() < 10000 ) {
        delay(100);
    }
    Serial.println("Hello from Teensy Clock");
}

void loop() {
    auto t = millis();
    int value = digitalRead(ANTENNA_PIN);

    digitalWrite(INTERNAL_LED_PIN, value);
    bool new_second = dcf77.loop(t, value == HIGH);

    if (!dcf77.valid && new_second) {
        displayProgress(t);
    }

    if (dcf77.valid) {
        displayTime(t);
    }
}
