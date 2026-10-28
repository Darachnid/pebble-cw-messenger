#include "radio/radio_state.h"

#include <string.h>

void radio_state_init(RadioState *state) {
  memset(state, 0, sizeof(*state));
  state->connection = RadioConnection_Offline;
  state->connection_freshness = RadioFieldFreshness_Unavailable;
  state->frequency_freshness = RadioFieldFreshness_Unavailable;
  state->mode_freshness = RadioFieldFreshness_Unavailable;
  state->filter_freshness = RadioFieldFreshness_Unavailable;
  state->audio_filter = K2AudioFilter_Off;
  state->audio_filter_freshness = RadioFieldFreshness_Unavailable;
  state->s_meter_freshness = RadioFieldFreshness_Unavailable;
  state->power_freshness = RadioFieldFreshness_Unavailable;
  state->rit_freshness = RadioFieldFreshness_Unavailable;
  state->agc_freshness = RadioFieldFreshness_Unavailable;
  state->noise_blanker = K2NoiseBlanker_Off;
  state->nb_freshness = RadioFieldFreshness_Unavailable;
}
