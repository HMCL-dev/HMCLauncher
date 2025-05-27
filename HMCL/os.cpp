#include <Objbase.h>

#include "os.h"

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

namespace hmcl {

std::wstring GetSelfPath() {
  DWORD res, size = MAX_PATH;
  std::wstring out;
  out.clear();
  out.resize(size);
  while ((res = GetModuleFileNameW(nullptr, &out[0], size)) == size) {
    out.resize(size += MAX_PATH);
  }
  if (res != 0) {
    out.resize(size - MAX_PATH + res);
  } else {
    out.resize(0);
  }
  return out;
}

Architecture GetArchitecture() {
  // https://learn.microsoft.com/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process2
  auto fnIsWow64Process2 =
      reinterpret_cast<BOOL(WINAPI *)(HANDLE, PUSHORT, PUSHORT)>(
          GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "IsWow64Process2"));

  if (fnIsWow64Process2 != nullptr) {
    USHORT uProcessMachine = 0;
    USHORT uNativeMachine = 0;
    if (fnIsWow64Process2(GetCurrentProcess(), &uProcessMachine,
                          &uNativeMachine)) {
      if (uNativeMachine == IMAGE_FILE_MACHINE_ARM64) {
        return Architecture::ARM64;
      }

      if (uNativeMachine == IMAGE_FILE_MACHINE_AMD64) {
        return Architecture::X86_64;
      }

      return Architecture::X86;
    }
  }

  SYSTEM_INFO systemInfo;
  GetNativeSystemInfo(&systemInfo);

  if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
    return Architecture::ARM64;
  }

  if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
    return Architecture::X86_64;
  }

  return Architecture::X86;
}

}  // namespace hmcl