#ifndef CW_SEND_H
#define CW_SEND_H
#include <stdint.h>

// Open an isolated local transmit/practice screen. No SMS is sent.
// The caller retains ownership of the receiving screen and its audio state.
void cw_send_open(uint16_t wpm, uint16_t tone_hz, uint8_t volume);
void cw_send_deinit(void);

#endif
