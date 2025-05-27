#pragma once

#include <cstdint>
#include <cstdlib>
#include <compare>
#include <optional>
#include <string>

#include "path.h"

constexpr std::uint16_t HL_EXPECTED_JAVA_MAJOR_VERSION = 11;
constexpr std::uint16_t HL_LEGACY_JAVA_MAJOR_VERSION = 8;  // TODO: Support for Java 8 will be removed in the future

struct HLJavaVersion {
  static HLJavaVersion INVALID;

  static HLJavaVersion FromJavaExecutable(const HLPath &filePath);
  static HLJavaVersion FromString(const std::wstring &versionString);

  std::uint16_t major;
  std::uint16_t minor;
  std::uint16_t build;
  std::uint16_t revision;

  std::strong_ordering operator<=>(const HLJavaVersion &other) const = default;
};

struct HLJavaRuntime {
  HLJavaVersion version;
  HLPath executablePath;

  std::strong_ordering operator<=>(const HLJavaRuntime &other) const { return version <=> other.version; }
};

struct HLJavaOptions {
  std::wstring workdir;
  std::wstring jarPath;
  std::optional<std::wstring> jvmOptions;
};

bool HLLaunchJVM(const HLPath &javaExecutablePath, const HLJavaOptions &options,
                 const std::optional<HLJavaVersion> &version = std::nullopt);

inline void HLTryLaunchJVM(const HLPath &javaExecutablePath, const HLJavaOptions &options,
                           const std::optional<HLJavaVersion> &version = std::nullopt) {
  if (javaExecutablePath.IsRegularFile() && HLLaunchJVM(javaExecutablePath, options, version)) {
    exit(EXIT_SUCCESS);
  }
}

void HLSearchJavaInDir(std::vector<HLJavaRuntime> &result, const HLPath &basedir);

void HLSearchJavaInProgramFiles(std::vector<HLJavaRuntime> &result, const HLPath &programFiles);

void HLSearchJavaInRegistry(std::vector<HLJavaRuntime> &result, HKEY rootKey, LPCWSTR subKey);