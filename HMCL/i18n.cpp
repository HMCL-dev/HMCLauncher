#include <windows.h>

#include "i18n.h"

HLI18N HLI18N::Instance() {
  HLI18N i18n = {
      .errorSelfPath = L"Failed to get the exe path.",
      .errorJavaHomeNotExist =
          L"The Java specified by HMCL_JAVA_HOME does not exist or is invalid.\n"
          L"Please modify or delete the environment variable.",
  };

  const auto language = GetUserDefaultUILanguage();
  if (language == 2052) {  // zh-CN
    i18n.errorSelfPath = L"获取程序路径失败。";
    i18n.errorJavaHomeNotExist = L"HMCL_JAVA_HOME 指定的 Java 不存在或无效，请修改或删除此环境变量。";
  }
  return i18n;
}
