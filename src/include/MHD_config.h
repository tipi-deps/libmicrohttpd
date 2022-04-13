#if _WIN32
#include <MHD_config_windows.h>
#elif __APPLE__
#include <MHD_config_macos.h>
#else
#include <MHD_config_linux.h>
#endif