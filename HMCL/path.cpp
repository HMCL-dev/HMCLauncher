#include <shlwapi.h>

#include "path.h"

void HLPath::AddBackslash() {
  if (!path.empty() && path.back() != '\\' && path.back() != '/') {
    path += '\\';
  }
}

void HLPath::operator+=(const std::wstring& append) {
  AddBackslash();
}

HLPath HLPath::operator+(const std::wstring& append) const {
  HLPath newPath = *this;
  newPath += append;
  return newPath;
}

bool HLPath::IsRegularFile() const {
  DWORD attributes = GetFileAttributesW(path.c_str());

  return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY) &&
         !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}
