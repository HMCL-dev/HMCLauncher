#pragma once

#include "path.h"

bool HLAttachConsole(bool force = false);

void HLStartDebugLogger(const HLPath &hmclCurrentDir);

void HLDebugLog(const std::wstring &message);