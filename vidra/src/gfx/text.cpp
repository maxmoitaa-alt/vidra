#include "vidra/gfx/text.hpp"
#include "vidra/core/log.hpp"
#include <algorithm>

namespace vidra::gfx {

std::vector<uint32_t> utf8Decode(const std::string& s) {
  std::vector<uint32_t> o;
  size_t i = 0;
  while (i < s.size()) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    uint32_t cp = c;
    int extra = 0;
    if (c >= 0xF0) { cp = c & 0x07; extra = 3; }
    else if (c >= 0xE0) { cp = c & 0x0F; extra = 2; }
    else if (c >= 0xC0) { cp = c & 0x1F; extra = 1; }
    ++i;
    for (int k = 0; k < extra && i < s.size(); ++k, ++i) cp = (cp << 6) | (s[i] & 0x3F);
    o.push_back(cp);
  }
  return o;
}

TextEngine::TextEngine() {
  atlas_.assign(size_t(aw_) * size_t(ah_) * 4, 0);
  if (FT_Init_FreeType(&lib_) != 0) core::VIDRA_LOGE("FreeType init falhou");
}

TextEngine::~TextEngine() {
  if (face_) FT_Done_Face(face_);
  if (lib_) FT_Done_FreeType(lib_);
}

bool TextEngine::load(const std::string& path, std::string* err) {
  if (FT_New_Face(lib_, path.c_str(), 0, &face_) != 0) {
    if (err) *err = "fonte: " + path;
    return false;
  }
  FT_Select_Charmap(face_, FT_ENCODING_UNICODE);
  core::VIDRA_LOGI("fonte: %s", path.c_str());
  return true;
}

int TextEngine::ascender(float px) const {
  if (!face_) return int(px * 0.8f);
  return int(face_->ascender * (px / float(face_->units_per_EM)));
}

const AtlasSlot* TextEngine::slotFor(uint32_t cp, float px) {
  const int ip = std::max(1, int(px));
  const uint64_t key = (uint64_t(cp) << 20) | uint64_t(ip);
  if (const auto it = cache_.find(key); it != cache_.end()) return &it->second;
  FT_Set_Pixel_Sizes(face_, 0, unsigned(ip));
  if (FT_Load_Char(face_, cp, FT_LOAD_RENDER) != 0) return nullptr;
  const FT_GlyphSlot g = face_->glyph;
  const int w = int(g->bitmap.width), h = int(g->bitmap.rows);
  if (w == 0 || h == 0) return nullptr;
  if (px_ + w + 1 >= aw_) { px_ = 1; py_ += rowH_ + 1; rowH_ = 0; }
  if (py_ + h + 1 >= ah_) return nullptr;
  for (int y = 0; y < h; ++y) {
    const uint8_t* s = g->bitmap.buffer + y * g->bitmap.pitch;
    uint8_t* d = atlas_.data() + ((py_ + y) * aw_ + px_) * 4;
    for (int x = 0; x < w; ++x) {
      d[x * 4 + 0] = 255; d[x * 4 + 1] = 255; d[x * 4 + 2] = 255; d[x * 4 + 3] = s[x];
    }
  }
  AtlasSlot sl{float(px_) / aw_, float(py_) / ah_, float(px_ + w) / aw_, float(py_ + h) / ah_,
               w, h, int(g->bitmap_top)};
  px_ += w + 1;
  rowH_ = std::max(rowH_, h);
  dirty_ = true;
  return &cache_.emplace(key, sl).first->second;
}

std::vector<AtlasSlot> TextEngine::layout(const std::string& utf8, float px, std::vector<float>* adv) {
  std::vector<AtlasSlot> out;
  if (!face_) return out;
  for (uint32_t cp : utf8Decode(utf8)) {
    const AtlasSlot* s = slotFor(cp, px);
    if (adv) adv->push_back(s ? float(s->w) + 1.0f : px * 0.45f);
    out.push_back(s ? *s : AtlasSlot{});
  }
  return out;
}

float TextEngine::measure(const std::string& utf8, float px) {
  std::vector<float> adv;
  layout(utf8, px, &adv);
  float t = 0;
  for (float a : adv) t += a;
  return t;
}

void TextEngine::upload(Renderer& r) {
  if (!dirty_) return;
  r.uploadAtlas(0, 0, aw_, ah_, atlas_.data());
  dirty_ = false;
}

} // namespace vidra::gfx
