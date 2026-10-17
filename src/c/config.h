#ifndef CW_CONFIG_H
#define CW_CONFIG_H
#include <pebble.h>
#include <stdint.h>

typedef struct {
  uint32_t version;
  int32_t wpm;
  int32_t tone_hz;
  int32_t volume;
  int32_t receive_audio;
  int32_t receive_vibe;
  int32_t send_audio;
  int32_t send_vibe;
  int32_t reverse_paddles;
  int32_t straight_threshold_pct;
  int32_t sync_ms;
  int32_t start_mode; // 0 = SEND first, 1 = RECEIVE first
  int32_t autoplay;
  char sender[32];
  char demo_text[128];
} CwSettings;

void cw_settings_load(CwSettings *config);
void cw_settings_save(const CwSettings *config);
bool cw_settings_from_appmessage(CwSettings *config, DictionaryIterator *iter);

#endif
