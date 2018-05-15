#ifndef IRSENDEXT_H
#define IRSENDEXT_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

class IRsendExt : public IRsend {
public:
    IRsendExt(uint8_t pin)
        : IRsend {pin}
    {}

    void sendPanasonicRaw(const uint8_t* data, size_t length);
};

enum custom_encoding_t {
    DELAY         = 239,
    PANASONIC_RAW = 241,
};

#endif
