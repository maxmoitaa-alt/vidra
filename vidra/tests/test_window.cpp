#include "check.hpp"
#include "vidra/platform/window.hpp"
#include <cstdlib>
int main() {
  if (!std::getenv("DISPLAY")) return 77;
  auto w = vidra::platform::createWindow(vidra::platform::Role::Main, "teste", 640, 480);
  CHECK(w != nullptr);
  const auto g = w->geometry();
  CHECK(g.w > 0 && g.h > 0);
  w->resize(800, 600);
  CHECK(!w->shouldClose());
  w->close();
  CHECK(w->shouldClose());
  return report("window");
}
