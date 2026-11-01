#include "vidra/gfx/renderer.hpp"

namespace vidra::gfx {

Renderer* createGLRenderer();
#ifdef VIDRA_HAVE_VULKAN
Renderer* createVkRenderer();
#endif

Renderer* createRenderer(BackendKind kind) {
  if (kind == BackendKind::OpenGL) return createGLRenderer();
#ifdef VIDRA_HAVE_VULKAN
  if (kind == BackendKind::Vulkan) return createVkRenderer();
#endif
  return nullptr;
}

} // namespace vidra::gfx
