#include "check.hpp"
#include "vidra/core/paths.hpp"
#include "vidra/core/settings.hpp"
#include <cstdlib>
int main() {
  setenv("XDG_CONFIG_HOME", "/tmp/vidra-test-cfg", 1);
  vidra::core::ensureDirs();
  auto& s = vidra::core::Settings::get();
  s.dark = false;
  s.seed = 0x112233;
  s.favorites = {"a", "b"};
  s.save();
  s.dark = true;
  s.favorites.clear();
  s.load();
  CHECK(!s.dark);
  CHECK_EQ(s.seed, 0x112233u);
  CHECK_EQ(s.favorites.size(), size_t(2));
  return report("settings");
}
