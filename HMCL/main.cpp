#include <windows.h>
#include <shlwapi.h>
#include <cstdlib>

#include "i18n.h"
#include "path.h"
#include "platform.h"
#include "java.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
  const auto i18n = HLI18N::Instance();

  const auto selfPath = HLGetSelfPath();
  if (!selfPath.has_value()) {
    MessageBoxW(nullptr, i18n.errorSelfPath, nullptr, MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }

  HLJavaOptions options = {.workdir = selfPath.value().first,
                               .jarPath = selfPath.value().second,
                               .jvmOptions = HLGetEnvVar(L"HMCL_JAVA_OPTS")};

  const auto arch = HLArchitecture();
  const bool isX64 = arch == HLArchitecture::X86_64;
  const bool isARM64 = arch == HLArchitecture::ARM64;
  const bool isX86 = !isX64 && !isARM64;

  // Find Java
  const auto hmclJavaHome = HLGetEnvVar(L"HMCL_JAVA_HOME");
  if (hmclJavaHome.has_value() && !hmclJavaHome->empty()) {
    std::wstring javaExecutablePath = hmclJavaHome.value();
    if (!javaExecutablePath.ends_with('\\') && !javaExecutablePath.ends_with('/')) {
      javaExecutablePath += L'\\';
    }
    javaExecutablePath += L"bin\\java.exe";

    if (!PathFileExistsW(javaExecutablePath.c_str())) {
      MessageBoxW(nullptr, i18n.errorJavaHomeNotExist, nullptr, MB_OK | MB_ICONERROR);
      return EXIT_FAILURE;
    }

    if (HLLaunchJVM(javaExecutablePath, options)) {
      return EXIT_SUCCESS;
    } else {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}