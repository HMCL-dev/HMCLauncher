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
  LPCWSTR javaExecutableName;
  if (HLAttachConsole()) {
    javaExecutableName = L"java.exe";
  } else {
    javaExecutableName = L"javaw.exe";
  }

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
  HLDebugLog(std::format(L"Working directory: {}", options.workdir.path));
  HLDebugLog(std::format(L"Exe File: {}\\{}", options.workdir.path, options.jarPath));
  if (options.jvmOptions.has_value()) {
    HLDebugLog(std::format(L"JVM Options: {}", options.jvmOptions.value()));
  } else {
    HLDebugLog(L"JVM Options: Default");
  }

  const auto hmclCurrentDir = options.workdir + L".hmcl";

  const auto arch = HLGetArchitecture();
  const bool isX64 = arch == HLArchitecture::X86_64;
  const bool isARM64 = arch == HLArchitecture::ARM64;
  const bool isX86 = arch == HLArchitecture::X86;

  // ------ Find Java ------

  // If HMCL_JAVA_HOME is set, it should always be used
  {
    const auto hmclJavaHome = HLGetEnvPath(L"HMCL_JAVA_HOME");
    if (hmclJavaHome.has_value() && !hmclJavaHome.value().path.empty()) {
      HLDebugLog(std::format(L"HMCL_JAVA_HOME: {}", hmclJavaHome.value().path));
      HLPath javaExecutablePath = hmclJavaHome.value() + L"bin" + javaExecutableName;
      if (javaExecutablePath.IsRegularFile() && HLLaunchJVM(javaExecutablePath, options, std::nullopt)) {
        exit(EXIT_SUCCESS);
      }
      MessageBoxW(nullptr, i18n.errorInvalidHMCLJavaHome, nullptr, MB_OK | MB_ICONERROR);
      return EXIT_FAILURE;
    }
  }

  // Try the Java packaged together.
  {
    HLPath javaExecutablePath = options.workdir;
    if (isARM64) {
      javaExecutablePath += L"jre-arm64\\bin";
    } else if (isX64) {
      javaExecutablePath += L"jre-x64\\bin";
    } else {
      javaExecutablePath += L"jre-x86\\bin";
    }
    javaExecutablePath += javaExecutableName;
    if (javaExecutablePath.IsRegularFile()) {
      if (HLLaunchJVM(javaExecutablePath, options, std::nullopt)) {
        return EXIT_SUCCESS;
      }
    }
  }

  HLJavaList javaRuntimes;

  {
    HLPath hmclJavaDir = hmclCurrentDir + L"java";
    if (isARM64) {
      hmclJavaDir += L"windows-arm64";
    } else if (isX64) {
      hmclJavaDir += L"windows-x86_64";
    } else {
      hmclJavaDir += L"windows-x86";
    }
    HLSearchJavaInDir(javaRuntimes, hmclJavaDir, javaExecutableName);
  }

  {
    const auto javaHome = HLGetEnvPath(L"JAVA_HOME");
    if (javaHome.has_value() && !javaHome.value().path.empty()) {
      HLPath javaExecutablePath = javaHome.value() + L"bin" + javaExecutableName;
      auto version = HLJavaVersion::FromJavaExecutable(javaExecutablePath);
      if (version.major >= HL_EXPECTED_JAVA_MAJOR_VERSION) {
        if (javaExecutablePath.IsRegularFile() && HLLaunchJVM(javaExecutablePath, options, version)) {
          exit(EXIT_SUCCESS);
        }
      } else if (version.major == HL_LEGACY_JAVA_MAJOR_VERSION) {
        // Add it to the fallback list, to be tried only when no other Java is available
        javaRuntimes.Add(HLJavaRuntime{.version = version, .executablePath = javaExecutablePath});
      }
    }
  }

  {
    const auto appDataPath = HLGetEnvPath(L"APPDATA");
    if (appDataPath.has_value() && !appDataPath.value().path.empty()) {
      HLPath hmclJavaDir = appDataPath.value() + L".hmcl\\java";
      if (isARM64) {
        hmclJavaDir += L"windows-arm64";
      } else if (isX64) {
        hmclJavaDir += L"windows-x86_64";
      } else {
        hmclJavaDir += L"windows-x86";
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
  }

  // Search Java in registry

  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\JDK", javaExecutableName);
  HLSearchJavaInRegistry(javaRuntimes, HKEY_LOCAL_MACHINE, L"SOFTWARE\\JavaSoft\\JRE", javaExecutableName);

  // Try to launch JVM

  if (javaRuntimes.runtimes.empty()) {
    HLDebugLog(L"No Java runtime found.");
  } else {
    std::stable_sort(javaRuntimes.runtimes.begin(), javaRuntimes.runtimes.end());

    std::wstring message = L"Found the following Java runtimes:";
    for (const auto &item : javaRuntimes.runtimes) {
      message += std::format(L"\n  Java {}.{}.{}.{}: {}", item.version.major, item.version.minor, item.version.build,
                             item.version.revision, item.executablePath.path);
    }

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