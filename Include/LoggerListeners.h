
#include "Logger.h"
#include "OSSupport/File.h"

COMMON_API std::unique_ptr<cLogger::cListener> MakeConsoleListener(bool a_IsService);
COMMON_API std::pair<bool, std::unique_ptr<cLogger::cListener>> MakeFileListener();




