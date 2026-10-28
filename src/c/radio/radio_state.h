#pragma once

#include <stdbool.h>
#include <stdint.h>

/** Whether a displayed value can be trusted for UI emphasis. */
typedef enum {
  RadioFieldFreshness_Valid,
  RadioFieldFreshness_Stale,
  RadioFieldFreshness_Unavailable,
} RadioFieldFreshness;

typedef enum {
  RadioConnection_Offline,
  RadioConnection_Live,
  RadioConnection_Stale,
} RadioConnectionStatus;

typedef enum {
  K2AudioFilter_Off,
  K2AudioFilter_AF1,
  K2AudioFilter_AF2,
} K2AudioFilter;

typedef enum {
  K2NoiseBlanker_Off,
  K2NoiseBlanker_NB1,
  K2NoiseBlanker_NB2,
} K2NoiseBlanker;

/** Snapshot of K2 state for the dashboard (no I/O here). */
typedef struct {
  RadioConnectionStatus connection;
  RadioFieldFreshness connection_freshness;

  char frequency_text[16];
  RadioFieldFreshness frequency_freshness;

  char mode_text[8];
  char band_text[12];
  RadioFieldFreshness mode_freshness;

  /** Crystal filter bandwidth in Hz (for passband graphic scaling). */
  uint16_t crystal_bw_hz;
  RadioFieldFreshness filter_freshness;

  K2AudioFilter audio_filter;
  RadioFieldFreshness audio_filter_freshness;

  /** S-meter bar segments filled (0–10) and label (e.g. "S7"). */
  uint8_t s_meter_segments;
  char s_meter_label[8];
  RadioFieldFreshness s_meter_freshness;

  /** Forward power reading; display-only, never solicited via TX. */
  uint8_t power_segments;
  char power_label[8];
  RadioFieldFreshness power_freshness;

  bool rit_enabled;
  /** RIT offset in hundredths of a kHz (e.g. 12 → 0.12 kHz). */
  int16_t rit_offset_cHz;
  RadioFieldFreshness rit_freshness;

  char agc_text[12];
  RadioFieldFreshness agc_freshness;

  K2NoiseBlanker noise_blanker;
  RadioFieldFreshness nb_freshness;
} RadioState;

void radio_state_init(RadioState *state);
