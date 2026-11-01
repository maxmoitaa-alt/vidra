#include "vidra/core/log.hpp"
#include <chrono>

namespace vidra::core {
namespace {
double now() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}
} // namespace

Log::Log() : t0_(now()) {}
Log& Log::get() { static Log l; return l; }

void Log::write(Level l, const std::string& s) {
  {
    std::lock_guard<std::mutex> g(mu_);
    lines_.push_back({l, now() - t0_, s});
    if (lines_.size() > 4000) lines_.erase(lines_.begin(), lines_.begin() + 1000);
    if (l == Level::Error) ++errors_;
  }
  const char* tag = l == Level::Error ? "E" : l == Level::Warn ? "W" : l == Level::Info ? "I" : "D";
  std::fprintf(stderr, "[%s] %s\n", tag, s.c_str());
}

std::vector<LogLine> Log::snapshot() const {
  std::lock_guard<std::mutex> g(mu_);
  return lines_;
}
size_t Log::errorCount() const {
  std::lock_guard<std::mutex> g(mu_);
  return errors_;
}

void Log::clear() {
  std::lock_guard<std::mutex> g(mu_);
  lines_.clear();
  errors_ = 0;
}
} // namespace vidra::core
