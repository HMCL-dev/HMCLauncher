#include <windows.h>
#include <cstdlib>
#include <algorithm>
#include <format>

#include <config.h>

#include "debug.h"
#include "i18n.h"
#include "path.h"
#include "platform.h"
#include "java.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
  HLVerboseOutput = HLGetEnvVar(L"HMCL_LAUNCHER_VERBOSE_OUTPUT").value_or(L"") == L"true";

  LPCWSTR javaExecutableName;
  if (HLAttachConsole()) {
    javaExecutableName = L"java.exe";
  } else {
    javaExecutableName = L"javaw.exe";
  }

  const auto arch = HLGetArchitecture();
  const bool isX64 = arch == HLArchitecture::X86_64;
  const bool isARM64 = arch == HLArchitecture::ARM64;
  const bool isX86 = arch == HLArchitecture::X86;

  const auto i18n = HLI18N::Instance();
  const auto selfPath = HLGetSelfPath();
  if (!selfPath.has_value()) {
    HLDebugLog(L"Failed to get self path");
    MessageBoxW(nullptr, i18n.errorSelfPath, nullptr, MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }

  const HLJavaOptions options = {.workdir = selfPath.value().first,
                                 .jarPath = selfPath.value().second,
                                 .jvmOptions = HLGetEnvVar(L"HMCL_JAVA_OPTS")};
  HLDebugLog(std::format(L"*** HMCL Launcher {} ***", HMCL_LAUNCHER_VERSION));
  if (isX64) {
    HLDebugLog(L"System Architecture: x86-64");
  } else if (isARM64) {
    HLDebugLog(L"System Architecture: arm64");
  } else {
    HLDebugLog(L"System Architecture: x86");
  }

  HLDebugLog(std::format(L"Working directory: {}", options.workdir.path));
  HLDebugLog(std::format(L"Exe File: {}\\{}", options.workdir.path, options.jarPath));
  if (options.jvmOptions.has_value()) {
    HLDebugLog(std::format(L"JVM Options: {}", options.jvmOptions.value()));
  }

  const auto hmclCurrentDir = options.workdir / L".hmcl";

  // ------ Find Java ------

  // If HMCL_JAVA_HOME is set, it should always be used
  {
    const auto hmclJavaHome = HLGetEnvPath(L"HMCL_JAVA_HOME");
    if (hmclJavaHome.has_value() && !hmclJavaHome.value().path.empty()) {
      HLDebugLogVerbose(std::format(L"HMCL_JAVA_HOME is set to {}", hmclJavaHome.value().path));
      HLPath javaExecutablePath = hmclJavaHome.value() / L"bin" / javaExecutableName;
      if (javaExecutablePath.IsRegularFile()) {
        if (HLLaunchJVM(javaExecutablePath, options, std::nullopt)) {
          return EXIT_SUCCESS;
        }
      } else {
        HLDebugLog(std::format(L"Invalid HMCL_JAVA_HOME: {}", hmclJavaHome.value().path));
      }
      MessageBoxW(nullptr, i18n.errorInvalidHMCLJavaHome, nullptr, MB_OK | MB_ICONERROR);
      return EXIT_FAILURE;
    } else {
      HLDebugLogVerbose(L"HMCL_JAVA_HOME is not set");
    }
  }

  // Try the Java packaged together.
  {
    HLPath javaExecutablePath = options.workdir;
    if (isARM64) {
      javaExecutablePath /= L"jre-arm64";
    } else if (isX64) {
      javaExecutablePath /= L"jre-x64";
    } else {
      javaExecutablePath /= L"jre-x86";
    }
    javaExecutablePath /= L"bin";
    javaExecutablePath /= javaExecutableName;
    if (javaExecutablePath.IsRegularFile()) {
      HLDebugLogVerbose(std::format(L"Bundled JRE found: {}", javaExecutablePath.path));
      if (HLLaunchJVM(javaExecutablePath, options, std::nullopt)) {
        return EXIT_SUCCESS;
      }
    } else {
      HLDebugLogVerbose(std::format(L"Bundled JRE not found"));
    }
  }

  HLJavaList javaRuntimes;
  {
    HLPath hmclJavaDir = hmclCurrentDir / L"java";
    if (isARM64) {
      hmclJavaDir /= L"windows-arm64";
    } else if (isX64) {
      hmclJavaDir /= L"windows-x86_64";
    } else {
      hmclJavaDir /= L"windows-x86";
    }
    HLSearchJavaInDir(javaRuntimes, hmclJavaDir, javaExecutableName);
  }

  {
    const auto javaHome = HLGetEnvPath(L"JAVA_HOME");
    if (javaHome.has_value() && !javaHome.value().path.empty()) {
      HLPath javaExecutablePath = javaHome.value() / L"bin" / javaExecutableName;
      if (javaExecutablePath.IsRegularFile()) {
        javaRuntimes.AddIfAcceptable(javaExecutablePath);
      } else {
        HLDebugLog(std::format(L"JAVA_HOME is set to {}, but the Java executable {} does not exist",
                               javaHome.value().path, javaExecutablePath.path));
      }
    }
  }

  {
    const auto appDataPath = HLGetEnvPath(L"APPDATA");
    if (appDataPath.has_value() && !appDataPath.value().path.empty()) {
      HLPath hmclJavaDir = appDataPath.value() / L".hmcl\\java";
      if (isARM64) {
        hmclJavaDir /= L"windows-arm64";
      } else if (isX64) {
        hmclJavaDir /= L"windows-x86_64";
      } else {
        hmclJavaDir /= L"windows-x86";
      }
      HLSearchJavaInDir(javaRuntimes, hmclJavaDir, javaExecutableName);
    }
  }

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
    HLSearchJavaInProgramFiles(javaRuntimes, programFilesPath.value(), javaExecutableName);
  } else {
    HLDebugLogVerbose(L"Program Files path is not set, skipping search in Program Files");
  }

  // Search Java in registry
  HLSearchJavaInRegistry(javaRuntimes, L"SOFTWARE\\JavaSoft\\JDK", javaExecutableName);
  HLSearchJavaInRegistry(javaRuntimes, L"SOFTWARE\\JavaSoft\\JRE", javaExecutableName);

  // TODO: They are for Java 8 or earlier and will be removed in the future.
  HLSearchJavaInRegistry(javaRuntimes, L"SOFTWARE\\JavaSoft\\Java Development Kit", javaExecutableName);
  HLSearchJavaInRegistry(javaRuntimes, L"SOFTWARE\\JavaSoft\\Java Runtime Environment", javaExecutableName);

  // Try to launch JVM

  if (javaRuntimes.runtimes.empty()) {
    HLDebugLog(L"No Java runtime found.");
  } else {
    std::stable_sort(javaRuntimes.runtimes.begin(), javaRuntimes.runtimes.end());
    for (const auto &item : javaRuntimes.runtimes) {
      if (HLLaunchJVM(item.executablePath, options, item.version)) {
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
    ShellExecuteW(nullptr, nullptr, downloadLink, nullptr, nullptr, SW_SHOW);
  }
  return EXIT_FAILURE;
}