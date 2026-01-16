#include "genesis/core/Log.h"

namespace Genesis {

    void PlatformDetection() {
#if defined(GENESIS_PLATFORM_WINDOWS)
        GEN_INFO("Platform: Windows");
#elif defined(GENESIS_PLATFORM_MACOS)
        GEN_INFO("Platform: macOS");
#elif defined(GENESIS_PLATFORM_LINUX)
        GEN_INFO("Platform: Linux");
#else
        GEN_WARN("Platform: Unknown");
#endif
    }

}
