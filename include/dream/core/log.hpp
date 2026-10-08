#pragma once
#include <string>

namespace dream {
// Thread-safe in-memory log (also mirrored to logcat on Android).
void log_push(int level, const std::string& message);
// Returns and clears everything logged since the last call.
std::string log_drain();
}
