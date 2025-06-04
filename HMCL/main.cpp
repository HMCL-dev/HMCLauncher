#include <windows.h>
#include <cstdlib>
#include <algorithm>
#include <format>
#include <ranges>

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

  // If HMCL_JAVA_HOME is set, it should always be used
  {
    const auto hmclJavaHome = HLGetEnvPath(L"HMCL_JAVA_HOME");
    if (hmclJavaHome.has_value() && !hmclJavaHome.value().path.empty()) {
      HLDebugLog(L"HMCL_JAVA_HOME: " + hmclJavaHome.value().path);
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
      HLDebugLogVerbose(L"HMCL_JAVA_HOME: Not Found");
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
      HLDebugLog(std::format(L"Bundled JRE: {}", javaExecutablePath.path));
      if (HLLaunchJVM(javaExecutablePath, options, std::nullopt)) {
        return EXIT_SUCCESS;
      }
    } else {
      HLDebugLogVerbose(std::format(L"Bundled JRE: Not Found"));
    }
  }

  // ------ Search All Java ------

  // To make the log look better, we first print JAVA_HOME
  const auto javaHome = HLGetEnvPath(L"JAVA_HOME");
  if (javaHome.has_value() && !javaHome.value().path.empty()) {
    HLDebugLog(L"JAVA_HOME: " + javaHome.value().path);
  } else {
    HLDebugLogVerbose(L"JAVA_HOME: Not Found");
  }

  HLJavaList javaRuntimes;
  {
    HLPath hmclJavaDir = options.workdir / L".hmcl\\java";
    if (isARM64) {
      hmclJavaDir /= L"windows-arm64";
    } else if (isX64) {
      hmclJavaDir /= L"windows-x86_64";
    } else {
      hmclJavaDir /= L"windows-x86";
    }
    HLSearchJavaInDir(javaRuntimes, hmclJavaDir, javaExecutableName);
  }

  if (javaHome.has_value() && !javaHome.value().path.empty()) {
    HLDebugLogVerbose(L"Checking JAVA_HOME");

    HLPath javaExecutablePath = javaHome.value() / L"bin" / javaExecutableName;
    if (javaExecutablePath.IsRegularFile()) {
      javaRuntimes.TryAdd(javaExecutablePath);
    } else {
      HLDebugLog(std::format(L"JAVA_HOME is set to {}, but the Java executable {} does not exist",
                             javaHome.value().path, javaExecutablePath.path));
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

  // Search Java in PATH
  {
    const auto paths = HLGetEnvVar(L"PATH");

    std::size_t pos = 0;
    while (pos < paths->size()) {
      auto end = paths->find(L';', pos);

      if (end == std::wstring::npos) {
        end = paths->size();
      }

      auto pathCount = end - pos;
      if (pathCount > 0) {  // Not empty
        HLPath path = paths->substr(pos, pathCount);
        path /= javaExecutableName;
        javaRuntimes.TryAdd(path);
      }

      pos = end + 1;
    }
  }

  // Search Java in C:\Program Files
  {
    std::optional<HLPath> programFilesPath;
    if (isX64 || isARM64) {
      programFilesPath = HLGetEnvPath(L"ProgramW6432");
    } else if (isX86) {
      programFilesPath = HLGetEnvPath(L"ProgramFiles");
    } else {
      programFilesPath = std::nullopt;
    }

    if (programFilesPath.has_value() && !programFilesPath.value().path.empty()) {
      HLSearchJavaInProgramFiles(javaRuntimes, programFilesPath.value(), javaExecutableName);
    } else {
      HLDebugLog(L"Failed to obtain the path to Program Files");
    }
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

    if (HLVerboseOutput) {
      std::wstring message = L"Found Java runtimes:";
      for (const auto &item : javaRuntimes.runtimes) {
        message += L"\n  - ";
        message += item.executablePath.path;
        message += L", Version ";
        message += item.version.ToWString();
      }

      HLDebugLog(message);
    }

    for (const auto &item : javaRuntimes.runtimes | std::views::reverse) {
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