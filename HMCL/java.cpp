#include <windows.h>
#include <format>
#include <vector>

#include "debug.h"
#include "java.h"

HLJavaVersion HLJavaVersion::INVALID = HLJavaVersion{};

HLJavaVersion HLJavaVersion::FromJavaExecutable(const HLPath &filePath) {
  UINT size = 0;
  VS_FIXEDFILEINFO *pFileInfo;
  DWORD dwSize = GetFileVersionInfoSizeW(filePath.path.c_str(), nullptr);

  if (!dwSize) return INVALID;

  std::vector<BYTE> data(dwSize);
  if (!GetFileVersionInfoW(filePath.path.c_str(), 0, dwSize, &data[0])) {
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

HLJavaVersion HLJavaVersion::FromString(const std::wstring &versionString) {
  HLJavaVersion version = {};

  uint16_t ver[4] = {0, 0, 0, 0};  // major, minor, build, revision

  int idx = 0;

  for (auto &ch : versionString) {
    if (idx >= 4) {
      break;
    }
    if (ch == L'.' || ch == L'_') {
      if (idx == 0 && ver[0] == 1) {
        // For legacy Java version 1.x
        ver[0] = 0;
      } else {
        idx++;
      }
    } else if (ch >= L'0' && ch <= L'9') {
      ver[idx] = ver[idx] * 10 + (ch - L'0');
    }
  }

  return HLJavaVersion{
      .major = ver[0],
      .minor = ver[1],
      .build = ver[2],
      .revision = ver[3],
  };
}

bool HLLaunchJVM(const HLPath &javaExecutablePath, const HLJavaOptions &options,
                 const std::optional<HLJavaVersion> &version) {
  std::wstring command;
  command += '"';
  command += javaExecutablePath.path;
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

  BOOL result = CreateProcessW(nullptr, &command[0], nullptr, nullptr, false, NORMAL_PRIORITY_CLASS, nullptr,
                               options.workdir.path.c_str(), &startupInfo, &processInformation);
  if (result) {
    HLDebugLog(L"Successfully launched HMCL with " + javaExecutablePath.path);
  } else {
    HLDebugLog(L"Failed to launch HMCL with " + javaExecutablePath.path);
  }

  return result;
}

void HLSearchJavaInDir(HLJavaList &result, const HLPath &basedir, LPCWSTR javaExecutableName) {
  HLDebugLogVerbose(std::format(L"Searching for Java in directory: {}", basedir.path));

  HLPath pattern = basedir;
  pattern /= L"*";

  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFileW(pattern.path.c_str(), &data);  // Search all subdirectory
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      result.AddIfAcceptable(basedir / data.cFileName / L"bin" / javaExecutableName);
    } while (FindNextFile(hFind, &data));
    FindClose(hFind);
  }
}

static const LPCWSTR VENDORS[] = {L"Java",         L"Microsoft", L"BellSoft", L"Zulu", L"Eclipse Foundation",
                                  L"AdoptOpenJDK", L"Semeru"};

void HLSearchJavaInProgramFiles(HLJavaList &result, const HLPath &programFiles, LPCWSTR javaExecutableName) {
  for (LPCWSTR vendorDir : VENDORS) {
    HLPath dir = programFiles / vendorDir;
    HLSearchJavaInDir(result, dir, javaExecutableName);
  }
}

void HLSearchJavaInRegistry(HLJavaList &result, LPCWSTR subKey, LPCWSTR javaExecutableName) {
  HLDebugLogVerbose(std::format(L"Searching for Java in registry key: HKEY_LOCAL_MACHINE\\{}", subKey));

  constexpr int MAX_KEY_LENGTH = 255;

  WCHAR javaVer[MAX_KEY_LENGTH];  // buffer for subkey name, special for
                                  // JavaVersion
  WCHAR javaHome[MAX_PATH];       // buffer for JavaHome value
  DWORD cbName;                   // size of name string
  DWORD cSubKeys = 0;             // number of subkeys
  DWORD cbMaxSubKey;              // longest subkey size
  DWORD cValues;                  // number of values for key
  DWORD cchMaxValue;              // longest value name
  DWORD cbMaxValueData;           // longest value data

  HKEY hKey;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_WOW64_64KEY | KEY_READ, &hKey) != ERROR_SUCCESS) {
    return;
  }

  RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &cSubKeys, &cbMaxSubKey, nullptr, &cValues, &cchMaxValue,
                   &cbMaxValueData, nullptr, nullptr);

  if (!cSubKeys) {
    RegCloseKey(hKey);
    return;
  }

  for (DWORD i = 0; i < cSubKeys; ++i) {
    cbName = MAX_KEY_LENGTH;
    if (RegEnumKeyExW(hKey, i, javaVer, &cbName, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
      continue;
    }

    DWORD dataLength = sizeof(javaHome);
    if (RegGetValueW(hKey, javaVer, L"JavaHome", RRF_RT_REG_SZ, nullptr, &javaHome[0], &dataLength) != ERROR_SUCCESS) {
      continue;
    }

    result.AddIfAcceptable(HLPath(javaHome) / L"bin" / javaExecutableName);
  }

  RegCloseKey(hKey);
}

bool HLJavaList::AddIfAcceptable(const HLPath &javaExecutable) {
  if (paths.contains(javaExecutable.path)) {
    HLDebugLogVerbose(std::format(L"Ignore duplicate Java {}", javaExecutable.path));
    return false;
  }

  auto version = HLJavaVersion::FromJavaExecutable(javaExecutable);
  HLDebugLogVerbose(std::format(L"Found Java {}, Version {}", javaExecutable.path, version.ToWString(),
                                version.IsAcceptable() ? L"" : L", Ignored"));
  if (version.IsAcceptable()) {
    return false;
  }

  HLJavaRuntime javaRuntime = {
      .version = version,
      .executablePath = javaExecutable,
  };

  paths.insert(javaExecutable.path);
  runtimes.push_back(javaRuntime);
  return true;
}
