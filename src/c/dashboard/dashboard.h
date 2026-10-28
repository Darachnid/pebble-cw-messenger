#pragma once

#include "radio/radio_state.h"

#include <pebble.h>

typedef struct Dashboard Dashboard;

Dashboard *dashboard_create(void);
void dashboard_destroy(Dashboard *dashboard);

Layer *dashboard_get_layer(Dashboard *dashboard);

void dashboard_set_state(Dashboard *dashboard, const RadioState *state);
