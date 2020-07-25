#ifndef LEDMRG_H
#define LEDMRG_H

#include <Arduino.h>

class LedMgr {
private:
    int pin;

    void initLed() const {
        pinMode(pin, OUTPUT);
    }

public:
    explicit LedMgr(int pin);

    void blink(int ms) const;

    void blink(int count, int ms) const;

    void on() const;

    void off() const;
};

#endif