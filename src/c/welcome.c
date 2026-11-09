#include "welcome.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

typedef struct {
  const char *title;
  const char *body;
  const char *action;
} WelcomeStep;

static Window *s_window;
static TextLayer *s_title_layer;
static TextLayer *s_body_layer;
static TextLayer *s_action_layer;
static uint8_t s_step;
static bool s_replay;
static WelcomeDoneCallback s_on_done;

static const WelcomeStep *prv_steps;
static uint8_t prv_step_count;

static void prv_build_steps(void) {
  static WelcomeStep steps[8];
  static char body_rx[160];
  static char body_cal[160];
  static bool built;
  if (built) {
    return;
  }
  built = true;

  const bool speaker = cw_device_has_speaker();
  const char *rx_media = speaker ? "sound and vibes" : "vibration only";

  steps[0] = (WelcomeStep){
      "WELCOME",
      "CW Messenger turns SMS into Morse on your wrist. Read as CW; "
      "compose replies with the paddle keys (UP dit, DOWN dah) or straight "
      "key (SELECT).",
      "SELECT: NEXT"};

  steps[1] = (WelcomeStep){
      "PHONE SETUP",
      "Open CW SMS Bridge: grant notifications, enable SMS, open Messages "
      "for history, and User settings for WPM and demo text.",
      "SELECT: NEXT"};

  snprintf(body_rx, sizeof(body_rx),
           "New texts open this screen. Morse plays with %s. "
           "After the tour the demo plays each time you open Receive until "
           "you LONG BACK here to turn that off.",
           rx_media);
  steps[2] = (WelcomeStep){"RECEIVE", body_rx, "SELECT: NEXT"};

  steps[3] = (WelcomeStep){
      "PLAY & VOLUME",
      "SELECT plays CW. UP/DOWN adjust volume."
      " BACK once exits after playback; BACK twice stops early.",
      "SELECT: NEXT"};

  if (speaker) {
    snprintf(body_cal, sizeof(body_cal),
             "LONG SELECT toggles audio/vibe sync. Use UP/DOWN for +/-10 ms "
             "while calibrating.");
  } else {
    snprintf(body_cal, sizeof(body_cal),
             "LONG SELECT toggles vibe timing tweaks. Use UP/DOWN for +/-10 ms "
             "while calibrating.");
  }
  steps[4] = (WelcomeStep){"CALIBRATION", body_cal, "SELECT: NEXT"};

  steps[5] = (WelcomeStep){
      "SEND CW",
      "HOLD DOWN from Receive to open the keyer. UP = dit, DOWN = dah, "
      "SELECT = straight key.",
      "SELECT: NEXT"};

  steps[6] = (WelcomeStep){
      "REPLY WORKFLOW",
      "Triple BACK sends draft for spellcheck on the phone. SELECT on the "
      "confirm screen sends via Messages.",
      "SELECT: FINISH"};

  prv_steps = steps;
  prv_step_count = 7;
}

static void prv_redraw(void) {
  if (!s_title_layer || !prv_steps) {
    return;
  }
  const WelcomeStep *st = &prv_steps[s_step];
  text_layer_set_text(s_title_layer, st->title);
  text_layer_set_text(s_body_layer, st->body);
  text_layer_set_text(s_action_layer, st->action);
}

static void prv_finish(bool mark_done, bool completed_tour) {
  if (mark_done && !s_replay) {
    cw_onboarding_mark_complete();
  }
  window_stack_remove(s_window, true);
  if (s_on_done) {
    WelcomeDoneCallback cb = s_on_done;
    s_on_done = NULL;
    int outcome = CW_WELCOME_READY;
    if (!completed_tour) {
      outcome = CW_WELCOME_SKIPPED;
    } else if (!s_replay) {
      outcome = CW_WELCOME_FINISHED;
    }
    cb(outcome);
  }
}

static void prv_click(ClickRecognizerRef ref, void *ctx) {
  (void)ref;
  (void)ctx;
  if (s_step + 1 >= prv_step_count) {
    prv_finish(true, true);
    return;
  }
  s_step++;
  prv_redraw();
}

static void prv_back(ClickRecognizerRef ref, void *ctx) {
  (void)ref;
  (void)ctx;
  if (s_step == 0) {
    prv_finish(true, false);
    return;
  }
  s_step--;
  prv_redraw();
}

static void prv_clicks(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_click);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back);
}

static void prv_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect frame = layer_get_bounds(root);
  const CwScreenLayout layout = cw_screen_layout(frame);

  window_set_background_color(window, cw_color_body_bg());

  s_title_layer = text_layer_create(GRect(layout.side_inset, layout.top_inset,
                                          frame.size.w - 2 * layout.side_inset,
                                          layout.header_h));
  text_layer_set_background_color(s_title_layer, cw_color_header_bg());
  text_layer_set_text_color(s_title_layer, cw_color_header_fg());
  text_layer_set_font(s_title_layer, fonts_get_system_font(layout.header_font));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_title_layer));

  const int body_y = layout.top_inset + layout.header_h + layout.body_pad_top;
  const int body_h = frame.size.h - body_y - layout.footer_h - layout.top_inset - 4;
  s_body_layer = text_layer_create(
      GRect(layout.side_inset, body_y, frame.size.w - 2 * layout.side_inset, body_h));
  text_layer_set_background_color(s_body_layer, cw_color_body_bg());
  text_layer_set_text_color(s_body_layer, cw_color_body_fg());
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_body_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_body_layer));

  s_action_layer = text_layer_create(GRect(
      layout.side_inset, frame.size.h - layout.footer_h - layout.top_inset,
      frame.size.w - 2 * layout.side_inset, layout.footer_h));
  text_layer_set_background_color(s_action_layer, cw_color_footer_bg());
  text_layer_set_text_color(s_action_layer, cw_color_footer_fg());
  text_layer_set_font(s_action_layer, fonts_get_system_font(layout.footer_font));
  text_layer_set_text_alignment(s_action_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_action_layer));

  prv_redraw();
}

static void prv_unload(Window *window) {
  (void)window;
  text_layer_destroy(s_action_layer);
  text_layer_destroy(s_body_layer);
  text_layer_destroy(s_title_layer);
  s_action_layer = NULL;
  s_body_layer = NULL;
  s_title_layer = NULL;
}

static void prv_push(bool replay, WelcomeDoneCallback on_done) {
  prv_build_steps();
  s_replay = replay;
  s_on_done = on_done;
  s_step = 0;
  if (!s_window) {
    s_window = window_create();
    window_set_click_config_provider(s_window, prv_clicks);
    window_set_window_handlers(s_window, (WindowHandlers){
        .load = prv_load,
        .unload = prv_unload,
    });
  }
  window_stack_push(s_window, true);
}

void welcome_push_if_needed(CwSettings *cfg, WelcomeDoneCallback on_done) {
  (void)cfg;
  const bool first = !cw_onboarding_complete();
  const bool again = cw_show_welcome_pending();
  if (!first && !again) {
    if (on_done) {
      on_done(CW_WELCOME_READY);
    }
    return;
  }
  if (again) {
    cw_show_welcome_clear();
  }
  prv_push(!first, on_done);
}

void welcome_push_replay(WelcomeDoneCallback on_done) {
  prv_push(true, on_done);
}

bool welcome_is_active(void) {
  return s_window && window_stack_get_top_window() == s_window;
}
