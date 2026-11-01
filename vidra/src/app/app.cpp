#include "vidra/app/app.hpp"
#include "vidra/core/log.hpp"
#include "vidra/core/paths.hpp"
#include "vidra/motion/spring.hpp"
#include "vidra/ui/editor.hpp"
#include "vidra/ui/screens.hpp"
#include "vidra/ui/widgets.hpp"
#include <algorithm>
#include <cstdlib>

namespace vidra::app {

namespace {
const char* glyphFor(platform::Role r) {
  switch (r) {
    case platform::Role::Settings: return "settings";
    case platform::Role::Console: return "terminal";
    case platform::Role::Explorer: return "folder";
    case platform::Role::About: return "info";
    case platform::Role::Appearance: return "palette";
    case platform::Role::Editor: return "edit";
    default: return "home";
  }
}
}

App::App() : st_(core::Settings::get()) {
  core::ensureDirs();
  st_.load();
  pal_ = color::buildPalette(st_.seed, st_.dark);
}

App::~App() {
  saveStates();
  st_.save();
}

void App::saveStates() {
  auto set = [](core::WindowState& ws, Slot& s) {
    if (!s.win) return;
    const auto g = s.win->geometry();
    ws = {g.x, g.y, g.w, g.h, true};
  };
  if (slots_.count(platform::Role::Main)) set(st_.main, slots_[platform::Role::Main]);
  if (slots_.count(platform::Role::Settings)) set(st_.settings, slots_[platform::Role::Settings]);
  if (slots_.count(platform::Role::Console)) set(st_.console, slots_[platform::Role::Console]);
  if (slots_.count(platform::Role::Editor)) set(st_.editor, slots_[platform::Role::Editor]);
  if (slots_.count(platform::Role::Explorer)) set(st_.explorer, slots_[platform::Role::Explorer]);
  if (slots_.count(platform::Role::Appearance)) set(st_.appearance, slots_[platform::Role::Appearance]);
}

void App::loadStates() {}

void App::open(platform::Role r) {
  if (slots_.count(r)) { slots_[r].win->raise(); return; }
  const core::WindowState& ws = r == platform::Role::Settings ? st_.settings
                                : r == platform::Role::Console ? st_.console
                                : r == platform::Role::Editor ? st_.editor
                                : r == platform::Role::Explorer ? st_.explorer
                                : r == platform::Role::Appearance ? st_.appearance : st_.main;
  const int w = ws.w > 200 ? ws.w : 1100;
  const int h = ws.h > 200 ? ws.h : 760;
  Slot s;
  try {
    s.win = platform::createWindow(r, std::string("Vidra — ") + glyphFor(r), w, h);
  } catch (const std::exception& e) {
    VIDRA_LOGE("janela: %s", e.what());
    return;
  }
  if (ws.x >= 0 && ws.y >= 0) s.win->move(ws.x, ws.y);
  s.w = w; s.h = h;
  s.text = std::make_unique<gfx::TextEngine>();
  std::string ferr;
  for (const char* p : {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                        "/usr/share/fonts/TTF/DejaVuSans.ttf",
                        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
                        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"}) {
    if (s.text->load(p, &ferr)) break;
  }
  if (!s.text->ready()) VIDRA_LOGW("nenhuma fonte encontrada");
  gfx::BackendKind kind = gfx::BackendKind::OpenGL;
#ifdef VIDRA_HAVE_VULKAN
  const char* forced = std::getenv("VIDRA_RENDERER");
  if (!forced || std::string(forced) != "gl") kind = gfx::BackendKind::Vulkan;
#endif
  s.ren.reset(gfx::createRenderer(kind));
  std::string err;
  if (s.ren && s.ren->init(s.win->nativeDisplay(), s.win->nativeHandle(), w, h, &err)) {
    VIDRA_LOGI("renderer %s", kind == gfx::BackendKind::Vulkan ? "vulkan" : "opengl");
  } else {
    VIDRA_LOGW("vulkan indisponivel (%s), caindo para OpenGL", err.c_str());
    s.ren.reset(gfx::createRenderer(gfx::BackendKind::OpenGL));
    if (!s.ren->init(s.win->nativeDisplay(), s.win->nativeHandle(), w, h, &err)) {
      VIDRA_LOGE("OpenGL tambem falhou: %s", err.c_str());
      return;
    }
  }
  s.painter = std::make_unique<ui::Painter>(*s.ren, *s.text);
  s.root = std::make_unique<ui::Root>();

  switch (r) {
    case platform::Role::Main: buildMain(s); break;
    case platform::Role::Settings: buildSettings(s); break;
    case platform::Role::About: buildAbout(s); break;
    case platform::Role::Console: buildConsole(s); break;
    case platform::Role::Explorer: buildExplorer(s); break;
    case platform::Role::Appearance: buildAppearance(s); break;
    default: buildEditor(s, "script.luau", "print('ola')\n"); break;
  }
  slots_[r] = std::move(s);
  VIDRA_LOGI("janela aberta role=%d", int(r));
}

void App::close(platform::Role r) {
  if (auto it = slots_.find(r); it != slots_.end()) {
    if (it->second.ren) it->second.ren->shutdown();
    slots_.erase(it);
  }
}

void App::buildMain(Slot& s) {
  auto home = std::make_unique<ui::Home>(st_, vm_);
  home->onOpen = [this](const std::string& title, const std::string& src) {
    if (slots_.count(platform::Role::Editor)) close(platform::Role::Editor);
    open(platform::Role::Editor);
    if (slots_.count(platform::Role::Editor)) {
      if (auto* ed = dynamic_cast<ui::Editor*>(slots_[platform::Role::Editor].root->child())) {
        (void)title;
        ed->buffer().lines.clear();
        ed->buffer().lines.push_back("");
        ed->buffer().insert(src);
      }
    }
  };
  s.root->setChild(std::move(home));
}

void App::buildSettings(Slot& s) {
  auto view = std::make_unique<ui::SettingsView>(st_);
  view->onThemeChange = [this] {
    st_.dark = !st_.dark;
    pal_ = color::buildPalette(st_.seed, st_.dark);
    st_.save();
  };
  auto bar = std::make_unique<ui::Widget>();
  bar->layout({24, 300, 400, 48});
  s.root->setChild(std::move(view));
}

void App::buildAbout(Slot& s) { s.root->setChild(std::make_unique<ui::AboutView>()); }

void App::buildConsole(Slot& s) {
  auto c = std::make_unique<ui::ConsoleView>(vm_);
  auto btn = std::make_unique<ui::Button>("Limpar");
  auto* raw = c.get();
  btn->onClick = [raw] { (void)raw; };
  auto wrap = std::make_unique<ui::Widget>();
  wrap->add(std::move(btn));
  wrap->add(std::move(c));
  s.root->setChild(std::move(wrap));
}

void App::buildEditor(Slot& s, const std::string&, const std::string& src) {
  auto ed = std::make_unique<ui::Editor>();
  ed->buffer().lines.clear();
  ed->buffer().lines.push_back("");
  ed->buffer().insert(src);
  ui::Editor* raw = ed.get();
  ed->onSave = [raw](const std::string& path) {
    raw->buffer().save(path);
    VIDRA_LOGI("salvo %s", path.c_str());
  };
  s.root->setChild(std::move(ed));
}

void App::buildExplorer(Slot& s) { s.root->setChild(std::make_unique<ui::Explorer>()); }

void App::buildAppearance(Slot& s) {
  auto wrap = std::make_unique<ui::Widget>();
  for (size_t i = 0; i < tok::kSeeds.size(); ++i) {
    auto b = std::make_unique<ui::Button>(std::string(tok::kSeeds[i]));
    b->onClick = [this, i] {
      st_.seed = color::parseHex(std::string(tok::kSeeds[i]));
      pal_ = color::buildPalette(st_.seed, st_.dark);
      st_.save();
    };
    b->layout({24.0f + float(i) * 150.0f, 80, 140, 48});
    wrap->add(std::move(b));
  }
  s.root->setChild(std::move(wrap));
}

void App::tickSlot(platform::Role r, Slot& s, double dt) {
  s.win->pump([&](const platform::Event& e) {
    switch (e.type) {
      case platform::Event::Type::Close: close(r); break;
      case platform::Event::Type::Resize:
        s.w = int(e.x);
        s.h = int(e.y);
        if (s.ren && s.w > 0 && s.h > 0) s.ren->resize(s.w, s.h);
        break;
      case platform::Event::Type::Move: s.root->pointer(e.x, e.y, false, 0); break;
      case platform::Event::Type::Button: s.root->pointer(e.x, e.y, e.pressed, e.button); break;
      case platform::Event::Type::Key: {
        ui::Key k;
        k.code = e.key;
        k.text = e.text;
        k.shift = (e.mods & 1) != 0;
        k.ctrl = (e.mods & 4) != 0;
        if (!s.root->key(k, e.pressed) && e.pressed && r == platform::Role::Main) {
          if (k.code == 't' || k.code == 'T') {
            st_.dark = !st_.dark;
            pal_ = color::buildPalette(st_.seed, st_.dark);
            st_.save();
          }
          if (k.code == '1') open(platform::Role::Settings);
          if (k.code == '2') open(platform::Role::Console);
          if (k.code == '3') open(platform::Role::Explorer);
          if (k.code == '4') open(platform::Role::Appearance);
          if (k.code == '5') open(platform::Role::About);
        }
        break;
      }
      case platform::Event::Type::Focus: s.alive = true; break;
      default: break;
    }
  });

  const auto g = s.win->geometry();
  if (g.w != s.w || g.h != s.h) {
    s.w = g.w;
    s.h = g.h;
    s.ren->resize(s.w, s.h);
  }
  s.root->update(dt, st_.reduceMotion);
  s.text->upload(*s.ren);
  s.ren->setPalette(reinterpret_cast<const float*>(&pal_[color::Role::SurfaceTint]),
                    reinterpret_cast<const float*>(&pal_[color::Role::Surface]), 0.0f);
  s.ren->beginFrame(float(t_));
  s.root->paint(*s.painter, pal_);
  s.ren->endFrame();
}

int App::run() {
  open(platform::Role::Main);
  auto prev = std::chrono::steady_clock::now();
  while (running_ && !slots_.empty()) {
    const auto now = std::chrono::steady_clock::now();
    const double dt = std::chrono::duration<double>(now - prev).count();
    prev = now;
    t_ += dt;
    std::vector<platform::Role> dead;
    for (auto& [r, s] : slots_) {
      if (!s.win || s.win->shouldClose()) { dead.push_back(r); continue; }
      tickSlot(r, s, dt);
    }
    for (auto r : dead) close(r);
    if (crossfade_ < 1.0f) crossfade_ = std::min(1.0f, crossfade_ + float(dt) * 3.0f);
    if (!slots_.count(platform::Role::Main) && slots_.empty()) break;
  }
  return 0;
}

} // namespace vidra::app
