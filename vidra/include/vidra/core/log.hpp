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

#define VIDRA_LOGD(...) ::vidra::core::Log::get().write(::vidra::core::Level::Debug, __VA_ARGS__)
#define VIDRA_LOGI(...) ::vidra::core::Log::get().write(::vidra::core::Level::Info, __VA_ARGS__)
#define VIDRA_LOGW(...) ::vidra::core::Log::get().write(::vidra::core::Level::Warn, __VA_ARGS__)
#define VIDRA_LOGE(...) ::vidra::core::Log::get().write(::vidra::core::Level::Error, __VA_ARGS__)
