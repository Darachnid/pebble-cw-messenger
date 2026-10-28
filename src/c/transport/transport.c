#include "transport/transport.h"

#include <stdlib.h>

struct Transport {
  bool connected;
  TransportStateCallback callback;
  void *callback_context;
};

Transport *transport_create(void) {
  Transport *t = malloc(sizeof(Transport));
  if (t) {
    t->connected = false;
    t->callback = NULL;
    t->callback_context = NULL;
  }
  return t;
}

void transport_destroy(Transport *transport) {
  free(transport);
}

bool transport_is_connected(const Transport *transport) {
  return transport && transport->connected;
}

void transport_set_state_callback(Transport *transport, TransportStateCallback callback,
                                    void *context) {
  if (!transport) {
    return;
  }
  transport->callback = callback;
  transport->callback_context = context;
}
