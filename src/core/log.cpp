#include "dream/core/log.hpp"
#if defined(__ANDROID__)
#include <android/log.h>
#endif
#include <mutex>

namespace dream {
namespace {
std::mutex g_mutex;
std::string g_buffer;
constexpr std::size_t kMaxBuffer = 64 * 1024;
}

void log_push(int level, const std::string& message) {
#if defined(__ANDROID__)
    __android_log_write(level, "DreamEngine", message.c_str());
#else
    (void)level;
#endif
    std::lock_guard<std::mutex> lock(g_mutex);
    g_buffer += message;
    if (message.empty() || message.back() != '\n') g_buffer += '\n';
    if (g_buffer.size() > kMaxBuffer) g_buffer.erase(0, g_buffer.size() - kMaxBuffer);
}

std::string log_drain() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string out;
    out.swap(g_buffer);
    return out;
}
}
