#pragma once

#include <pebble.h>

/** Vertical layout constants derived from the 200×228 emery design. */
typedef struct DashboardLayout {
  GRect bounds;
  int inset_x;
  int content_w;
  int y_header;
  int y_filter;
  int y_freq;
  int h_freq;
  const char *freq_font;
  int y_mode;
  int y_meter_label;
  int y_meter_bar;
  int y_meter_val;
  int y_divider;
  int y_rit_label;
  int y_rit_track;
  int y_bottom_label;
  int y_bottom_val;
} DashboardLayout;

DashboardLayout dashboard_layout_for_bounds(GRect bounds);
