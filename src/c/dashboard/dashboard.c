#include "dashboard/dashboard.h"

#include <stdio.h>
#include <string.h>

#define DASH_SEGMENTS 10

typedef struct Dashboard {
  Layer *layer;
  RadioState state;
} Dashboard;

static GColor prv_accent_cyan(void) {
  return PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite);
}

static GColor prv_accent_blue(void) {
  return PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite);
}

static GColor prv_accent_yellow(void) {
  return PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite);
}

static GColor prv_accent_orange(void) {
  return PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite);
}

static GColor prv_text_primary(void) {
  return GColorWhite;
}

static GColor prv_bar_empty(void) {
  return PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
}

static GColor prv_field_color(RadioFieldFreshness freshness, GColor when_valid) {
  switch (freshness) {
    case RadioFieldFreshness_Valid:
      return when_valid;
    case RadioFieldFreshness_Stale:
      return prv_accent_yellow();
    case RadioFieldFreshness_Unavailable:
    default:
      return prv_text_primary();
  }
}

static void prv_draw_hline(GContext *ctx, GRect bounds, int y) {
  graphics_context_set_stroke_color(ctx, prv_text_primary());
  graphics_draw_line(ctx, GPoint(4, y), GPoint(bounds.size.w - 4, y));
}

static void prv_draw_segmented_bar(GContext *ctx, GRect frame, uint8_t filled, GColor fill,
                                   GColor empty) {
  if (filled > DASH_SEGMENTS) {
    filled = DASH_SEGMENTS;
  }
  const int gap = 2;
  const int seg_w = (frame.size.w - gap * (DASH_SEGMENTS - 1)) / DASH_SEGMENTS;
  for (int i = 0; i < DASH_SEGMENTS; i++) {
    GRect seg = GRect(frame.origin.x + i * (seg_w + gap), frame.origin.y, seg_w, frame.size.h);
    graphics_context_set_fill_color(ctx, (i < filled) ? fill : empty);
    graphics_fill_rect(ctx, seg, 0, GCornerNone);
  }
}

static void prv_draw_crystal_filter(GContext *ctx, GRect box, uint16_t bw_hz) {
  const int baseline_y = box.origin.y + box.size.h - 1;
  const int mid_x = box.origin.x + box.size.w / 2;
  const int max_half = box.size.w / 2 - 4;
  const int ref_hz = 2700;
  int half = (bw_hz >= ref_hz) ? max_half : (int)((uint32_t)bw_hz * max_half / ref_hz);
  if (half < 3) {
    half = 3;
  }

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(box.origin.x + 2, baseline_y - 8),
                     GPoint(box.origin.x + 2, baseline_y));
  graphics_draw_line(ctx, GPoint(box.origin.x + box.size.w - 2, baseline_y - 8),
                     GPoint(box.origin.x + box.size.w - 2, baseline_y));
  graphics_draw_line(ctx, GPoint(box.origin.x + 2, baseline_y),
                     GPoint(box.origin.x + box.size.w - 2, baseline_y));
  graphics_draw_line(ctx, GPoint(mid_x - half, baseline_y), GPoint(mid_x - half, baseline_y - 7));
  graphics_draw_line(ctx, GPoint(mid_x + half, baseline_y), GPoint(mid_x + half, baseline_y - 7));
  graphics_draw_line(ctx, GPoint(mid_x - half, baseline_y - 7), GPoint(mid_x - 3, baseline_y - 10));
  graphics_draw_line(ctx, GPoint(mid_x + half, baseline_y - 7), GPoint(mid_x + 3, baseline_y - 10));
  graphics_draw_line(ctx, GPoint(mid_x - 3, baseline_y - 10), GPoint(mid_x + 3, baseline_y - 10));
}

