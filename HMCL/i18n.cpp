#include <windows.h>

#include "i18n.h"

namespace hmcl {

I18N I18N::Instance() {
  I18N i18n = {};
  i18n.errorSelfPath = L"Failed to get the exe path.";

  auto language = GetUserDefaultUILanguage();
  if (language == 2052) {  // zh-CN
    i18n.errorSelfPath = L"获取程序路径失败。";
  }
  return i18n;
}

}  // namespace hmcl
