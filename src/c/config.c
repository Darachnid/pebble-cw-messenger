#include "config.h"
#include "cw_ui.h"
#include <message_keys.auto.h>
#include <string.h>
#include <stdlib.h>

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
  int32_t start_mode;
  int32_t autoplay;
  char sender[32];
  char demo_text[128];
} CwSettingsV1;

static bool s_show_welcome_session;

#define CW_SETTINGS_KEY 918
#define CW_SETTINGS_VERSION 2
#define LEGACY_SYNC_KEY 701
#define CW_ONBOARDING_KEY 919
#define CW_SHOW_WELCOME_PENDING_KEY 921

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
  config->start_mode = 1;
  config->autoplay = 1;
  config->demo_autostart = 1;
  strcpy(config->sender, "Ben");
  strcpy(config->demo_text, "test");

  CwSettings stored;
  const int stored_size = persist_get_size(CW_SETTINGS_KEY);
  if (stored_size == (int)sizeof(stored) &&
      persist_read_data(CW_SETTINGS_KEY, &stored, sizeof(stored)) ==
      (int)sizeof(stored) &&
      stored.version == CW_SETTINGS_VERSION) {
    *config = stored;
  } else if (stored_size == (int)sizeof(CwSettingsV1)) {
    CwSettingsV1 legacy;
    if (persist_read_data(CW_SETTINGS_KEY, &legacy, sizeof(legacy)) ==
        (int)sizeof(legacy) && legacy.version == 1) {
      config->wpm = legacy.wpm;
      config->tone_hz = legacy.tone_hz;
      config->volume = legacy.volume;
      config->receive_audio = legacy.receive_audio;
      config->receive_vibe = legacy.receive_vibe;
      config->send_audio = legacy.send_audio;
      config->send_vibe = legacy.send_vibe;
      config->reverse_paddles = legacy.reverse_paddles;
      config->straight_threshold_pct = legacy.straight_threshold_pct;
      config->sync_ms = legacy.sync_ms;
      config->start_mode = legacy.start_mode;
      config->autoplay = legacy.autoplay;
      config->demo_autostart = 1;
      strncpy(config->sender, legacy.sender, sizeof(config->sender));
      strncpy(config->demo_text, legacy.demo_text, sizeof(config->demo_text));
      cw_settings_save(config);
    }
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
  config->demo_autostart = !!config->demo_autostart;
  config->receive_audio = !!config->receive_audio;
  config->receive_vibe = !!config->receive_vibe;
  config->send_audio = !!config->send_audio;
  config->send_vibe = !!config->send_vibe;
  config->reverse_paddles = !!config->reverse_paddles;
  config->sender[sizeof(config->sender)-1] = '\0';
  config->demo_text[sizeof(config->demo_text)-1] = '\0';
  cw_settings_clamp_for_hardware(config);
}

void cw_settings_clamp_for_hardware(CwSettings *config) {
  if (!cw_device_has_speaker()) {
    config->receive_audio = 0;
    config->send_audio = 0;
  }
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
  changed |= read_number(iter, MESSAGE_KEY_DemoAutostart,
                         &config->demo_autostart, 0, 1);
  {
    int32_t show = 0;
    if (read_number(iter, MESSAGE_KEY_ShowWelcome, &show, 0, 1) && show) {
      cw_show_welcome_request();
    }
  }
  changed |= read_text(iter, MESSAGE_KEY_DemoSender,
                       config->sender, sizeof(config->sender));
  changed |= read_text(iter, MESSAGE_KEY_DemoMessage,
                       config->demo_text, sizeof(config->demo_text));
  cw_settings_clamp_for_hardware(config);
  if (changed) cw_settings_save(config);
  return changed;
}

bool cw_onboarding_complete(void) {
  return persist_exists(CW_ONBOARDING_KEY) && persist_read_int(CW_ONBOARDING_KEY) != 0;
}

void cw_onboarding_mark_complete(void) {
  persist_write_int(CW_ONBOARDING_KEY, 1);
}

void cw_onboarding_reset(void) {
  persist_delete(CW_ONBOARDING_KEY);
}

void cw_show_welcome_request(void) {
  persist_write_int(CW_SHOW_WELCOME_PENDING_KEY, 1);
  s_show_welcome_session = true;
}

bool cw_show_welcome_pending(void) {
  if (s_show_welcome_session) {
    return true;
  }
  return persist_exists(CW_SHOW_WELCOME_PENDING_KEY) &&
         persist_read_int(CW_SHOW_WELCOME_PENDING_KEY) != 0;
}

void cw_show_welcome_clear(void) {
  s_show_welcome_session = false;
  persist_delete(CW_SHOW_WELCOME_PENDING_KEY);
}