static void prv_draw_rit_fader(GContext *ctx, GRect track, bool enabled, int16_t offset_cHz) {
  const int mid_x = track.origin.x + track.size.w / 2;
  const int y = track.origin.y + track.size.h / 2;

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(track.origin.x, y), GPoint(track.origin.x + track.size.w, y));

  for (int i = 0; i <= 8; i++) {
    int tx = track.origin.x + (track.size.w * i) / 8;
    int tick_h = (i == 4) ? 5 : 3;
    graphics_draw_line(ctx, GPoint(tx, y - tick_h), GPoint(tx, y + tick_h));
  }

  if (!enabled) {
    return;
  }

  const int max_off = track.size.w / 2 - 2;
  int px = mid_x + (int)((int32_t)offset_cHz * max_off / 500);
  if (px < track.origin.x) {
    px = track.origin.x;
  }
  if (px > track.origin.x + track.size.w) {
    px = track.origin.x + track.size.w;
  }

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(px - 1, y - 9, 3, 10), 0, GCornerNone);
}

static const char *prv_audio_filter_label(K2AudioFilter af) {
  switch (af) {
    case K2AudioFilter_AF1:
      return "AF1";
    case K2AudioFilter_AF2:
      return "AF2";
    case K2AudioFilter_Off:
    default:
      return "OFF";
  }
}

static const char *prv_nb_label(K2NoiseBlanker nb) {
  switch (nb) {
    case K2NoiseBlanker_NB1:
      return "NB1";
    case K2NoiseBlanker_NB2:
      return "NB2";
    case K2NoiseBlanker_Off:
    default:
      return "OFF";
  }
}

static void prv_connection_label(const RadioState *state, char *buf, size_t len) {
  if (state->connection == RadioConnection_Offline ||
      state->connection_freshness == RadioFieldFreshness_Unavailable) {
    snprintf(buf, len, "OFFLINE");
    return;
  }
  if (state->connection == RadioConnection_Stale ||
      state->connection_freshness == RadioFieldFreshness_Stale) {
    snprintf(buf, len, "STALE");
    return;
  }
  snprintf(buf, len, "LIVE");
}

