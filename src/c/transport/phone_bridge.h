#pragma once

#include "dashboard/dashboard.h"

/** Register AppMessage inbox and open the channel (PKJS → watch). */
void phone_bridge_init(Dashboard *dashboard);
