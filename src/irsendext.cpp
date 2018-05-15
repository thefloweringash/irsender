#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "irsendext.h"

// Verbatim from ir_Panasonic.cpp

#define PANASONIC_TICK                     432U
#define PANASONIC_HDR_MARK_TICKS             8U
#define PANASONIC_HDR_MARK         (PANASONIC_HDR_MARK_TICKS * PANASONIC_TICK)
#define PANASONIC_HDR_SPACE_TICKS            4U
#define PANASONIC_HDR_SPACE        (PANASONIC_HDR_SPACE_TICKS * PANASONIC_TICK)
#define PANASONIC_BIT_MARK_TICKS             1U
#define PANASONIC_BIT_MARK         (PANASONIC_BIT_MARK_TICKS * PANASONIC_TICK)
#define PANASONIC_ONE_SPACE_TICKS            3U
#define PANASONIC_ONE_SPACE        (PANASONIC_ONE_SPACE_TICKS * PANASONIC_TICK)
#define PANASONIC_ZERO_SPACE_TICKS           1U
#define PANASONIC_ZERO_SPACE       (PANASONIC_ZERO_SPACE_TICKS * PANASONIC_TICK)
#define PANASONIC_MIN_COMMAND_LENGTH_TICKS 378UL
#define PANASONIC_MIN_COMMAND_LENGTH (PANASONIC_MIN_COMMAND_LENGTH_TICKS * \
                                      PANASONIC_TICK)
#define PANASONIC_END_GAP              5000U  // See issue #245
#define PANASONIC_MIN_GAP_TICKS (PANASONIC_MIN_COMMAND_LENGTH_TICKS - \
    (PANASONIC_HDR_MARK_TICKS + PANASONIC_HDR_SPACE_TICKS + \
     PANASONIC_BITS * (PANASONIC_BIT_MARK_TICKS + PANASONIC_ONE_SPACE_TICKS) + \
     PANASONIC_BIT_MARK_TICKS))
#define PANASONIC_MIN_GAP ((uint32_t)(PANASONIC_MIN_GAP_TICKS * PANASONIC_TICK))


/**
 * Same encoding scheme as the built in encoding, but no arbitrary
 * split of address(16)/data(32).
 *
 * The remote I'm using has a 5-byte packet, and doesn't fit the
 * 6-byte packet of sendPanasonic. I don't see any reason to impose
 * rigid structure that doesn't fit, so let's just send a bytestream.
 */
void IRsendExt::sendPanasonicRaw(const uint8_t *data, size_t length) {
  sendGeneric(PANASONIC_HDR_MARK, PANASONIC_HDR_SPACE,
              PANASONIC_BIT_MARK, PANASONIC_ONE_SPACE,
              PANASONIC_BIT_MARK, PANASONIC_ZERO_SPACE,
              PANASONIC_BIT_MARK,
              PANASONIC_MIN_GAP,
              data, length, 36700U, false, 0, 50);
}
