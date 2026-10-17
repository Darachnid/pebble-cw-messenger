#include <pebble.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#define MESSAGE_SENDER "Ben"
#define MESSAGE_TEXT "good night bens best"
#define MORSE_WPM 20
#define TONE_HZ 600
#define SAMPLE_RATE 16000u
#define MAX_SEGMENTS 512
#define PCM_CHUNK 512
#define BACK_DOUBLE_MS 400

// Single, contiguous PCM stream: square carrier during marks, zero samples
// during spaces. No envelopes, fades, simulated reverb, or per-dit restart.
typedef struct {
  bool mark;
  uint16_t duration_ms;
} MorseSegment;

static Window *s_window;
static TextLayer *s_sender_layer;
static TextLayer *s_message_layer;
static TextLayer *s_footer_layer;
static AppTimer *s_vibe_timer;
static AppTimer *s_pcm_timer;
static MorseSegment s_segments[MAX_SEGMENTS];
static uint16_t s_segment_count;
static uint16_t s_vibe_cursor;
static uint32_t s_vibe_duration;
static uint8_t s_volume = 60;
static bool s_playing;
static bool s_audio_enabled = true;
static bool s_haptics_enabled = true;
static bool s_stream_open;
static char s_footer[48];

// Generator and write cursor are independent: handle partial stream writes.
static uint16_t s_pcm_segment;
static uint32_t s_pcm_samples_remaining;
static uint32_t s_carrier_phase;
static int8_t s_pcm_chunk[PCM_CHUNK];
static uint16_t s_pcm_size;
static uint16_t s_pcm_offset;
static bool s_pcm_generated_all;

static const char *morse_for(char c) {
  switch (toupper((unsigned char)c)) {
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
    case '\'': return ".----.";
    default: return NULL;
  }
}

static bool append_segment(bool mark, uint16_t ms) {
  if (!ms) return true;
  if (!mark && s_segment_count && !s_segments[s_segment_count - 1].mark) {
    s_segments[s_segment_count - 1].duration_ms += ms;
    return true;
  }
  if (s_segment_count == MAX_SEGMENTS) return false;
  s_segments[s_segment_count++] = (MorseSegment) {mark, ms};
  return true;
}

static bool encode_message(const char *message) {
  const uint16_t unit = 1200 / MORSE_WPM;
  s_segment_count = 0;
  bool had_letter = false;
  bool pending_word_gap = false;
  for (const char *p = message; *p; p++) {
    if (isspace((unsigned char)*p)) {
      if (had_letter) pending_word_gap = true;
      continue;
    }
    const char *pattern = morse_for(*p);
    if (!pattern) continue;
    if (had_letter && !append_segment(false, (pending_word_gap ? 7 : 3) * unit)) return false;
    for (const char *symbol = pattern; *symbol; symbol++) {
      if (symbol != pattern && !append_segment(false, unit)) return false;
      if (!append_segment(true, (*symbol == '-' ? 3 : 1) * unit)) return false;
    }
    had_letter = true;
    pending_word_gap = false;
  }
  return s_segment_count > 0;
}

static void audio_stop(void) {
  if (s_pcm_timer) {
    app_timer_cancel(s_pcm_timer);
    s_pcm_timer = NULL;
  }
  // Also abort any stream still draining after speaker_stream_close().
  speaker_stop();
  s_stream_open = false;
}

static void stop_playback(void) {
  if (s_vibe_timer) {
    app_timer_cancel(s_vibe_timer);
    s_vibe_timer = NULL;
  }
  audio_stop();
  vibes_cancel();
  s_playing = false;
}

static void pcm_fill_chunk(void) {
  s_pcm_size = 0;
  s_pcm_offset = 0;
  while (s_pcm_size < PCM_CHUNK) {
    while (!s_pcm_samples_remaining) {
      if (s_pcm_segment == s_segment_count) {
        s_pcm_generated_all = true;
        return;
      }
      s_pcm_samples_remaining = (uint32_t)s_segments[s_pcm_segment].duration_ms *
                                (SAMPLE_RATE / 1000u);
      if (!s_pcm_samples_remaining) s_pcm_segment++;
    }
    bool on = s_segments[s_pcm_segment].mark;
    // Fixed 50% duty cycle. The phase progresses through silent gaps so
    // the carrier is continuous; only its amplitude is keyed on/off.
    s_pcm_chunk[s_pcm_size++] = on ? (s_carrier_phase < SAMPLE_RATE / 2 ? 48 : -48) : 0;
    s_carrier_phase += TONE_HZ;
    if (s_carrier_phase >= SAMPLE_RATE) s_carrier_phase -= SAMPLE_RATE;
    if (--s_pcm_samples_remaining == 0) s_pcm_segment++;
  }
}

