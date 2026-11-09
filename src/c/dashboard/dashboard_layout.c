#include "dashboard/dashboard_layout.h"

DashboardLayout dashboard_layout_for_bounds(GRect bounds) {
  DashboardLayout l;
  l.bounds = bounds;

  const bool compact =
#ifdef PBL_ROUND
      true;
#else
      bounds.size.h < 200;
#endif

  l.inset_x = compact ? 20 : 8;
  l.content_w = bounds.size.w - 2 * l.inset_x;

  if (compact) {
    l.y_header = 6;
    l.y_filter = 20;
    l.y_freq = 34;
    l.h_freq = 34;
    l.freq_font = FONT_KEY_LECO_32_BOLD_NUMBERS;
    l.y_mode = 66;
    l.y_meter_label = 80;
    l.y_meter_bar = 98;
    l.y_meter_val = 110;
    l.y_divider = 124;
    l.y_rit_label = 128;
    l.y_rit_track = 140;
    l.y_bottom_label = 158;
    l.y_bottom_val = 170;
  } else {
    l.y_header = 2;
    l.y_filter = 18;
    l.y_freq = 44;
    l.h_freq = 38;
    l.freq_font = FONT_KEY_LECO_38_BOLD_NUMBERS;
    l.y_mode = 78;
    l.y_meter_label = 96;
    l.y_meter_bar = 118;
    l.y_meter_val = 130;
    l.y_divider = 148;
    l.y_rit_label = 152;
    l.y_rit_track = 166;
    l.y_bottom_label = 198;
    l.y_bottom_val = 212;
  }

  return l;
}