static void prv_dashboard_update(Layer *layer, GContext *ctx) {
  Dashboard *dash = *(Dashboard **)layer_get_data(layer);
  const RadioState *s = &dash->state;
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  char line[48];
  GRect rect;

  graphics_context_set_text_color(ctx, prv_accent_yellow());
  rect = GRect(6, 2, 48, 22);
  graphics_draw_text(ctx, "K2", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  prv_connection_label(s, line, sizeof(line));
  GColor conn_color = prv_accent_cyan();
  if (strcmp(line, "LIVE") != 0) {
    conn_color = prv_accent_yellow();
  }
  graphics_context_set_text_color(ctx, conn_color);
  rect = GRect(bounds.size.w - 52, 2, 46, 18);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  GRect filter_box = GRect(bounds.size.w - 50, 18, 44, 16);
  if (s->filter_freshness != RadioFieldFreshness_Unavailable) {
    prv_draw_crystal_filter(ctx, filter_box, s->crystal_bw_hz);
  }

  const char *af = prv_audio_filter_label(s->audio_filter);
  GColor af_color = prv_text_primary();
  if (s->audio_filter != K2AudioFilter_Off) {
    af_color = prv_accent_cyan();
  }
  graphics_context_set_text_color(ctx, prv_field_color(s->audio_filter_freshness, af_color));
  rect = GRect(bounds.size.w - 50, 34, 44, 14);
  graphics_draw_text(ctx, af, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  graphics_context_set_text_color(ctx, prv_field_color(s->frequency_freshness, GColorWhite));
  rect = GRect(0, 44, bounds.size.w, 38);
  graphics_draw_text(ctx, s->frequency_text, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS),
                     rect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  snprintf(line, sizeof(line), "%s \xE2\x80\xA2 %s", s->mode_text, s->band_text);
  graphics_context_set_text_color(ctx, prv_field_color(s->mode_freshness, prv_accent_orange()));
  rect = GRect(0, 78, bounds.size.w, 18);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  const int meter_y_label = 96;
  const int meter_y_bar = 118;
  const int meter_y_val = 130;
  const int col_gap = 10;
  const int half_w = (bounds.size.w - 16 - col_gap) / 2;
  const int left_x = 8;
  const int right_x = left_x + half_w + col_gap;
  const int bar_h = 10;
  GColor empty_bar = prv_bar_empty();

  graphics_context_set_text_color(ctx, prv_text_primary());
  rect = GRect(left_x, meter_y_label, half_w, 14);
  graphics_draw_text(ctx, "S-METER", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  rect = GRect(right_x, meter_y_label, half_w, 14);
  graphics_draw_text(ctx, "PWR", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  if (s->s_meter_freshness != RadioFieldFreshness_Unavailable) {
    prv_draw_segmented_bar(ctx, GRect(left_x, meter_y_bar, half_w, bar_h), s->s_meter_segments,
                           prv_accent_cyan(), empty_bar);
  }
  if (s->power_freshness != RadioFieldFreshness_Unavailable) {
    prv_draw_segmented_bar(ctx, GRect(right_x, meter_y_bar, half_w, bar_h), s->power_segments,
                           prv_accent_blue(), empty_bar);
  }

  graphics_context_set_text_color(ctx, prv_field_color(s->s_meter_freshness, prv_accent_cyan()));
  rect = GRect(left_x, meter_y_val, half_w, 16);
  graphics_draw_text(ctx, s->s_meter_label, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  const char *pwr_text =
      (s->power_freshness == RadioFieldFreshness_Unavailable) ? "--" : s->power_label;
  graphics_context_set_text_color(ctx, prv_field_color(s->power_freshness, prv_accent_blue()));
  rect = GRect(right_x, meter_y_val, half_w, 16);
  graphics_draw_text(ctx, pwr_text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  prv_draw_hline(ctx, bounds, 148);

  const int rit_label_y = 152;
  const int rit_track_y = 166;
  const int bottom_label_y = 198;
  const int bottom_value_y = 212;

  graphics_context_set_text_color(ctx, prv_text_primary());
  rect = GRect(8, rit_label_y, 40, 14);
  graphics_draw_text(ctx, "RIT", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  prv_draw_rit_fader(ctx, GRect(8, rit_track_y, bounds.size.w - 16, 22), s->rit_enabled,
                     s->rit_offset_cHz);

  graphics_context_set_text_color(ctx, prv_text_primary());
  rect = GRect(8, bottom_label_y, 56, 14);
  graphics_draw_text(ctx, "AGC", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  rect = GRect(bounds.size.w - 56, bottom_label_y, 48, 14);
  graphics_draw_text(ctx, "NB", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  const char *agc =
      (s->agc_freshness == RadioFieldFreshness_Unavailable) ? "--" : s->agc_text;
  graphics_context_set_text_color(ctx, prv_field_color(s->agc_freshness, GColorWhite));
  rect = GRect(8, bottom_value_y, 80, 16);
  graphics_draw_text(ctx, agc, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  const char *nb =
      (s->nb_freshness == RadioFieldFreshness_Unavailable) ? "--" : prv_nb_label(s->noise_blanker);
  GColor nb_color =
      (s->noise_blanker == K2NoiseBlanker_Off) ? prv_accent_yellow() : GColorWhite;
  graphics_context_set_text_color(ctx, prv_field_color(s->nb_freshness, nb_color));
  rect = GRect(bounds.size.w - 80, bottom_value_y, 72, 16);
  graphics_draw_text(ctx, nb, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

Dashboard *dashboard_create(void) {
  Dashboard *dash = malloc(sizeof(Dashboard));
  if (!dash) {
    return NULL;
  }
  radio_state_init(&dash->state);

  dash->layer = layer_create_with_data(GRect(0, 0, 200, 228), sizeof(Dashboard *));
  *(Dashboard **)layer_get_data(dash->layer) = dash;
  layer_set_update_proc(dash->layer, prv_dashboard_update);

  return dash;
}

void dashboard_destroy(Dashboard *dashboard) {
  if (!dashboard) {
    return;
  }
  layer_destroy(dashboard->layer);
  free(dashboard);
}

Layer *dashboard_get_layer(Dashboard *dashboard) {
  return dashboard ? dashboard->layer : NULL;
}

void dashboard_set_state(Dashboard *dashboard, const RadioState *state) {
  if (!dashboard || !state) {
    return;
  }
  dashboard->state = *state;
  layer_mark_dirty(dashboard->layer);
}
