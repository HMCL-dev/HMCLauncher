#include <objbase.h>

#include "path.h"
#include "platform.h"

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

HLArchitecture HLGetArchitecture() {
  // https://learn.microsoft.com/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process2
  auto fnIsWow64Process2 = reinterpret_cast<BOOL(WINAPI *)(HANDLE, PUSHORT, PUSHORT)>(
      GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "IsWow64Process2"));

  if (fnIsWow64Process2 != nullptr) {
    USHORT uProcessMachine = 0;
    USHORT uNativeMachine = 0;
    if (fnIsWow64Process2(GetCurrentProcess(), &uProcessMachine, &uNativeMachine)) {
      if (uNativeMachine == IMAGE_FILE_MACHINE_ARM64) {
        return HLArchitecture::ARM64;
      }

      if (uNativeMachine == IMAGE_FILE_MACHINE_AMD64) {
        return HLArchitecture::X86_64;
      }

      return HLArchitecture::X86;
    }
  }

  SYSTEM_INFO systemInfo;
  GetNativeSystemInfo(&systemInfo);

  if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
    return HLArchitecture::ARM64;
  }

  if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
    return HLArchitecture::X86_64;
  }

  return HLArchitecture::X86;
}

std::optional<std::pair<HLPath, std::wstring>> HLGetSelfPath() {
  DWORD res, size = MAX_PATH;
  std::wstring selfPath(size, L'\0');
  while ((res = GetModuleFileNameW(nullptr, &selfPath[0], size)) == size) {
    selfPath.resize(size += MAX_PATH);
  }
  if (res == 0) return std::nullopt;

  selfPath.resize(size - MAX_PATH + res);

  size_t last_slash = selfPath.find_last_of(L"/\\");
  if (last_slash != std::wstring::npos && last_slash + 1 < selfPath.size()) {
    return std::optional{std::pair{HLPath{selfPath.substr(0, last_slash)}, selfPath.substr(last_slash + 1)}};
  } else {
    return std::nullopt;
  }
}

std::optional<std::wstring> HLGetEnvVar(LPCWSTR name) {
  DWORD size = MAX_PATH;
  std::wstring out(size, L'\0');

  do {
    SetLastError(ERROR_SUCCESS);
    DWORD res = GetEnvironmentVariableW(name, &out[0], size);
    if (res == 0 && GetLastError() != ERROR_SUCCESS) {
      return std::nullopt;
    }

    if (res < size) {
      out.resize(res);
      return std::optional{out};
    }

    if (res == size) {
      // I think it's not possible, but I'm not really sure, so do something to avoid an infinite loop.
      size = res + 1;
    } else {
      size = res;
    }
    out.resize(size);
  } while (true);
}

std::optional<HLPath> HLGetEnvPath(LPCWSTR name) {
  auto res = HLGetEnvVar(name);
  if (res.has_value()) {
    return HLPath{res.value()};
  } else {
    return std::nullopt;
  }
}
