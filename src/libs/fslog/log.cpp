#include <memory>
#include <vector>

#include "fslog/log.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

void init_log(LogLevel level, std::string file, int size, int count)
{
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, size, count);
    std::vector<spdlog::sink_ptr> sinks { stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::logger>("log", sinks.begin(), sinks.end());
    spdlog::register_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e %t %s:%#] <%^%L%$> %v");
    spdlog::set_level((spdlog::level::level_enum)level);
    spdlog::flush_on((spdlog::level::level_enum)level);
}
