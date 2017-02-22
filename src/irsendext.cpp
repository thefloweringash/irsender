#include <IRremoteESP8266.h>
#include <IRremoteInt.h>

#include "irsendext.h"

/**
 * Same encoding scheme as the built in encoding, but no arbitrary
 * split of address(16)/data(32).
 *
 * The remote I'm using has a 5-byte packet, and doesn't fit the
 * 6-byte packet of sendPanasonic. I don't see any reason to impose
 * rigid structure that doesn't fit, so let's just send a bytestream.
 */
void IRsendExt::sendPanasonicRaw(const uint8_t *data, size_t length) {
    enableIROut(35);

    mark (PANASONIC_HDR_MARK);
    space(PANASONIC_HDR_SPACE);

    for (size_t i = 0; i < length; ++i) {
        for (uint8_t bitmask = 1; bitmask; bitmask <<= 1) {
            mark(PANASONIC_BIT_MARK);
            space((data[i] & bitmask) ?
                  PANASONIC_ONE_SPACE :
                  PANASONIC_ZERO_SPACE);
        }
    }

    mark(PANASONIC_BIT_MARK);
    space(0);
}
