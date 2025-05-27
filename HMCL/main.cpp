#include <windows.h>
#include <cstdlib>

#include "debug.h"
#include "i18n.h"
#include "os.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow) {
  const auto i18n = hmcl::I18N::Instance();

  auto selfPath = hmcl::GetSelfPath();
  auto last_slash = selfPath.find_last_of(L"/\\");
  if (selfPath.empty() || last_slash == std::wstring::npos || last_slash >= selfPath.size() - 1) {
    MessageBoxW(nullptr, i18n.errorSelfPath, nullptr, MB_OK | MB_ICONERROR);
    return EXIT_FAILURE;
  }

  auto workDir = selfPath.substr(0, last_slash);


  // Find Java

  return EXIT_SUCCESS;
}