#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace hmcl {

struct JavaRuntime {
  static std::vector<JavaRuntime> FindAll();

  std::uint32_t majorVersion;
  std::wstring path;
};

}  // namespace hmcl