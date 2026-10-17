#include "config.h"
#include <message_keys.auto.h>
#include <string.h>
#include <stdlib.h>

#define CW_SETTINGS_KEY 918
#define CW_SETTINGS_VERSION 1
#define LEGACY_SYNC_KEY 701

static int32_t bounded(int32_t value, int32_t lo, int32_t hi) {
  return value < lo ? lo : value > hi ? hi : value;
}

void cw_settings_load(CwSettings *config) {
  memset(config, 0, sizeof(*config));
  config->version = CW_SETTINGS_VERSION;
  config->wpm = 20;
  config->tone_hz = 600;
  config->volume = 60;
  config->receive_audio = 1;
  config->receive_vibe = 1;
  config->send_audio = 1;
  config->send_vibe = 1;
  config->reverse_paddles = 0;
  config->straight_threshold_pct = 200;
  config->sync_ms = 80;
  config->start_mode = 0;
  config->autoplay = 1;
  strcpy(config->sender, "Ben");
  strcpy(config->demo_text, "good night bens best");

  CwSettings stored;
  if (persist_get_size(CW_SETTINGS_KEY) == (int)sizeof(stored) &&
      persist_read_data(CW_SETTINGS_KEY, &stored, sizeof(stored)) ==
      (int)sizeof(stored) &&
      stored.version == CW_SETTINGS_VERSION) {
    *config = stored;
  } else if (persist_exists(LEGACY_SYNC_KEY)) {
    // Preserve the receive alignment you already calibrated.
    config->sync_ms = persist_read_int(LEGACY_SYNC_KEY);
  }

  config->wpm = bounded(config->wpm, 5, 40);
  config->tone_hz = bounded(config->tone_hz, 200, 1500);
  config->volume = bounded(config->volume, 0, 100);
  config->straight_threshold_pct =
      bounded(config->straight_threshold_pct, 125, 300);
  config->sync_ms = bounded(config->sync_ms, -300, 400);
  config->start_mode = !!config->start_mode;
  config->autoplay = !!config->autoplay;
  config->receive_audio = !!config->receive_audio;
  config->receive_vibe = !!config->receive_vibe;
  config->send_audio = !!config->send_audio;
  config->send_vibe = !!config->send_vibe;
  config->reverse_paddles = !!config->reverse_paddles;
  config->sender[sizeof(config->sender)-1] = '\0';
  config->demo_text[sizeof(config->demo_text)-1] = '\0';
}

void cw_settings_save(const CwSettings *config) {
  persist_write_data(CW_SETTINGS_KEY, config, sizeof(*config));
  // Maintain compatibility with the old on-watch sync calibration.
  persist_write_int(LEGACY_SYNC_KEY, config->sync_ms);
}

static bool read_number(DictionaryIterator *iter, uint32_t key,
                        int32_t *dest, int32_t min, int32_t max) {
  Tuple *t = dict_find(iter, key);
  if (!t) return false;
  int32_t number = 0;
  // Pebble AppMessage encodes toggles as 1 byte and some sliders as 4.
  // Reading int32 unconditionally can use bytes beyond a 1-byte tuple.
  if (t->type == TUPLE_UINT) {
    if (t->length == 1) number = t->value->uint8;
    else if (t->length == 2) number = t->value->uint16;
    else if (t->length == 4) number = (int32_t)t->value->uint32;
    else return false;
  } else if (t->type == TUPLE_INT) {
    if (t->length == 1) number = t->value->int8;
    else if (t->length == 2) number = t->value->int16;
    else if (t->length == 4) number = t->value->int32;
    else return false;
  } else if (t->type == TUPLE_CSTRING && t->length > 0) {
    const char *str = t->value->cstring;
    if (!strcmp(str, "true")) number = 1;
    else if (!strcmp(str, "false")) number = 0;
    else {
      char *end = NULL;
      long parsed = strtol(str, &end, 10);
      if (end == str || *end != '\0') return false;
      number = (int32_t)parsed;
    }
  } else {
    return false;
  }
  *dest = bounded(number, min, max);
  return true;
}

static bool read_text(DictionaryIterator *iter, uint32_t key,
                      char *dest, size_t length) {
  Tuple *t = dict_find(iter, key);
  if (!t || t->type != TUPLE_CSTRING || t->length < 1) return false;
  strncpy(dest, t->value->cstring, length - 1);
  dest[length - 1] = '\0';
  return true;
}

bool cw_settings_from_appmessage(CwSettings *config, DictionaryIterator *iter) {
  bool changed = false;
  changed |= read_number(iter, MESSAGE_KEY_Wpm, &config->wpm, 5, 40);
  changed |= read_number(iter, MESSAGE_KEY_ToneHz, &config->tone_hz, 200, 1500);
  changed |= read_number(iter, MESSAGE_KEY_Volume, &config->volume, 0, 100);
  changed |= read_number(iter, MESSAGE_KEY_RxAudio, &config->receive_audio, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_RxVibe, &config->receive_vibe, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_TxAudio, &config->send_audio, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_TxVibe, &config->send_vibe, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_ReversePaddles,
                         &config->reverse_paddles, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_StraightThreshold,
                         &config->straight_threshold_pct, 125, 300);
  changed |= read_number(iter, MESSAGE_KEY_SyncMs,
                         &config->sync_ms, -300, 400);
  // read_number supports both numeric and Clay C-string selector tuples.
  changed |= read_number(iter, MESSAGE_KEY_StartMode,
                         &config->start_mode, 0, 1);
  changed |= read_number(iter, MESSAGE_KEY_AutoPlay,
                         &config->autoplay, 0, 1);
  changed |= read_text(iter, MESSAGE_KEY_DemoSender,
                       config->sender, sizeof(config->sender));
  changed |= read_text(iter, MESSAGE_KEY_DemoMessage,
                       config->demo_text, sizeof(config->demo_text));
  if (changed) cw_settings_save(config);
  return changed;
}
