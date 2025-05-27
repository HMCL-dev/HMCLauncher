#pragma once

#include <string>
#include <utility>

struct HLPath {
  std::string path;

  HLPath() = default;
  HLPath(std::string path) : path(std::move(path)) {}

  void AddBackslash();

  void operator+=(const std::string& append);

  HLPath operator+(const std::string& append) const;
};
