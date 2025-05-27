#include <windows.h>
#include <cstdlib>
#include <algorithm>
#include <vector>

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

  const auto arch = HLGetArchitecture();
  const bool isX64 = arch == HLArchitecture::X86_64;
  const bool isARM64 = arch == HLArchitecture::ARM64;
  const bool isX86 = arch == HLArchitecture::X86;

  // ------ Find Java ------

  // If HMCL_JAVA_HOME is set, it should always be used
  const auto hmclJavaHome = HLGetEnvPath(L"HMCL_JAVA_HOME");
  if (hmclJavaHome.has_value() && !hmclJavaHome.value().path.empty()) {
    HLPath javaExecutablePath = hmclJavaHome.value() + L"bin\\javaw.exe";
    HLTryLaunchJVM(javaExecutablePath, options);
    MessageBoxW(nullptr, i18n.errorInvalidHMCLJavaHome, nullptr, MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }

  // Try the Java packaged together.
  {
    HLPath javaExecutablePath;
    if (isARM64) {
      javaExecutablePath = L"jre-arm64\\bin\\javaw.exe";
      HLTryLaunchJVM(javaExecutablePath, options);
    }

    if (isARM64 || isX64) {
      javaExecutablePath = L"jre-x64\\bin\\javaw.exe";
      HLTryLaunchJVM(javaExecutablePath, options);
    }

    javaExecutablePath = L"jre-x86\\bin\\javaw.exe";
    HLTryLaunchJVM(javaExecutablePath, options);
  }

  std::vector<HLJavaRuntime> javaRuntimes{};

  // Search Java in C:\Program Files

  std::optional<HLPath> programFilesPath;
  if (isX64 || isARM64) {
    programFilesPath = HLGetEnvPath(L"ProgramW6432");
  } else if (isX86) {
    programFilesPath = HLGetEnvPath(L"ProgramFiles(x86)");
  } else {
    programFilesPath = std::nullopt;
  }

  if (programFilesPath.has_value() && !programFilesPath.value().path.empty()) {
    HLSearchJavaInProgramFiles(javaRuntimes, programFilesPath.value());
  }

  // Search Java in registry

  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\JDK");
  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\JRE");

  // TODO: They are only used in Java 8 or earlier, so they should be removed in the future
  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Development Kit");
  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\Java Runtime Environment");

  // Try to launch JVM

  if (!javaRuntimes.empty()) {
    std::stable_sort(javaRuntimes.begin(), javaRuntimes.end());
    for (const auto &item : javaRuntimes) {
      if (HLLaunchJVM(item.executablePath, options)) {
        return EXIT_SUCCESS;
      }
    }
  }

  LPCWSTR downloadLink;

  if (isARM64) {
    downloadLink = L"https://docs.hmcl.net/downloads/windows/arm64.html";
  } else if (isX64) {
    downloadLink = L"https://docs.hmcl.net/downloads/windows/x86_64.html";
  } else {
    downloadLink = L"https://docs.hmcl.net/downloads/windows/x86.html";
  }

  if (MessageBoxW(nullptr, i18n.errorJavaNotFound, nullptr, MB_ICONWARNING | MB_OKCANCEL) == IDOK) {
    ShellExecuteW(0, 0, downloadLink, 0, 0, SW_SHOW);
  }
  return EXIT_FAILURE;
}