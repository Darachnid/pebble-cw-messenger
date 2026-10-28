#include "radio/simulated_radio.h"

#include <stdio.h>
#include <string.h>

void simulated_radio_apply(RadioState *state) {
  radio_state_init(state);

  state->connection = RadioConnection_Live;
  state->connection_freshness = RadioFieldFreshness_Valid;

  snprintf(state->frequency_text, sizeof(state->frequency_text), "14.060.00");
  state->frequency_freshness = RadioFieldFreshness_Valid;

  snprintf(state->mode_text, sizeof(state->mode_text), "CW");
  snprintf(state->band_text, sizeof(state->band_text), "20 m");
  state->mode_freshness = RadioFieldFreshness_Valid;

  state->crystal_bw_hz = 400;
  state->filter_freshness = RadioFieldFreshness_Valid;

  state->audio_filter = K2AudioFilter_AF1;
  state->audio_filter_freshness = RadioFieldFreshness_Valid;

  state->s_meter_segments = 7;
  snprintf(state->s_meter_label, sizeof(state->s_meter_label), "S7");
  state->s_meter_freshness = RadioFieldFreshness_Valid;

  state->power_segments = 4;
  snprintf(state->power_label, sizeof(state->power_label), "5W");
  state->power_freshness = RadioFieldFreshness_Valid;

  state->rit_enabled = true;
  state->rit_offset_cHz = 0;
  state->rit_freshness = RadioFieldFreshness_Valid;

  snprintf(state->agc_text, sizeof(state->agc_text), "SLOW");
  state->agc_freshness = RadioFieldFreshness_Valid;

  state->noise_blanker = K2NoiseBlanker_Off;
  state->nb_freshness = RadioFieldFreshness_Valid;
}
