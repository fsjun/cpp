#include <memory>
#include <vector>

#include "fslog/Log.h"

#include "spdlog/formatter.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using std::unique_ptr;
using std::string;

void init_log(LogLevel level, std::string file, int size, int count, bool console)
{
    auto logger = spdlog::get("log");
    if (!logger) {
        logger = std::make_shared<spdlog::logger>("log");
        spdlog::register_logger(logger);
        if (console) {
            auto& sinks = logger->sinks();
            auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sinks.emplace_back(stdout_sink);
        }
    }
    if (!file.empty()) {
        auto& sinks = logger->sinks();
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, size, count);
        sinks.emplace_back(rotating_sink);
    }
    std::string eol("");
    std::string pattern("[%Y-%m-%d %H:%M:%S.%e %t %s:%#] <%^%L%$> %v");
    unique_ptr<spdlog::formatter> formatter = std::make_unique<spdlog::pattern_formatter>(pattern, spdlog::pattern_time_type::local, eol);
    spdlog::set_formatter(std::move(formatter));
    spdlog::set_level((spdlog::level::level_enum)level);
    spdlog::flush_on((spdlog::level::level_enum)level);
}
