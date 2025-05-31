#pragma once

#include "path.h"

extern bool HLVerboseOutput;

bool HLAttachConsole(bool force = false);

void HLStartDebugLogger(const HLPath &hmclCurrentDir);

void HLDebugLog(const std::wstring &message);

#define HLDebugLogVerbose(message) \
  do {                             \
    if (HLVerboseOutput) {         \
      HLDebugLog(message);         \
    }                              \
  } while (0)