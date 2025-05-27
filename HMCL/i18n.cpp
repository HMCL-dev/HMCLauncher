#include <windows.h>

#include "i18n.h"

HLI18N HLI18N::Instance() {
  HLI18N i18n = {.errorSelfPath = L"Failed to get the exe path.",
                 .errorInvalidHMCLJavaHome =
                     L"\n"
                     "The Java path specified by HMCL_JAVA_HOME is invalid. Please update it to a valid Java "
                     "installation path or remove this environment variable.",
                 .errorJavaNotFound =
                     L"The Java runtime environment is required to run HMCL and Minecraft,\n"
                     L"Click 'OK' to start downloading java.\n"
                     L"Please restart HMCL after installing Java."};

  const auto language = GetUserDefaultUILanguage();
  if (language == 2052) {  // zh-CN
    i18n.errorSelfPath = L"获取程序路径失败。";
    i18n.errorInvalidHMCLJavaHome = L"HMCL_JAVA_HOME 所指向的 Java 路径无效，请更新或删除该变量。\n";
    i18n.errorJavaNotFound =
        L"运行 HMCL 以及 Minecraft 需要 Java 运行时环境，点击“确定”开始下载。\n"
        L"请在安装 Java 完成后重新启动 HMCL。";
  }
  return i18n;
}
