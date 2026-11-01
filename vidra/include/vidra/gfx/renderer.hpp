#pragma once
#include "vidra/color/hct.hpp"
#include "vidra/gfx/sdf.hpp"
#include <cstdint>
#include <string>

namespace vidra::gfx {
enum class Material : uint8_t { Tonal, Glass, Text, Shadow, Scrim };
enum class BackendKind { Vulkan, OpenGL };

struct Quad {
  Material mat = Material::Tonal;
  RectF box{};
  float radius = 0, exponent = 4.0f, alpha = 1.0f;
  color::Rgba8 color{};
  RectF uv{};
  float thickness = 24.0f, blur = 0.0f, notchDepth = 0.0f;
};

class Renderer {
 public:
  virtual ~Renderer() = default;
  virtual bool init(void* display, uint64_t window, int w, int h, std::string* err) = 0;
  virtual void shutdown() = 0;
  virtual void resize(int w, int h) = 0;
  virtual void beginFrame(float time) = 0;
  virtual void endFrame() = 0;
  virtual void push(const Quad& q) = 0;
  virtual BackendKind kind() const = 0;
  virtual bool ok() const = 0;
  virtual void uploadAtlas(int x, int y, int w, int h, const uint8_t* rgba) = 0;
  virtual int width() const = 0;
  virtual int height() const = 0;
  virtual void setPalette(const float tint[3], const float surface[3], float hover) = 0;
};

Renderer* createRenderer(BackendKind k);
} // namespace vidra::gfx
