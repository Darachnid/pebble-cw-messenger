#include <pebble.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "send.h"

#define TX_TEXT_CAP 160
#define TX_CODE_CAP 12
#define TX_VIBE_MAX 10000u

typedef struct {
  char letter;
  const char *code;
} CwPair;

// The same International Morse alphabet as the receive demo.
static const CwPair s_alphabet[] = {
  {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
  {'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."},
  {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."},
  {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."},
  {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
  {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"},
  {'Y', "-.--"}, {'Z', "--.."}, {'0', "-----"}, {'1', ".----"},
  {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
  {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
  {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."},
  {'/', "-..-."}, {':', "---..."}, {'=', "-...-"},
  {'+', ".-.-."}, {'-', "-....-"}, {'@', ".--.-."},
  {'\'', ".----."}
};

static Window *s_window;
static TextLayer *s_title_layer;
static TextLayer *s_message_layer;
static TextLayer *s_code_layer;
static TextLayer *s_footer_layer;
static AppTimer *s_mark_timer;
static AppTimer *s_gap_timer;
static AppTimer *s_letter_timer;
static AppTimer *s_word_timer;

static uint16_t s_wpm = 20;
static uint16_t s_frequency = 600;
static uint16_t s_unit_ms = 60;
static uint8_t s_volume = 60;
static bool s_active;
static bool s_up_held;
static bool s_down_held;
static bool s_up_pending;
static bool s_down_pending;
static bool s_straight_held;
static bool s_paddle_mark;
static int8_t s_last_paddle; // +1 for UP (dit); -1 for DOWN (dah)
static char s_mark_symbol;
static char s_text[TX_TEXT_CAP + 1];
static size_t s_text_length;
static char s_code[TX_CODE_CAP + 1];
static uint8_t s_code_length;
static uint32_t s_press_start;
static uint32_t s_last_mark_end;
static uint32_t s_vibration_duration;
static char s_footer_text[48];

// UTC with millisecond resolution. Unsigned subtraction handles rollover.
static uint32_t now_ms(void) {
  time_t seconds = 0;
  uint16_t millis = 0;
  time_ms(&seconds, &millis);
  return (uint32_t)seconds * 1000u + millis;
}

static void cancel_timer(AppTimer **timer) {
  if (*timer) {
    app_timer_cancel(*timer);
    *timer = NULL;
  }
}

static void draw_message(void) {
  if (s_message_layer) {
    text_layer_set_text(s_message_layer, s_text_length ?
                        s_text : "UP: DIT   DOWN: DAH\n\nSELECT: STRAIGHT KEY");
  }
  if (s_code_layer) {
    text_layer_set_text(s_code_layer, s_code_length ? s_code : " ");
  }
}

static void append_text(char value) {
  if (s_text_length >= TX_TEXT_CAP) {
    // Keep the most recent characters rather than overflowing the buffer.
    memmove(s_text, s_text + 1, TX_TEXT_CAP - 1);
    --s_text_length;
  }
  s_text[s_text_length++] = value;
  s_text[s_text_length] = '\0';
}

static char decode_code(const char *code) {
  for (size_t i = 0; i < sizeof(s_alphabet) / sizeof(s_alphabet[0]); ++i) {
    if (!strcmp(s_alphabet[i].code, code)) return s_alphabet[i].letter;
  }
  return '?';
}

static void finish_letter(void) {
  if (s_code_length) {
    append_text(decode_code(s_code));
    s_code_length = 0;
    s_code[0] = '\0';
    draw_message();
  }
}

static void letter_timeout(void *context) {
  (void)context;
  s_letter_timer = NULL;
  finish_letter();
}

static void word_timeout(void *context) {
  (void)context;
  s_word_timer = NULL;
  finish_letter();
  if (s_text_length && s_text[s_text_length - 1] != ' ') {
    append_text(' ');
    draw_message();
  }
}

static void prepare_mark(void) {
  // Recover from late timer delivery based on the actual recorded edge.
  if (s_last_mark_end && s_code_length) {
    const uint32_t gap = now_ms() - s_last_mark_end;
    if (gap >= 3u * s_unit_ms) finish_letter();
    if (gap >= 7u * s_unit_ms && s_text_length &&
        s_text[s_text_length - 1] != ' ') append_text(' ');
  }
  cancel_timer(&s_letter_timer);
  cancel_timer(&s_word_timer);
  draw_message();
}

static void schedule_decode(void) {
  s_last_mark_end = now_ms();
  cancel_timer(&s_letter_timer);
  cancel_timer(&s_word_timer);
  s_letter_timer = app_timer_register(3u * s_unit_ms, letter_timeout, NULL);
  s_word_timer = app_timer_register(7u * s_unit_ms, word_timeout, NULL);
}

static void record_symbol(char symbol) {
  if (s_code_length < TX_CODE_CAP) {
    s_code[s_code_length++] = symbol;
    s_code[s_code_length] = '\0';
  } else {
    // Overlong/unrecognized Morse still yields one '?' upon finalization.
    s_code_length = TX_CODE_CAP;
  }
  draw_message();
  schedule_decode();
}

// Live keying must NOT continuously pre-buffer PCM. The previous pump
// queued 30 ms of audio every 10 ms, building an arbitrarily long backlog.
// Use Pebble's direct square-wave tone generator on each physical key edge
// and stop it together with the vibration on key-up instead.
static void key_down(uint32_t duration_ms) {
  // Clear any previous note or draining audio from the speaker immediately.
  speaker_stop();
  const uint32_t tone_ms = duration_ms > TX_VIBE_MAX ? TX_VIBE_MAX : duration_ms;
  // Initiate the speaker request before vibration to minimize perceived lag.
  if (!speaker_play_tone(s_frequency, tone_ms, s_volume,
                         SpeakerWaveformSquare)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "TX tone failed: %u Hz, %lu ms",
            (unsigned)s_frequency, (unsigned long)tone_ms);
  }
  s_vibration_duration = tone_ms;
  VibePattern vibe = {
    .durations = &s_vibration_duration,
    .num_segments = 1
  };
  vibes_enqueue_custom_pattern(vibe);
}

static void key_up(void) {
  // Flush rather than drain queued sound. Do this on the actual button-up
  // event, NOT at character decode time.
  speaker_stop();
  vibes_cancel();
}

static void begin_paddle(void);

static void gap_done(void *context) {
  (void)context;
  s_gap_timer = NULL;
  begin_paddle();
}

static void mark_done(void *context) {
  (void)context;
  s_mark_timer = NULL;
  if (!s_active || !s_paddle_mark) return;
  s_paddle_mark = false;
  key_up();
  record_symbol(s_mark_symbol);
  // Mandatory one-unit inter-element gap.
  s_gap_timer = app_timer_register(s_unit_ms, gap_done, NULL);
}

static void begin_paddle(void) {
  if (!s_active || s_straight_held || s_paddle_mark || s_gap_timer) return;

  const bool up = s_up_pending || s_up_held;
  const bool down = s_down_pending || s_down_held;
  if (!up && !down) return;

  // Alternate when both paddles are held (simple squeeze; not iambic A/B).
  const bool choose_up = (up && down) ? s_last_paddle != 1 : up;
  if (choose_up) s_up_pending = false;
  else s_down_pending = false;

  s_last_paddle = choose_up ? 1 : -1;
  s_mark_symbol = choose_up ? '.' : '-';
  s_paddle_mark = true;
  prepare_mark();
  const uint32_t duration = choose_up ? s_unit_ms : 3u * s_unit_ms;
  key_down(duration);
  s_mark_timer = app_timer_register(duration, mark_done, NULL);
  if (!s_mark_timer) {
    // Fail safely; don't leave continuous key-down on timer exhaustion.
    s_paddle_mark = false;
    key_up();
  }
}

static void up_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (s_straight_held) return;
  s_up_held = true;
  s_up_pending = true;
  begin_paddle();
}

static void up_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_up_held = false;
}

static void down_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (s_straight_held) return;
  s_down_held = true;
  s_down_pending = true;
  begin_paddle();
}

static void down_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_down_held = false;
}

static void straight_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (!s_active || s_paddle_mark || s_gap_timer) return;
  s_straight_held = true;
  prepare_mark();
  s_press_start = now_ms();
  key_down(TX_VIBE_MAX);
}

static void straight_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (!s_straight_held) return;
  uint32_t held_ms = now_ms() - s_press_start;
  s_straight_held = false;
  key_up();
  // Decode arbitrary press duration relative to configured Morse unit.
  record_symbol(held_ms >= 2u * s_unit_ms ? '-' : '.');
  begin_paddle();
}

static void back_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  window_stack_pop(true);
}

static void configure_clicks(void *context) {
  (void)context;
  window_raw_click_subscribe(BUTTON_ID_UP, up_down, up_up, NULL);
  window_raw_click_subscribe(BUTTON_ID_DOWN, down_down, down_up, NULL);
  window_raw_click_subscribe(BUTTON_ID_SELECT, straight_down, straight_up, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click);
}

static void stop_session(void) {
  s_active = false;
  cancel_timer(&s_mark_timer);
  cancel_timer(&s_gap_timer);
  cancel_timer(&s_letter_timer);
  cancel_timer(&s_word_timer);
  key_up();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  const int16_t header_height = 40;
  const int16_t footer_height = 32;

  window_set_background_color(window, GColorWhite);

  s_title_layer = text_layer_create(
    GRect(0, 0, bounds.size.w, header_height));
  text_layer_set_background_color(s_title_layer, GColorBlack);
  text_layer_set_text_color(s_title_layer, GColorWhite);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  text_layer_set_text(s_title_layer, "SEND CW");
  layer_add_child(root, text_layer_get_layer(s_title_layer));

  s_message_layer = text_layer_create(
    GRect(8, header_height + 8, bounds.size.w - 16,
          bounds.size.h - header_height - footer_height - 40));
  text_layer_set_background_color(s_message_layer, GColorWhite);
  text_layer_set_text_color(s_message_layer, GColorBlack);
  text_layer_set_font(s_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_message_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_message_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_message_layer));

  s_code_layer = text_layer_create(
    GRect(8, bounds.size.h - footer_height - 34, bounds.size.w - 16, 32));
  text_layer_set_background_color(s_code_layer, GColorWhite);
  text_layer_set_text_color(s_code_layer, GColorBlack);
  text_layer_set_font(s_code_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_code_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_code_layer));

  s_footer_layer = text_layer_create(
    GRect(0, bounds.size.h - footer_height, bounds.size.w, footer_height));
  text_layer_set_background_color(s_footer_layer, GColorBlack);
  text_layer_set_text_color(s_footer_layer, GColorWhite);
  text_layer_set_font(s_footer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_footer_layer, GTextAlignmentCenter);
  snprintf(s_footer_text, sizeof(s_footer_text), "%u WPM  |  %u Hz",
           s_wpm, s_frequency);
  text_layer_set_text(s_footer_layer, s_footer_text);
  layer_add_child(root, text_layer_get_layer(s_footer_layer));
  draw_message();
}

static void window_unload(Window *window) {
  (void)window;
  stop_session();
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_message_layer);
  text_layer_destroy(s_code_layer);
  text_layer_destroy(s_footer_layer);
  s_title_layer = NULL;
  s_message_layer = NULL;
  s_code_layer = NULL;
  s_footer_layer = NULL;
}

void cw_send_open(uint16_t wpm, uint16_t tone_hz, uint8_t volume) {
  if (!s_window) {
    s_window = window_create();
    if (!s_window) return;
    window_set_click_config_provider(s_window, configure_clicks);
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    });
  }
  s_wpm = wpm ? wpm : 20;
  s_frequency = tone_hz ? tone_hz : 600;
  s_unit_ms = 1200u / s_wpm;
  if (!s_unit_ms) s_unit_ms = 1;
  s_volume = volume;
  s_up_held = s_down_held = s_straight_held = false;
  s_up_pending = s_down_pending = false;
  s_last_paddle = 0;
  s_paddle_mark = false;
  s_text[0] = '\0';
  s_text_length = 0;
  s_code[0] = '\0';
  s_code_length = 0;
  s_last_mark_end = 0;
  s_active = true;
  window_stack_push(s_window, true);
}

void cw_send_deinit(void) {
  stop_session();
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}
