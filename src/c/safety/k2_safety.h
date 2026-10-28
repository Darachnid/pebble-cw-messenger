#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * Receive-only safety gate for all KIO2 traffic (Milestone 3+).
 * UI and configuration must never send commands around this layer.
 */

typedef enum {
  K2SafeCommand_FA,
  K2SafeCommand_FB,
  K2SafeCommand_IF,
  K2SafeCommand_FW,
  K2SafeCommand_SM,
  K2SafeCommand_BG,
  K2SafeCommand_GT,
  K2SafeCommand_NB,
  K2SafeCommand_TQ,
  K2SafeCommand_DS,
  K2SafeCommand_Count,
} K2SafeCommand;

typedef enum {
  K2SafetyDenyReason_None,
  K2SafetyDenyReason_NotOnAllowlist,
  K2SafetyDenyReason_TransmitRelated,
  K2SafetyDenyReason_TxStateUnknown,
  K2SafetyDenyReason_RadioTransmitting,
  K2SafetyDenyReason_StateChangingWhileUnsafe,
} K2SafetyDenyReason;

bool k2_safety_command_allowed(K2SafeCommand command, bool radio_in_rx, bool rx_known,
                               K2SafetyDenyReason *reason_out);

bool k2_safety_validate_command_bytes(const char *command, size_t length,
                                      K2SafetyDenyReason *reason_out);
