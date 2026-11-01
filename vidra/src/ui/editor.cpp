#include "vidra/ui/editor.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace vidra::ui {
namespace {
const char* kKeywords[] = {"local", "function", "end", "if", "then", "else", "elseif", "for",
                           "while", "do", "return", "and", "or", "not", "nil", "true", "false",
                           "break", "continue", "in", "repeat", "until"};
bool isKeyword(const std::string& w) {
  for (const char* k : kKeywords) if (w == k) return true;
  return false;
}
bool isIdent(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
} // namespace

void Buffer::insert(const std::string& s) {
  undo.push_back({lines, {line, col}});
  if (undo.size() > 200) undo.erase(undo.begin());
  lines[line].insert(col, s);
  col += s.size();
}
void Buffer::newline() {
  undo.push_back({lines, {line, col}});
  const std::string rest = lines[line].substr(col);
  lines[line] = lines[line].substr(0, col);
  lines.insert(lines.begin() + long(line) + 1, rest);
  ++line;
  col = 0;
}
void Buffer::backspace() {
  if (col == 0 && line == 0) return;
  undo.push_back({lines, {line, col}});
  if (col > 0) { lines[line].erase(col - 1, 1); --col; }
  else {
    col = lines[line - 1].size();
    lines[line - 1] += lines[line];
    lines.erase(lines.begin() + long(line));
    --line;
  }
}
void Buffer::del() {
  if (col < lines[line].size()) { undo.push_back({lines, {line, col}}); lines[line].erase(col, 1); }
  else if (line + 1 < lines.size()) {
    undo.push_back({lines, {line, col}});
    lines[line] += lines[line + 1];
    lines.erase(lines.begin() + long(line) + 1);
  }
}
void Buffer::move(int dl, int dc, bool select) {
  if (!select && hasAnchor) hasAnchor = false;
  if (select && !hasAnchor) { anchorLine = line; anchorCol = col; hasAnchor = true; }
  const int nl = std::clamp(int(line) + dl, 0, int(lines.size()) - 1);
  line = size_t(nl);
  const int nc = std::clamp(int(col) + dc, 0, int(lines[line].size()));
  col = size_t(nc);
}
void Buffer::undoLast() {
  if (undo.empty()) return;
  auto [l, c] = undo.back();
  lines = l;
  line = std::min(c.first, lines.size() - 1);
  col = std::min(c.second, lines[line].size());
  undo.pop_back();
}
std::string Buffer::all() const {
  std::string o;
  for (size_t i = 0; i < lines.size(); ++i) { o += lines[i]; if (i + 1 < lines.size()) o += '\n'; }
  return o;
}
bool Buffer::load(const std::string& p) {
  std::ifstream f(p);
  if (!f) return false;
  lines.clear();
  std::string l;
  while (std::getline(f, l)) lines.push_back(l);
  if (lines.empty()) lines.push_back("");
  line = col = 0;
  path = p;
  undo.clear();
  return true;
}
bool Buffer::save(const std::string& p) {
  std::ofstream f(p);
  if (!f) return false;
  f << all();
  path = p;
  return true;
}

Editor::Editor() {
  focusable = true;
  bounds = {0, 0, 600, 400};
}

void Editor::openFile(const std::string& p) {
  buf_.load(p);
}

void Editor::paint(Painter& p, const Palette& pal) {
  p.rect(bounds, 0.0f, pal[Role::SurfaceLowest]);
  const float gutter = 56.0f;
  p.rect({bounds.x, bounds.y, gutter, bounds.h}, 0.0f, pal[Role::SurfaceLow]);
  const int visible = int(bounds.h / lineH_) + 1;
  const int first = int(scroll_ / lineH_);
  for (int i = 0; i < visible; ++i) {
    const int li = first + i;
    if (li >= int(buf_.lines.size())) break;
    const float y = bounds.y + float(i) * lineH_ - std::fmod(scroll_, lineH_) + 16.0f;
    p.text(std::to_string(li + 1), bounds.x + 12, y, 13.0f, pal[Role::OnSurfaceVariant]);
    const std::string& ln = buf_.lines[size_t(li)];
    float x = bounds.x + gutter + 10;
    size_t i2 = 0;
    while (i2 < ln.size()) {
      if (isIdent(ln[i2])) {
        size_t j = i2;
        while (j < ln.size() && isIdent(ln[j])) ++j;
        const std::string w = ln.substr(i2, j - i2);
        p.text(w, x, y, 14.0f, pal[isKeyword(w) ? Role::Tertiary : Role::OnSurface]);
        x += p.measure(w, 14.0f);
        i2 = j;
      } else {
        const std::string ch(1, ln[i2]);
        p.text(ch, x, y, 14.0f, pal[Role::OnSurfaceVariant]);
        x += p.measure(ch, 14.0f);
        ++i2;
      }
    }
    if (size_t(li) == buf_.line) {
      const std::string pre = ln.substr(0, buf_.col);
      const float cx = bounds.x + gutter + 10 + p.measure(pre, 14.0f);
      p.rect({cx, y - 14, 2, 18}, 1.0f, pal[Role::Primary]);
    }
  }
}

void Editor::keyDefault(const Key& k) {
  switch (k.code) {
    case 0xFF0D: buf_.newline(); break;
    case 0xFF08: buf_.backspace(); break;
    case 0xFF51: buf_.move(0, -1, k.shift); break;
    case 0xFF53: buf_.move(0, 1, k.shift); break;
    case 0xFF52: buf_.move(-1, 0, k.shift); break;
    case 0xFF54: buf_.move(1, 0, k.shift); break;
    case 0xFF50: buf_.move(-10, 0, k.shift); break;
    case 0xFF57: buf_.move(10, 0, k.shift); break;
    case 0xFFE3: break;
    default:
      if (!k.text.empty()) buf_.insert(k.text);
      else if (k.code == 0xFF09) buf_.insert("  ");
      break;
  }
}

bool Editor::onKey(const Key& k, bool pressed) {
  if (!pressed) return false;
  if (k.ctrl) {
    if (k.code == 'z' || k.code == 'Z') { buf_.undoLast(); return true; }
    if (k.code == 's' || k.code == 'S') {
      if (onSave) onSave(buf_.path.empty() ? "untitled.luau" : buf_.path);
      return true;
    }
    return false;
  }
  keyDefault(k);
  const float target = float(buf_.line) * lineH_;
  if (target < scroll_) scroll_ = target;
  if (target > scroll_ + bounds.h - lineH_ * 2) scroll_ = target - bounds.h + lineH_ * 2;
  return true;
}

bool Editor::onPointer(float x, float y, bool pressed, int) {
  if (pressed) {
    dragging_ = true;
    const int li = int((y - bounds.y + scroll_) / lineH_);
    buf_.line = size_t(std::clamp(li, 0, int(buf_.lines.size()) - 1));
    buf_.col = std::min(buf_.lines[buf_.line].size(), size_t(std::max(0.0f, (x - bounds.x - 66) / 8.0f)));
    requestFocus();
    return true;
  }
  dragging_ = false;
  return true;
}

} // namespace vidra::ui
