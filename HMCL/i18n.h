#pragma once

#include <windows.h>

struct HLI18N {
  static HLI18N Instance();

  // Error Messages
  LPCWSTR errorSelfPath;
  LPCWSTR errorInvalidHMCLJavaHome;
  LPCWSTR errorJavaNotFound;
};
