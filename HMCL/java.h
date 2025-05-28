#pragma once

#include <cstdint>
#include <cstdlib>
#include <compare>
#include <optional>
#include <string>
#include <unordered_set>

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

  [[nodiscard]] bool IsAcceptable() const {
    return major >= HL_EXPECTED_JAVA_MAJOR_VERSION || major == HL_LEGACY_JAVA_MAJOR_VERSION;
  }

  [[nodiscard]] std::wstring ToWString() const {
    if (major != 0) {
      return std::format(L"{}.{}.{}.{}", major, minor, build, revision);
    } else {
      return L"Unknown";
    }
  }
};

struct HLJavaRuntime {
  HLJavaVersion version;
  HLPath executablePath;

  std::strong_ordering operator<=>(const HLJavaRuntime &other) const { return version <=> other.version; }
};

struct HLJavaOptions {
  HLPath workdir;
  std::wstring jarPath;
  std::optional<std::wstring> jvmOptions;
};

struct HLJavaList {
  std::vector<HLJavaRuntime> runtimes;
  std::unordered_set<std::wstring> paths;

  bool TryAdd(const HLPath &javaExecutable);
};

bool HLLaunchJVM(const HLPath &javaExecutablePath, const HLJavaOptions &options,
                 const std::optional<HLJavaVersion> &version = std::nullopt);

void HLSearchJavaInDir(HLJavaList &result, const HLPath &basedir, LPCWSTR javaExecutableName);

void HLSearchJavaInProgramFiles(HLJavaList &result, const HLPath &programFiles, LPCWSTR javaExecutableName);

void HLSearchJavaInRegistry(HLJavaList &result, LPCWSTR subKey, LPCWSTR javaExecutableName);