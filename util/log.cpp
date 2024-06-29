//
// Created by Luis Ruisinger on 13.06.24.
//

#include "log.h"

#include <regex>
#include <filesystem>

namespace util::log {
    auto print(
            std::string_view out,
            Level kind,
            const char *file,
            const char *caller,
            const size_t line) -> void {
        // Normalize and strip the project root directory from the file path
        std::filesystem::path filePath = std::filesystem::canonical(file);
        std::string f = filePath.string();

        // Remove the project root prefix
        const std::string projectRoot = PROJECT_ROOT_DIR;
        size_t pos = f.find(projectRoot);
        if (pos != std::string::npos) {
            f = f.substr(pos + projectRoot.length() + 1); // +1 to remove the leading slash
        }

        // Remove the file extension
        f = std::regex_replace(f, std::regex("\\.(cpp|h)$"), "");

        std::ostringstream oss;
        switch (kind) {
            case Level::LOG_LEVEL_DEBUG : oss << "[\033[38;5;208mDEBUG\033[0m]"; break;
            case Level::LOG_LEVEL_NORMAL: oss << "[\033[38;5;46mLOG\033[0m]  ";  break;
            case Level::LOG_LEVEL_WARN  : oss << "[\033[38;5;226mWARN\033[0m] "; break;
            case Level::LOG_LEVEL_ERROR : oss << "[\033[1;31mERROR\033[0m]";     break;
        }

        oss << "[" << f << ":" << line << "]"
            << "[" << caller << "] "
            << out;

        if (oss.str().back() != '\n') {
            oss << '\n';
        }

        kind != Level::LOG_LEVEL_ERROR ? std::cout << oss.str() : std::cerr << oss.str();
    }
}
