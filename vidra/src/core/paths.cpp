#include "vidra/core/paths.hpp"
#include <cstdlib>
#include <random>

namespace vidra::core {
namespace {
fs::path envOr(const char* var, const char* fb) {
  const char* v = std::getenv(var);
  if (v && *v) return fs::path(v) / "vidra";
  const char* h = std::getenv("HOME");
  return fs::path(h ? h : "/tmp") / fb / "vidra";
}
} // namespace

fs::path configDir() { return envOr("XDG_CONFIG_HOME", ".config"); }
fs::path dataDir() { return envOr("XDG_DATA_HOME", ".local/share"); }
fs::path cacheDir() { return envOr("XDG_CACHE_HOME", ".cache"); }
fs::path runtimeDir() {
  const char* v = std::getenv("XDG_RUNTIME_DIR");
  return fs::path(v ? v : "/tmp") / "vidra";
}
void ensureDirs() {
  std::error_code ec;
  for (const auto& p : {configDir(), dataDir(), cacheDir(), runtimeDir()}) fs::create_directories(p, ec);
}
std::string instanceToken() {
  static const std::string t = [] {
    std::random_device rd;
    std::mt19937_64 g(rd());
    char b[17];
    std::snprintf(b, sizeof(b), "%016llx", (unsigned long long)(g() & 0xffffffffffffffffULL));
    return std::string(b);
  }();
  return t;
}
} // namespace vidra::core
