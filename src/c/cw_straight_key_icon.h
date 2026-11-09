#pragma once

#include <pebble.h>

Layer *cw_straight_key_icon_layer_create(GRect frame);
void cw_straight_key_icon_set_pressed(Layer *layer, bool pressed);
