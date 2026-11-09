#ifndef CW_SEND_H
#define CW_SEND_H
#include <stdint.h>
#include "config.h"

// Open an isolated local transmit/practice screen. No SMS is sent.
// The caller retains ownership of the receiving screen and its audio state.
void cw_send_open(const CwSettings *settings);
void cw_send_deinit(void);
// Safely switch to Receive only when there is no active or drafted CW.
bool cw_send_can_auto_switch(void);

void cw_send_receive_review(const char *text);
void cw_send_receive_status(const char *status);
#endif
