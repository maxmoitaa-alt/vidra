#include "vidra/ui/screens.hpp"
#include <algorithm>

namespace vidra::ui {

Home::Home(core::Settings& s, runtime::Vm& v) : st_(s), vm_(v) {
  all_ = {
    {"Infinite Yield", "author/iy", "terminal", "print('iy')", "util"},
    {"Hydroxide", "author/hx", "water", "print('hx')", "util"},
    {"Rayfield", "author/rf", "brush", "print('rf')", "ui"},
    {"Fly", "author/vidra", "flight", "print('fly')", "movement"},
    {"Speed", "author/vidra", "bolt", "print('speed')", "movement"},
    {"ESP", "author/vidra", "visibility", "print('esp')", "visual"},
    {"Auto Farm", "author/vidra", "agriculture", "print('farm')", "farm"},
    {"Teleport", "author/vidra", "pin_drop", "print('tp')", "movement"},
  };
  search_ = std::make_unique<Field>("Buscar scripts");
  search_->onInput = [this](const std::string& v) { filter_ = v; rebuild(); };
  for (const char* c : {"util", "ui", "movement", "visual", "farm"}) {
    auto ch = std::make_unique<Chip>(c);
    ch->onToggle = [this, n = std::string(c)](bool on) {
      category_ = on ? n : std::string();
      rebuild();
    };
    chips_.push_back(std::move(ch));
  }
  rebuild();
}

void Home::rebuild() {
  cards_.clear();
  for (const auto& e : all_) {
    if (!filter_.empty() && e.name.find(filter_) == std::string::npos) continue;
    if (!category_.empty() && e.category != category_) continue;
    auto c = std::make_unique<Card>(e.name, e.author, e.glyph);
    c->setFav(std::find(st_.favorites.begin(), st_.favorites.end(), e.name) != st_.favorites.end());
    c->onOpen = [this, n = e.name, s = e.source] { if (onOpen) onOpen(n, s); };
    c->onFav = [this, n = e.name] {
      auto it = std::find(st_.favorites.begin(), st_.favorites.end(), n);
      if (it == st_.favorites.end()) st_.favorites.push_back(n);
      else st_.favorites.erase(it);
      st_.save();
    };
    cards_.push_back(std::move(c));
  }
}

void Home::layout(RectF b) {
  bounds = b;
  search_->layout({b.x, b.y + 64, 320, 48});
  float cx = b.x;
  for (auto& c : chips_) { c->layout({cx, b.y + 128, 110, 40}); cx += 118; }
  int i = 0;
  for (auto& c : cards_) {
    const float col = float(i % 3), row = float(i / 3);
    c->layout({b.x + col * 316.0f, b.y + 192 + row * 164.0f, 300, 148});
    ++i;
  }
}

void Home::update(double dt, bool reduce) {
  search_->update(dt, reduce);
  for (auto& c : chips_) c->update(dt, reduce);
  for (auto& c : cards_) c->update(dt, reduce);
}

void Home::paint(Painter& p, const Palette& pal) {
  p.text("Scripts", bounds.x, bounds.y + 44, tok::kTypeDisplayMediumSize, pal[Role::OnSurface]);
  search_->paint(p, pal);
  for (auto& c : chips_) c->paint(p, pal);
  for (auto& c : cards_) c->paint(p, pal);
  if (cards_.empty()) {
    p.text("Nada encontrado", bounds.x, bounds.y + 240, tok::kTypeTitleLargeSize,
           pal[Role::OnSurfaceVariant]);
  }
}

ConsoleView::ConsoleView(runtime::Vm& v) : vm_(v) {
  vm_.setOutput([this](bool err, const std::string& t) { append(err, t); });
}
void ConsoleView::append(bool err, const std::string& t) {
  lines_.push_back({err, t});
  if (lines_.size() > 500) lines_.pop_front();
}
void ConsoleView::update(double dt, bool reduce) { Widget::update(dt, reduce); }
void ConsoleView::paint(Painter& p, const Palette& pal) {
  p.rect(bounds, 0.0f, pal[Role::SurfaceLowest]);
  float y = bounds.y + 24;
  for (const auto& [err, t] : lines_) {
    if (y > bounds.y + bounds.h) break;
    p.text(t, bounds.x + 16, y, tok::kTypeBodyLargeSize,
           pal[err ? Role::Error : Role::OnSurfaceVariant]);
    y += 22;
  }
}

Explorer::Explorer() { build(); }
void Explorer::build() {
  nodes_ = {{"Workspace", 0, true, {1, 2}}, {"Camera", 1, false, {}},
            {"Baseplate", 1, false, {}}, {"Players", 0, true, {4}}, {"LocalPlayer", 1, false, {}}};
  visible_.clear();
  flatten(0);
  flatten(3);
}
void Explorer::flatten(int i) {
  visible_.push_back(i);
  if (nodes_[size_t(i)].open) for (int k : nodes_[size_t(i)].kids) flatten(k);
}
void Explorer::layout(RectF b) { bounds = b; }
void Explorer::paint(Painter& p, const Palette& pal) {
  p.rect(bounds, 0.0f, pal[Role::SurfaceLow]);
  float y = bounds.y + 30;
  for (int i : visible_) {
    const auto& n = nodes_[size_t(i)];
    p.text(n.open ? "▾" : "▸", bounds.x + 10 + n.depth * 18, y, 13.0f, pal[Role::OnSurfaceVariant]);
    p.text(n.name, bounds.x + 28 + n.depth * 18, y, tok::kTypeBodyLargeSize, pal[Role::OnSurface]);
    y += 26;
  }
}
bool Explorer::onPointer(float, float y, bool pressed, int) {
  if (!pressed) return true;
  int row = int((y - bounds.y - 18) / 26);
  if (row < 0 || row >= int(visible_.size())) return true;
  auto& n = nodes_[size_t(visible_[size_t(row)])];
  n.open = !n.open;
  visible_.clear();
  flatten(0);
  flatten(3);
  return true;
}

SettingsView::SettingsView(core::Settings& s) : st_(s) {}
void SettingsView::layout(RectF b) {
  bounds = b;
  float y = b.y + 90;
  for (auto& r : rows_) { r->layout({b.x, y, 260, 40}); y += 60; }
}
void SettingsView::paint(Painter& p, const Palette& pal) {
  p.text("Configuracoes", bounds.x, bounds.y + 50, tok::kTypeHeadlineLargeSize, pal[Role::OnSurface]);
  p.text("Tema", bounds.x, bounds.y + 90, tok::kTypeBodyLargeSize, pal[Role::OnSurfaceVariant]);
  p.rect({bounds.x + 140, bounds.y + 72, 52, 32}, 16.0f,
         pal[st_.dark ? Role::Primary : Role::SurfaceHighest]);
  p.rect({bounds.x + 140 + (st_.dark ? 26.0f : 4.0f), bounds.y + 76, 24, 24}, 12.0f,
         pal[st_.dark ? Role::OnPrimary : Role::Outline]);
  p.text("Reduzir movimento", bounds.x, bounds.y + 150, tok::kTypeBodyLargeSize,
         pal[Role::OnSurfaceVariant]);
  p.rect({bounds.x + 180, bounds.y + 132, 52, 32}, 16.0f,
         pal[st_.reduceMotion ? Role::Primary : Role::SurfaceHighest]);
  p.rect({bounds.x + 180 + (st_.reduceMotion ? 26.0f : 4.0f), bounds.y + 136, 24, 24}, 12.0f,
         pal[st_.reduceMotion ? Role::OnPrimary : Role::Outline]);
}
AboutView::AboutView() { bounds = {0, 0, 400, 300}; }
void AboutView::paint(Painter& p, const Palette& pal) {
  p.text("Vidra 1.0.1", bounds.x, bounds.y + 80, tok::kTypeDisplayMediumSize, pal[Role::OnSurface]);
  p.text("Executor Luau para Roblox em Linux.", bounds.x, bounds.y + 120,
         tok::kTypeBodyLargeSize, pal[Role::OnSurfaceVariant]);
  p.text("Material 3 Expressive + Liquid Glass.", bounds.x, bounds.y + 148,
         tok::kTypeBodyLargeSize, pal[Role::OnSurfaceVariant]);
}

} // namespace vidra::ui
