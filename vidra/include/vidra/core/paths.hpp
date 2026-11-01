#pragma once
#include <filesystem>
#include <string>

namespace vidra::core {
namespace fs = std::filesystem;
fs::path configDir();
fs::path dataDir();
fs::path cacheDir();
fs::path runtimeDir();
void ensureDirs();
std::string instanceToken();
} // namespace vidra::core
