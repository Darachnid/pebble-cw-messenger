#include "transport/phone_bridge.h"

#include "message_keys.auto.h"
#include "radio/radio_state.h"

#include <stdio.h>
#include <string.h>

static Dashboard *s_dashboard;

static RadioFieldFreshness prv_freshness_for_connection(RadioConnectionStatus conn) {
  switch (conn) {
    case RadioConnection_Live:
      return RadioFieldFreshness_Valid;
    case RadioConnection_Stale:
      return RadioFieldFreshness_Stale;
    case RadioConnection_Offline:
    default:
      return RadioFieldFreshness_Unavailable;
  }
}

static void prv_copy_string(char *dest, size_t dest_len, const char *src) {
  if (!dest || dest_len == 0) {
    return;
  }
  if (!src) {
    dest[0] = '\0';
    return;
  }
  snprintf(dest, dest_len, "%s", src);
}

static void prv_apply_tuple_string(Tuple *tuple, char *dest, size_t dest_len) {
  if (!tuple || tuple->type != TUPLE_CSTRING) {
    return;
  }
  prv_copy_string(dest, dest_len, tuple->value->cstring);
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  (void)context;
  if (!s_dashboard) {
    return;
  }

  RadioState state;
  radio_state_init(&state);

  Tuple *conn = dict_find(iter, MESSAGE_KEY_conn);
  if (conn && conn->type == TUPLE_INT) {
    switch (conn->value->int32) {
      case 1:
        state.connection = RadioConnection_Live;
        break;
      case 2:
        state.connection = RadioConnection_Stale;
        break;
      default:
        state.connection = RadioConnection_Offline;
        break;
    }
  }
  state.connection_freshness = prv_freshness_for_connection(state.connection);
  RadioFieldFreshness field_fresh = state.connection == RadioConnection_Live
                                        ? RadioFieldFreshness_Valid
                                        : (state.connection == RadioConnection_Stale
                                               ? RadioFieldFreshness_Stale
                                               : RadioFieldFreshness_Unavailable);

  prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_freq), state.frequency_text,
                         sizeof(state.frequency_text));
  state.frequency_freshness = field_fresh;

  prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_mode), state.mode_text, sizeof(state.mode_text));
  prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_band), state.band_text, sizeof(state.band_text));
  state.mode_freshness = field_fresh;

  Tuple *xtal = dict_find(iter, MESSAGE_KEY_xtal_bw);
  if (xtal && xtal->type == TUPLE_INT) {
    state.crystal_bw_hz = (uint16_t)xtal->value->int32;
    state.filter_freshness = field_fresh;
  }

  Tuple *audio_f = dict_find(iter, MESSAGE_KEY_audio_f);
  if (audio_f && audio_f->type == TUPLE_INT) {
    switch (audio_f->value->int32) {
      case 1:
        state.audio_filter = K2AudioFilter_AF1;
        break;
      case 2:
        state.audio_filter = K2AudioFilter_AF2;
        break;
      default:
        state.audio_filter = K2AudioFilter_Off;
        break;
    }
    state.audio_filter_freshness = field_fresh;
  }

  Tuple *s_seg = dict_find(iter, MESSAGE_KEY_s_seg);
  if (s_seg && s_seg->type == TUPLE_INT) {
    int seg = s_seg->value->int32;
    if (seg < 0) {
      seg = 0;
    }
    if (seg > 10) {
      seg = 10;
    }
    state.s_meter_segments = (uint8_t)seg;
    state.s_meter_freshness = field_fresh;
  }
  prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_s_lbl), state.s_meter_label,
                         sizeof(state.s_meter_label));
  if (!state.s_meter_label[0] && state.s_meter_freshness != RadioFieldFreshness_Unavailable) {
    snprintf(state.s_meter_label, sizeof(state.s_meter_label), "S%d", state.s_meter_segments);
  }

  Tuple *pwr_valid = dict_find(iter, MESSAGE_KEY_pwr_valid);
  Tuple *pwr_seg = dict_find(iter, MESSAGE_KEY_pwr_seg);
  if (pwr_valid && pwr_valid->type == TUPLE_INT && pwr_valid->value->int32) {
    int pseg = (pwr_seg && pwr_seg->type == TUPLE_INT) ? pwr_seg->value->int32 : 0;
    if (pseg < 0) {
      pseg = 0;
    }
    if (pseg > 10) {
      pseg = 10;
    }
    state.power_segments = (uint8_t)pseg;
    state.power_freshness = field_fresh;
    prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_pwr_lbl), state.power_label,
                           sizeof(state.power_label));
    if (!state.power_label[0]) {
      snprintf(state.power_label, sizeof(state.power_label), "%d seg", pseg);
    }
  }

  Tuple *rit_on = dict_find(iter, MESSAGE_KEY_rit_on);
  if (rit_on && rit_on->type == TUPLE_INT) {
    state.rit_enabled = rit_on->value->int32 != 0;
    state.rit_freshness = field_fresh;
  }
  Tuple *rit_off = dict_find(iter, MESSAGE_KEY_rit_off);
  if (rit_off && rit_off->type == TUPLE_INT) {
    state.rit_offset_cHz = (int16_t)rit_off->value->int32;
  }

  prv_apply_tuple_string(dict_find(iter, MESSAGE_KEY_agc), state.agc_text, sizeof(state.agc_text));
  state.agc_freshness = field_fresh;

  Tuple *nb = dict_find(iter, MESSAGE_KEY_nb);
  if (nb && nb->type == TUPLE_INT) {
    switch (nb->value->int32) {
      case 1:
        state.noise_blanker = K2NoiseBlanker_NB1;
        break;
      case 2:
        state.noise_blanker = K2NoiseBlanker_NB2;
        break;
      default:
        state.noise_blanker = K2NoiseBlanker_Off;
        break;
    }
    state.nb_freshness = field_fresh;
  }

  dashboard_set_state(s_dashboard, &state);
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  (void)reason;
  (void)context;
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  (void)iter;
  (void)reason;
  (void)context;
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {
  (void)iter;
  (void)context;
}

void phone_bridge_init(Dashboard *dashboard) {
  s_dashboard = dashboard;

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);

  const uint32_t inbox = app_message_inbox_size_maximum();
  const uint32_t outbox = app_message_outbox_size_maximum();
  const AppMessageResult open_result = app_message_open(inbox, outbox);
  if (open_result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open failed: %d", (int)open_result);
  }
}
