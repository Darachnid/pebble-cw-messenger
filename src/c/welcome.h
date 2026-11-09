#pragma once

#include "config.h"

enum {
  CW_WELCOME_READY = 0,
  CW_WELCOME_SKIPPED = 1,
  CW_WELCOME_FINISHED = 2,
};

typedef void (*WelcomeDoneCallback)(int outcome);

/** First launch: multi-step guided tour. */
void welcome_push_if_needed(CwSettings *cfg, WelcomeDoneCallback on_done);

/** Help → replay tour (does not clear onboarding_done). */
void welcome_push_replay(WelcomeDoneCallback on_done);

bool welcome_is_active(void);
