#pragma once

#include <stdbool.h>

/** Phone / KIO2 bridge (Milestone 3+). Milestone 1 uses simulated data only. */
typedef struct Transport Transport;

typedef void (*TransportStateCallback)(void *context);

Transport *transport_create(void);
void transport_destroy(Transport *transport);

bool transport_is_connected(const Transport *transport);

void transport_set_state_callback(Transport *transport, TransportStateCallback callback,
                                    void *context);
