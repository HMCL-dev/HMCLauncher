#pragma once

#include <string>
#include <optional>
#include <windows.h>

#include "path.h"

enum HLArchitecture { X86, X86_64, ARM64 };

HLArchitecture HLGetArchitecture();

std::optional<std::pair<std::wstring, std::wstring>> HLGetSelfPath();

std::optional<std::wstring> HLGetEnvVar(LPCWSTR name);

std::optional<HLPath> HLGetEnvPath(LPCWSTR name);