#include "path.h"

void HLPath::AddBackslash() {
  if (!path.empty() && path.back() != '\\' && path.back() != '/') {
    path += '\\';
  }
}

void HLPath::operator+=(const std::string& append) {
  AddBackslash();
  path += append;
}

HLPath HLPath::operator+(const std::string& append) const {
  HLPath newPath = *this;
  newPath += append;
  return newPath;
}
