#include <app/Log.hpp>

namespace sauce {

    std::ofstream Log::logFile;
    std::mutex Log::logMutex;
    bool Log::palantirMode = true;
    bool Log::initialized = false;

    void Log::init(const std::string& filepath) {
#ifdef NDEBUG
        (void)filepath;
#else
        std::lock_guard lock(logMutex);
        if (initialized)
            return;

        logFile.open(filepath, std::ios::out | std::ios::trunc);
        initialized = logFile.is_open();
#endif
    }

    void Log::shutdown() {
#ifndef NDEBUG
        std::lock_guard lock(logMutex);
        if (logFile.is_open()) {
            logFile.flush();
            logFile.close();
        }
        initialized = false;
#endif
    }

    void Log::setPalantirMode(bool enabled) {
        palantirMode = enabled;
    }

    bool Log::isPalantirMode() {
        return palantirMode;
    }

    void Log::write(const std::string& category, const std::string& message) {
        std::lock_guard lock(logMutex);
        if (!initialized)
            return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::floor<std::chrono::milliseconds>(now);

        logFile << std::format("[{:%H:%M:%S}] [{}] {}\n", time, category, message);
        logFile.flush();
    }

} // namespace sauce
