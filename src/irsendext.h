#ifndef IRSENDEXT_H
#define IRSENDEXT_H

#include <arduino.h>
#include <IRremoteESP8266.h>

class IRsendExt : public IRsend {
public:
    IRsendExt(uint8_t pin)
        : IRsend {pin}
    {}

    void sendPanasonicRaw(const uint8_t* data, size_t length);
};

enum custom_encoding_t {
    RAW           = 240,
    PANASONIC_RAW = 241,
};

#endif
