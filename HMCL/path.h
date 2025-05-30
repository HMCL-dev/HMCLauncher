#pragma once

#include <string>
#include <utility>

struct HLPath {
  std::wstring path;

  HLPath() = default;
  HLPath(const HLPath&) = default;
  explicit(false) HLPath(std::wstring path) : path(std::move(path)) {}
  explicit(false) HLPath(const wchar_t* path) : path(path) {}

  HLPath &operator=(const HLPath& other) = default;

  bool IsRegularFile() const;

  void AddBackslash();
  HLPath operator/(const std::wstring& append) const;
  void operator/=(const std::wstring& append);
};
