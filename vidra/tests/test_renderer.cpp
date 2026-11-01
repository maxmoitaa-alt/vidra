#include "check.hpp"
#include "vidra/gfx/renderer.hpp"
#include "vidra/platform/window.hpp"
#include <cstdlib>
int main() {
  if (!std::getenv("DISPLAY")) return 77;
  auto w = vidra::platform::createWindow(vidra::platform::Role::Main, "gl", 320, 240);
  auto* r = vidra::gfx::createRenderer(vidra::gfx::BackendKind::OpenGL);
  std::string err;
  if (!r->init(w->nativeDisplay(), w->nativeHandle(), 320, 240, &err)) { delete r; return 77; }
  vidra::gfx::Quad q{};
  q.box = {10, 10, 100, 60};
  q.color = {255, 255, 255, 255};
  r->beginFrame(0.0f);
  r->push(q);
  r->endFrame();
  CHECK(r->ok());
  r->shutdown();
  delete r;
  return report("renderer");
}
