#include <Arduino.h>
#include "TimeLib.h"
#include <cstdint>

#undef B1
#include "fmt/core.h"


#ifndef ANTENNA_PIN
#define ANTENNA_PIN 20
#endif

const uint64_t ONE = 1ull;

int get_value(uint64_t data, uint8_t first_bit, uint8_t n_bits) {
    return (data >> first_bit) & ((ONE << n_bits) - ONE);
}

bool get_bit(uint64_t data, uint8_t pos) {
    return data & (ONE << pos);
}

bool check_parity(uint64_t data, uint8_t first_bit, uint8_t n_bits) {
    int n_on = 0;
    for (uint8_t bit = first_bit; bit < (first_bit + n_bits); bit++) {
        n_on += get_bit(data, bit);
    }
    return n_on % 2 == 0;
}

// dcf77 submits up to 60, so we can use a single 64 bit integer
struct DCF77 {
    bool valid = false;
    uint64_t data = 0;
    int current_bit = 0;

    uint32_t current_pulse_begin = 0;
    uint32_t previous_pulse_begin = 0;
    uint32_t first_pulse_begin = 0;

    bool state = false;

    void reset() {
        data = 0;
        current_bit = 0;
        current_pulse_begin = 0;
        first_pulse_begin = 0;
    }

    void newMinute() {
        Serial.println(fmt::format("Full minute mark. Bits received: {}", current_bit).c_str());
        if (current_bit >= 59) {
            updateTime();
        }
        data = 0;
        current_bit = 0;
    }

    void updateTime() {
        Serial.println(fmt::format("Decoding data: {:064b}", data).c_str());

        TimeElements t;
        t.Second = 0;

        if (!check_parity(data, 21, 8)) {
            Serial.println("ERROR: Minute data failed parity check");
            return;
        }
        t.Minute = get_value(data, 21, 4) + 10 * get_value(data, 25, 3);


        if (!check_parity(data, 29, 7)) {
            Serial.println("ERROR: Hour data failed parity check");
            return;
        }
        t.Hour = get_value(data, 29, 4) + 10 * get_value(data, 33, 2);

        if (!check_parity(data, 36, 23)) {
            Serial.println("ERROR: Date data failed parity check");
            return;
        }
        t.Day = get_value(data, 36, 4) + 10 * get_value(data, 40, 2);
        t.Month = get_value(data, 45, 4) + 10 * get_value(data, 49, 1);

        // we only get the last two digits, so offset from 2000
        // FIXME: do something clever when we approach year 3000 to prevent a Y3k bug
        int decade = get_value(data, 54, 4);
        int year = get_value(data, 50, 4);
        year = 2000 + 10 * decade + year;

        // time library defines years as offset from 1970
        t.Year = year - 1970;

        bool cest = get_bit(data, 17);
        bool cet = get_bit(data, 18);
        if (cet && cest) {
            Serial.println("ERROR: CEST and CET bit set");
            return;
        }

        Serial.println(fmt::format(
            "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d} {}",
            year, t.Month, t.Day,
            t.Hour, t.Minute, t.Second,
            cest ? "CEST" : "CET"
        ).c_str());
        setTime(makeTime(t));
        valid = true;
    }

    void loop(bool current_value) {
        if (!state && current_value) {
            state = current_value;

            // new pulse starts
            previous_pulse_begin = current_pulse_begin;
            current_pulse_begin = millis();

            if (first_pulse_begin == 0) first_pulse_begin = current_pulse_begin;

            // there is a one second gap before the a new minute starts,
            // which also signals one complete data transfer
            if (previous_pulse_begin != 0 && (current_pulse_begin - previous_pulse_begin) > 1500) {
                newMinute();
            } 
        } else if (state && !current_value){
            state = current_value;

            // pulse end
            int duration = millis() - current_pulse_begin;
            int time_since_previous = current_pulse_begin - previous_pulse_begin;

            Serial.println(fmt::format("Falling edge. time since last pulse: {:4d}, high duration: {}", time_since_previous, duration).c_str());
            if (time_since_previous < 800 || duration < 80) {
                Serial.println("Short pulse, noise?");
                reset();
                return;
            }
            // DCF77 uses 200ms for an 1 bit, 100ms for 0 bit
            bool value = duration > 150;
            if (value) {
                data |= ONE << current_bit;
            }

            float t = (current_pulse_begin - first_pulse_begin) / 1000.0;
            Serial.println(fmt::format("{:8.1f} {:2d} {:1d} {:064b}", t, current_bit, value, data).c_str());
            current_bit++;
        }
    }

};

