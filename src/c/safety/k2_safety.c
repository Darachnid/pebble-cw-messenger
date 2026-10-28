#include "safety/k2_safety.h"

#include <ctype.h>
#include <string.h>

static bool prv_chars_equal_ignore_case(char a, char b) {
  if (a >= 'a' && a <= 'z') {
    a = (char)(a - ('a' - 'A'));
  }
  if (b >= 'a' && b <= 'z') {
    b = (char)(b - ('a' - 'A'));
  }
  return a == b;
}

static bool prv_token_equals_lit(const char *token, size_t len, const char *lit) {
  size_t lit_len = strlen(lit);
  if (len != lit_len) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    if (!prv_chars_equal_ignore_case(token[i], lit[i])) {
      return false;
    }
  }
  return true;
}

static bool prv_is_transmit_related_token(const char *token, size_t len) {
  if (len == 0) {
    return false;
  }

  static const char *const blocked[] = {"TX", "KY", "RX", "SW"};

  for (size_t i = 0; i < sizeof(blocked) / sizeof(blocked[0]); i++) {
    if (prv_token_equals_lit(token, len, blocked[i])) {
      return true;
    }
  }

  if (len == 2 && token[0] == 'K' && isdigit((unsigned char)token[1])) {
    return true;
  }

  return false;
}

bool k2_safety_validate_command_bytes(const char *command, size_t length,
                                      K2SafetyDenyReason *reason_out) {
  if (reason_out) {
    *reason_out = K2SafetyDenyReason_None;
  }
  if (!command || length == 0) {
    if (reason_out) {
      *reason_out = K2SafetyDenyReason_NotOnAllowlist;
    }
    return false;
  }

  char token[8];
  size_t t = 0;
  for (size_t i = 0; i <= length; i++) {
    char c = (i < length) ? command[i] : ';';
    if (c == ';' || c == '\0') {
      if (t > 0) {
        if (prv_is_transmit_related_token(token, t)) {
          if (reason_out) {
            *reason_out = K2SafetyDenyReason_TransmitRelated;
          }
          return false;
        }
        t = 0;
      }
      continue;
    }
    if (t + 1 < sizeof(token)) {
      token[t++] = c;
    }
  }

  return true;
}

bool k2_safety_command_allowed(K2SafeCommand command, bool radio_in_rx, bool rx_known,
                               K2SafetyDenyReason *reason_out) {
  if (reason_out) {
    *reason_out = K2SafetyDenyReason_None;
  }

  switch (command) {
    case K2SafeCommand_FA:
    case K2SafeCommand_FB:
    case K2SafeCommand_IF:
    case K2SafeCommand_FW:
    case K2SafeCommand_SM:
    case K2SafeCommand_BG:
    case K2SafeCommand_GT:
    case K2SafeCommand_NB:
    case K2SafeCommand_TQ:
    case K2SafeCommand_DS:
      return true;
    default:
      if (reason_out) {
        *reason_out = K2SafetyDenyReason_NotOnAllowlist;
      }
      return false;
  }

  (void)radio_in_rx;
  (void)rx_known;
}
