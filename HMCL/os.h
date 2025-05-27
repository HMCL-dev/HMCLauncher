#pragma once

#include <string>

#include <windows.h>

namespace hmcl {

enum Architecture {
  X86,
  X86_64,
  ARM64
};

std::wstring GetSelfPath();

Architecture GetArchitecture();

} // namespace hmcl
