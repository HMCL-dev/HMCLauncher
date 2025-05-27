#pragma once

#include <windows.h>

namespace hmcl {

struct I18N {
  static I18N Instance();

  LPCWSTR errorSelfPath;
};

}  // namespace hmcl