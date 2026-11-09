#include "cw_ui.h"

#include <pebble_sdk_version.h>

CwScreenLayout cw_screen_layout(GRect bounds) {
  CwScreenLayout layout;
  const bool compact =
#ifdef PBL_ROUND
      true;
#else
      bounds.size.h < 170;
#endif

  layout.top_inset = compact ? 10 : 0;
  layout.side_inset = compact ? 14 : 0;
  layout.header_h = compact ? 30 : 36;
  layout.footer_h = compact ? 28 : 30;
  layout.body_pad_top = compact ? 6 : 10;
  layout.header_font = compact ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD;
  layout.body_font = compact ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_28_BOLD;
  layout.code_font = FONT_KEY_GOTHIC_18_BOLD;
  layout.footer_font = compact ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_18_BOLD;
  (void)bounds;
  return layout;
}

GColor cw_color_header_bg(void) {
  return GColorBlack;
}

GColor cw_color_header_fg(void) {
  return GColorWhite;
}

GColor cw_color_body_bg(void) {
  return PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite);
}

GColor cw_color_body_fg(void) {
  return PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack);
}

GColor cw_color_footer_bg(void) {
  return PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
}

GColor cw_color_footer_fg(void) {
  return GColorWhite;
}

bool cw_device_has_speaker(void) {
#if defined(PBL_ROUND)
  return false;
#elif defined(_PBL_API_EXISTS_speaker_stream_open)
  return true;
#else
  return false;
#endif
}
