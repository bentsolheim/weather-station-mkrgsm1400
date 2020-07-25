#include <Arduino.h>
#include "LedMgr.h"

LedMgr::LedMgr(int pin) {
    this->pin = pin;
    initLed();
}

void LedMgr::blink(int ms) const {
    this->on();
    delay(ms);
    this->off();
}

void LedMgr::blink(int count, int ms) const {
    for (int i = 0; i < count; i++) {
        blink(ms);
        delay(ms);
    }
}

void LedMgr::on() const {
    digitalWrite(this->pin, HIGH);
}

void LedMgr::off() const {
    digitalWrite(this->pin, LOW);
}
