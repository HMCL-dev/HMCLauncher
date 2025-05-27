#pragma once

#include <string>
#include <optional>

#include <windows.h>

enum HLArchitecture {
  X86,
  X86_64,
  ARM64
};

std::optional<std::pair<std::wstring, std::wstring>> HLGetSelfPath();

std::optional<std::wstring> HLGetEnvVar(LPCWSTR name);

HLArchitecture HLArchitecture();
