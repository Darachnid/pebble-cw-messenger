#include "cw_straight_key_icon.h"
#include "cw_ui.h"

typedef struct {
  bool pressed;
} StraightKeyIcon;

static void prv_update(Layer *layer, GContext *ctx) {
  StraightKeyIcon *icon = layer_get_data(layer);
  const GRect b = layer_get_bounds(layer);
  const bool down = icon && icon->pressed;

  GColor fg = down ? PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite) : cw_color_header_fg();
  graphics_context_set_fill_color(ctx, fg);
  graphics_context_set_stroke_color(ctx, fg);

  const int cx = b.size.w / 2;
  const int base_y = b.size.h - 1;
  const int pivot_y = base_y - 2;
  const int knob_r = b.size.w >= 20 ? 4 : 3;

  graphics_draw_line(ctx, GPoint(1, base_y), GPoint(b.size.w - 1, base_y));
  graphics_draw_line(ctx, GPoint(3, base_y), GPoint(3, pivot_y));
  graphics_draw_line(ctx, GPoint(b.size.w - 3, base_y), GPoint(b.size.w - 3, pivot_y));

  GPoint knob;
  GPoint foot;
  if (down) {
    foot = GPoint(cx + 2, pivot_y);
    knob = GPoint(cx - 5, 5);
  } else {
    foot = GPoint(cx, pivot_y);
    knob = GPoint(cx, 6);
  }
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, foot, knob);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_fill_circle(ctx, knob, knob_r);
}

Layer *cw_straight_key_icon_layer_create(GRect frame) {
  Layer *layer = layer_create_with_data(frame, sizeof(StraightKeyIcon));
  if (!layer) {
    return NULL;
  }
  StraightKeyIcon *icon = layer_get_data(layer);
  icon->pressed = false;
  layer_set_update_proc(layer, prv_update);
  return layer;
}

void cw_straight_key_icon_set_pressed(Layer *layer, bool pressed) {
  if (!layer) {
    return;
  }
  StraightKeyIcon *icon = layer_get_data(layer);
  if (icon->pressed == pressed) {
    return;
  }
  icon->pressed = pressed;
  layer_mark_dirty(layer);
}
