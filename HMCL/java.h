#pragma once

#include <cstdint>
#include <compare>
#include <optional>
#include <string>

struct HLJavaVersion {
  static HLJavaVersion INVALID;

  static HLJavaVersion fromJavaExecutable(const std::wstring &filePath);

  const std::uint16_t major;
  const std::uint16_t minor;
  const std::uint16_t build;
  const std::uint16_t revision;

  std::strong_ordering operator<=>(const HLJavaVersion &other) const = default;
};

struct HLJavaOptions {
  std::wstring workdir;
  std::wstring jarPath;
  std::optional<std::wstring> jvmOptions;
};

bool HLLaunchJVM(const std::wstring &javaExecutablePath, const HLJavaOptions &options,
               const std::optional<HLJavaVersion> &version = std::nullopt);