#include <windows.h>
#include <vector>

#include "java.h"

HLJavaVersion HLJavaVersion::INVALID = HLJavaVersion{};

HLJavaVersion HLJavaVersion::fromJavaExecutable(const std::wstring &filePath) {
  UINT size = 0;
  VS_FIXEDFILEINFO *pFileInfo;
  DWORD dwSize = GetFileVersionInfoSizeW(filePath.c_str(), nullptr);

  if (!dwSize) return INVALID;

  std::vector<BYTE> data(dwSize);
  if (!GetFileVersionInfoW(filePath.c_str(), 0, dwSize, &data[0])) {
    return INVALID;
  }

  if (!VerQueryValueW(&data[0], L"\\", (LPVOID *)&pFileInfo, &size)) {
    return INVALID;
  }

  return HLJavaVersion{.major = static_cast<uint16_t>((pFileInfo->dwFileVersionMS >> 16) & 0xFFFF),
                       .minor = static_cast<uint16_t>((pFileInfo->dwFileVersionMS >> 0) & 0xFFFF),
                       .build = static_cast<uint16_t>((pFileInfo->dwFileVersionLS >> 16) & 0xFFFF),
                       .revision = static_cast<uint16_t>((pFileInfo->dwFileVersionLS >> 0) & 0xFFFF)};
}

bool HLLaunchJVM(const std::wstring &javaExecutablePath, const HLJavaOptions &options,
                 const std::optional<HLJavaVersion> &version) {
  std::wstring command;
  command += '"';
  command += javaExecutablePath;
  command += L'"';
  if (options.jvmOptions.has_value()) {
    command += L' ';
    command += options.jvmOptions.value();
  } else {
    command += L" -Xmx1G -XX:MinHeapFreeRatio=5 -XX:MaxHeapFreeRatio=15";
  }
  command += L" -jar \"";
  command += options.jarPath;
  command += L'"';

  STARTUPINFOW startupInfo{.cb = sizeof(STARTUPINFOW)};
  PROCESS_INFORMATION processInformation{};

  return CreateProcessW(nullptr, &command[0], nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr,
                        options.workdir.c_str(), &startupInfo, &processInformation) != 0;
}
