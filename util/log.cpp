#include <regex>
#include <filesystem>

#include "log.h"
#include "../core/threading/thread_pool.h"

namespace util::log {
    auto print(
            std::string_view out,
            Level kind,
            std::string file,
            std::string caller,
            const size_t line) -> void {
        const std::filesystem::path file_path = std::filesystem::canonical(file);
        const std::string project_root = PROJECT_ROOT_DIR;

        auto f = file_path.string();
        auto pos = f.find(PROJECT_ROOT_DIR);
        if (pos != std::string::npos)
            f = f.substr(pos + project_root.length() + 1);

        f = std::regex_replace(f, std::regex("\\.(cpp|h)$"), "");

        std::ostringstream oss;
        switch (kind) {
            case Level::LOG_LEVEL_DEBUG : oss << "[\033[38;5;208mDEBUG\033[0m]"; break;
            case Level::LOG_LEVEL_NORMAL: oss << "[\033[38;5;46mLOG\033[0m]";    break;
            case Level::LOG_LEVEL_WARN  : oss << "[\033[38;5;226mWARN\033[0m]";  break;
            case Level::LOG_LEVEL_ERROR : oss << "[\033[1;31mERROR\033[0m]";     break;
        }

        oss << "[" << f << ":" << line << "]"
            << "[" << caller << "] "
            << "| " << out;

        if (oss.str().back() != '\n')
            oss << '\n';

        if (Level::LOG_LEVEL_ERROR) {
            std::cerr << oss.str();
        }
        else {
            std::cout << oss.str();
        }
    }
}
