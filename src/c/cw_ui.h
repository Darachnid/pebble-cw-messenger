#pragma once

#include <pebble.h>

/** Shared chrome metrics for receive, send, and confirm screens. */
typedef struct CwScreenLayout {
  int16_t top_inset;
  int16_t side_inset;
  int16_t header_h;
  int16_t footer_h;
  int16_t body_pad_top;
  const char *header_font;
  const char *body_font;
  const char *code_font;
  const char *footer_font;
} CwScreenLayout;

CwScreenLayout cw_screen_layout(GRect bounds);

GColor cw_color_header_bg(void);
GColor cw_color_header_fg(void);
GColor cw_color_body_bg(void);
GColor cw_color_body_fg(void);
GColor cw_color_footer_bg(void);
GColor cw_color_footer_fg(void);

/** False on round models (chalk/gabbro) and other platforms without a speaker. */
bool cw_device_has_speaker(void);
