#include <pebble.h>

#include "dashboard/dashboard.h"
#include "radio/radio_state.h"
#include "radio/simulated_radio.h"
#include "transport/phone_bridge.h"

static Window *s_window;
static Dashboard *s_dashboard;

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_dashboard = dashboard_create();
  if (!s_dashboard) {
    return;
  }

  Layer *dash_layer = dashboard_get_layer(s_dashboard);
  layer_set_frame(dash_layer, bounds);

  RadioState state;
  simulated_radio_apply(&state);
  dashboard_set_state(s_dashboard, &state);

  layer_add_child(root, dash_layer);
  phone_bridge_init(s_dashboard);
}

static void prv_window_unload(Window *window) {
  (void)window;
  dashboard_destroy(s_dashboard);
  s_dashboard = NULL;
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
      .load = prv_window_load,
      .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "K2 dashboard initialized");
  app_event_loop();
  prv_deinit();
}
