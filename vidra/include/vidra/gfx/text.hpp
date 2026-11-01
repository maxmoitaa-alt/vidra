#pragma once
#include "vidra/gfx/renderer.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <map>
#include <string>
#include <vector>

namespace vidra::gfx {

struct AtlasSlot { float u0, v0, u1, v1; int w, h, bearing; };

class TextEngine {
 public:
  TextEngine();
  ~TextEngine();
  bool load(const std::string& path, std::string* err);
  bool ready() const { return face_ != nullptr; }
  int ascender(float px) const;
  float measure(const std::string& utf8, float px);
  std::vector<AtlasSlot> layout(const std::string& utf8, float px, std::vector<float>* adv);
  void upload(Renderer& r);

 private:
  const AtlasSlot* slotFor(uint32_t cp, float px);
  FT_Library lib_ = nullptr;
  FT_Face face_ = nullptr;
  int aw_ = 1024, ah_ = 1024, px_ = 1, py_ = 1, rowH_ = 0;
  std::vector<uint8_t> atlas_;
  std::map<uint64_t, AtlasSlot> cache_;
  bool dirty_ = true;
};

std::vector<uint32_t> utf8Decode(const std::string& s);

} // namespace vidra::gfx