static void pcm_pump(void *context) {
  (void)context;
  s_pcm_timer = NULL;
  if (!s_stream_open || !s_audio_enabled) return;
  // Cap each event-loop pass; refill every 20 ms without blocking the UI.
  for (unsigned i = 0; i < 12; i++) {
    if (s_pcm_offset == s_pcm_size) {
      if (s_pcm_generated_all) {
        // Drain the final buffered PCM; does not insert a release envelope.
        speaker_stream_close();
        s_stream_open = false;
        return;
      }
      pcm_fill_chunk();
      if (!s_pcm_size) {
        speaker_stream_close();
        s_stream_open = false;
        return;
      }
    }
    uint32_t written = speaker_stream_write(s_pcm_chunk + s_pcm_offset,
                                             s_pcm_size - s_pcm_offset);
    s_pcm_offset += (uint16_t)written;
    if (!written) break;
  }
  s_pcm_timer = app_timer_register(20, pcm_pump, NULL);
  if (!s_pcm_timer) audio_stop();
}

static void vibe_next(void *context) {
  (void)context;
  s_vibe_timer = NULL;
  if (!s_playing) return;
  if (s_vibe_cursor == s_segment_count) {
    s_playing = false;
    return;
  }
  MorseSegment segment = s_segments[s_vibe_cursor++];
  if (segment.mark && s_haptics_enabled) {
    s_vibe_duration = segment.duration_ms;
    VibePattern pattern = {.durations = &s_vibe_duration, .num_segments = 1};
    vibes_enqueue_custom_pattern(pattern);
  } else if (!segment.mark) {
    vibes_cancel();
  }
  s_vibe_timer = app_timer_register(segment.duration_ms, vibe_next, NULL);
  if (!s_vibe_timer) stop_playback();
}

static void start_playback(void) {
  stop_playback();
  if (!encode_message(MESSAGE_TEXT)) return;
  s_audio_enabled = true;
  s_haptics_enabled = true;
  s_playing = true;
  s_vibe_cursor = 0;
  s_pcm_segment = 0;
  s_pcm_samples_remaining = 0;
  s_carrier_phase = 0;
  s_pcm_size = 0;
  s_pcm_offset = 0;
  s_pcm_generated_all = false;
  s_stream_open = speaker_stream_open(SpeakerPcmFormat_16kHz_8bit, s_volume);
  if (s_stream_open) pcm_pump(NULL);
  else s_audio_enabled = false;
  vibe_next(NULL);
}

static void volume_up(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_volume = s_volume >= 90 ? 100 : s_volume + 10;
  speaker_set_volume(s_volume);
}

static void volume_down(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  s_volume = s_volume <= 10 ? 0 : s_volume - 10;
  speaker_set_volume(s_volume);
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
    audio_stop();
  } else if (s_haptics_enabled) {
    s_haptics_enabled = false;
    vibes_cancel();
  }
}

static void back_double(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer; (void)context;
  if (s_playing) stop_playback();
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
  GRect frame = layer_get_bounds(root);
  const int16_t header_h = 40;
  const int16_t footer_h = 32;

  window_set_background_color(window, GColorWhite);
  s_sender_layer = text_layer_create(GRect(0, 0, frame.size.w, header_h));
  text_layer_set_background_color(s_sender_layer, GColorBlack);
  text_layer_set_text_color(s_sender_layer, GColorWhite);
  text_layer_set_font(s_sender_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_sender_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sender_layer, MESSAGE_SENDER);
  layer_add_child(root, text_layer_get_layer(s_sender_layer));

  s_message_layer = text_layer_create(GRect(10, header_h + 14,
                              frame.size.w - 20, frame.size.h - header_h - footer_h - 20));
  text_layer_set_background_color(s_message_layer, GColorWhite);
  text_layer_set_text_color(s_message_layer, GColorBlack);
  text_layer_set_font(s_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_message_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_message_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(s_message_layer, MESSAGE_TEXT);
  layer_add_child(root, text_layer_get_layer(s_message_layer));

  s_footer_layer = text_layer_create(GRect(0, frame.size.h - footer_h,
                                           frame.size.w, footer_h));
  text_layer_set_background_color(s_footer_layer, GColorBlack);
  text_layer_set_text_color(s_footer_layer, GColorWhite);
  text_layer_set_font(s_footer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_footer_layer, GTextAlignmentCenter);
  snprintf(s_footer, sizeof(s_footer), "%u WPM  |  %u Hz", MORSE_WPM, TONE_HZ);
  text_layer_set_text(s_footer_layer, s_footer);
  layer_add_child(root, text_layer_get_layer(s_footer_layer));
}

static void window_unload(Window *window) {
  (void)window;
  text_layer_destroy(s_footer_layer);
  text_layer_destroy(s_message_layer);
  text_layer_destroy(s_sender_layer);
  s_footer_layer = NULL;
  s_message_layer = NULL;
  s_sender_layer = NULL;
}

static void init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, clicks);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);
  // Prototype an incoming message by playing it as soon as it is shown.
  start_playback();
}

static void deinit(void) {
  stop_playback();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}