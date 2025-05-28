#include <windows.h>

#include "debug.h"

static HANDLE debugLoggerHandle = nullptr;
static bool consoleAllocated = false;

bool HLAttachConsole(bool force) {
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    wprintf(L"\n");
    consoleAllocated = true;
    return true;
  } else {
    return false;
  }
}

void HLStartDebugLogger(const HLPath &hmclCurrentDir) {
  // TODO: Make directories if not exist
  for (int i = 0; i < 9; ++i) {
    HLPath path = hmclCurrentDir + L"logs\\hmclauncher.log";
    if (i > 0) {
      path.path.push_back(L'.');
      path.path.push_back(static_cast<wchar_t>(L'0' + i));
    }

    auto handle = CreateFileW(path.path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);

    if (handle != INVALID_HANDLE_VALUE) {
      debugLoggerHandle = handle;
      return;
    }
  }
}

void HLDebugLog(const std::wstring &message) {
  SYSTEMTIME time;
  GetLocalTime(&time);
  wprintf(L"[%02d:%02d:%02d] [HMCLauncher] %s\n", time.wHour, time.wMinute, time.wSecond, message.c_str());
}