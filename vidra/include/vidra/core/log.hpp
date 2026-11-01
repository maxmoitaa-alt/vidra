#pragma once
#include <cstddef>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace vidra::core {

enum class Level { Debug, Info, Warn, Error };
struct LogLine { Level level; double t; std::string text; };

class Log {
 public:
  static Log& get();
  void write(Level l, const std::string& s);
  template <typename... Args>
  void writef(Level l, const char* format, Args... args) {
    const int length = std::snprintf(nullptr, 0, format, args...);
    if (length < 0) {
      write(l, format);
      return;
    }
    std::string message(static_cast<size_t>(length) + 1, '\\0');
    std::snprintf(message.data(), message.size(), format, args...);
    message.resize(static_cast<size_t>(length));
    write(l, message);
  }
  std::vector<LogLine> snapshot() const;
  void clear();
  size_t errorCount() const;

 private:
  mutable std::mutex mu_;
  std::vector<LogLine> lines_;
  size_t errors_ = 0;
  double t0_ = 0.0;
  Log();
};

} // namespace vidra::core

#define VIDRA_LOGD(...) ::vidra::core::Log::get().writef(::vidra::core::Level::Debug, __VA_ARGS__)
#define VIDRA_LOGI(...) ::vidra::core::Log::get().writef(::vidra::core::Level::Info, __VA_ARGS__)
#define VIDRA_LOGW(...) ::vidra::core::Log::get().writef(::vidra::core::Level::Warn, __VA_ARGS__)
#define VIDRA_LOGE(...) ::vidra::core::Log::get().writef(::vidra::core::Level::Error, __VA_ARGS__)
