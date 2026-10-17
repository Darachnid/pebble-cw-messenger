#include <pebble.h>
#include <ctype.h>
#include <stdio.h>

#define DEMO_TEXT "SOS AC1RV"
#define MORSE_WPM 20
#define TONE_HZ 600
#define MAX_SEGMENTS 512
#define BACK_DOUBLE_MS 400

typedef struct {
  bool mark;
  uint16_t duration_ms;
} MorseSegment;

static Window *s_window;
static TextLayer *s_text;
static AppTimer *s_timer;
static MorseSegment s_segments[MAX_SEGMENTS];
static uint16_t s_segment_count;
static uint16_t s_cursor;
static uint32_t s_vibe_duration;
static uint8_t s_volume = 60;
static bool s_playing;
static bool s_audio_enabled = true;
static bool s_haptics_enabled = true;
static const char *s_status = "READY";
static char s_display[200];

static const char *morse_for(char input) {
  switch (toupper((unsigned char)input)) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    case '.': return ".-.-.-";
    case ',': return "--..--";
    case '?': return "..--..";
    case '/': return "-..-.";
    case ':': return "---...";
    case '=': return "-...-";
    case '+': return ".-.-.";
    case '-': return "-....-";
    case '@': return ".--.-.";
    default: return NULL;
  }
}

static bool append_segment(bool mark, uint16_t ms) {
  if (!ms) return true;
  // Merge adjacent spaces, but not individual marks.
  if (!mark && s_segment_count > 0 &&
      !s_segments[s_segment_count - 1].mark) {
    s_segments[s_segment_count - 1].duration_ms += ms;
    return true;
  }
  if (s_segment_count >= MAX_SEGMENTS) return false;
  s_segments[s_segment_count++] = (MorseSegment) {
    .mark = mark,
    .duration_ms = ms
  };
  return true;
}

static bool encode_message(const char *message, uint16_t wpm) {
  if (wpm == 0) return false;
  const uint16_t unit = (1200 + wpm / 2) / wpm;
  s_segment_count = 0;
  bool sent_letter = false;
  bool word_gap = false;

  for (const char *p = message; *p; ++p) {
    if (*p == ' ' || *p == '\n' || *p == '\t') {
      if (sent_letter) word_gap = true;
      continue;
    }
    const char *pattern = morse_for(*p);
    if (!pattern) continue; // Unknown characters are omitted.

    if (sent_letter && !append_segment(false, (word_gap ? 7 : 3) * unit)) {
      return false;
    }
    for (const char *symbol = pattern; *symbol; ++symbol) {
      if (symbol != pattern && !append_segment(false, unit)) return false;
      if (!append_segment(true, (*symbol == '-' ? 3 : 1) * unit)) return false;
    }
    sent_letter = true;
    word_gap = false;
  }
  return s_segment_count > 0;
}

static void redraw(void) {
  const char *mode = (!s_audio_enabled && !s_haptics_enabled) ? "SILENT" :
                     !s_audio_enabled ? "HAPTIC ONLY" :
                     !s_haptics_enabled ? "AUDIO ONLY" : "AUDIO + HAPTIC";
  snprintf(s_display, sizeof(s_display),
           "CW MESSENGER\n%s\n\n%s\n%s\n%u WPM | %u Hz\nVolume: %u%%\n\nSELECT Play\nUP/DOWN Volume\nBACK Mute\n2x BACK Stop",
           DEMO_TEXT, s_status, mode,
           MORSE_WPM, TONE_HZ, s_volume);
  if (s_text) text_layer_set_text(s_text, s_display);
}

static void stop_playback(const char *status) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  speaker_stop();
  vibes_cancel();
  s_playing = false;
  s_status = status;
  redraw();
}

static void play_next(void *context) {
  (void)context;
  s_timer = NULL;
  if (!s_playing) return;
  if (s_cursor >= s_segment_count) {
    stop_playback("DONE");
    return;
  }

  MorseSegment segment = s_segments[s_cursor++];
  if (segment.mark) {
    if (s_audio_enabled) {
      // The speaker API is supported on the Pebble Time 2 (emery).
      speaker_play_tone(TONE_HZ, segment.duration_ms,
                        s_volume, SpeakerWaveformSine);
    }
    if (s_haptics_enabled) {
      s_vibe_duration = segment.duration_ms;
      const VibePattern pattern = {
        .durations = &s_vibe_duration,
        .num_segments = 1
      };
      vibes_enqueue_custom_pattern(pattern);
    }
  }
  // A single timeline drives audio and haptics. Callback jitter still
  // needs to be characterized on real hardware.
  s_timer = app_timer_register(segment.duration_ms, play_next, NULL);
  if (!s_timer) stop_playback("TIMER ERROR");
}

static void start_playback(void) {
  stop_playback("READY");
  if (!encode_message(DEMO_TEXT, MORSE_WPM)) {
    s_status = "ENCODE ERROR";
    redraw();
    return;
  }
  s_cursor = 0;
  s_audio_enabled = true;
  s_haptics_enabled = true;
  s_playing = true;
  s_status = "PLAYING";
  redraw();
  play_next(NULL);
}

static void volume_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_volume = s_volume > 90 ? 100 : s_volume + 10;
  speaker_set_volume(s_volume);
  redraw();
}

static void volume_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_volume = s_volume < 10 ? 0 : s_volume - 10;
  speaker_set_volume(s_volume);
  redraw();
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  start_playback();
}

static void back_single(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (!s_playing) {
    window_stack_pop(true);
    return;
  }
  if (s_audio_enabled) {
    s_audio_enabled = false;
    speaker_stop();
  } else if (s_haptics_enabled) {
    s_haptics_enabled = false;
    vibes_cancel();
  }
  redraw();
}

static void back_double(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (s_playing) stop_playback("STOPPED");
}

static void clicks(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_click_subscribe(BUTTON_ID_UP, volume_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, volume_down);
  window_single_click_subscribe(BUTTON_ID_BACK, back_single);
  window_multi_click_subscribe(BUTTON_ID_BACK, 2, 2,
                               BACK_DOUBLE_MS, true, back_double);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_text = text_layer_create(GRect(4, 0, bounds.size.w - 8, bounds.size.h));
  text_layer_set_font(s_text, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_text, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_text, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_text));
  redraw();
}

static void window_unload(Window *window) {
  (void)window;
  s_text = NULL;
}

static void init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, clicks);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  stop_playback("STOPPED");
  if (s_text) {
    text_layer_destroy(s_text);
    s_text = NULL;
  }
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
