#include "vidra/core/settings.hpp"
#include "vidra/core/json.hpp"
#include "vidra/core/log.hpp"
#include "vidra/core/paths.hpp"
#include <fstream>
#include <sstream>

namespace vidra::core {
namespace {

Json ws(const WindowState& w) {
  Json j;
  j["x"] = w.x; j["y"] = w.y; j["w"] = w.w; j["h"] = w.h; j["open"] = w.open;
  return j;
}
WindowState rs(const Json& j) {
  return {j["x"].asInt(-1), j["y"].asInt(-1), j["w"].asInt(0), j["h"].asInt(0), j["open"].asBool()};
}
std::vector<std::string> rv(const Json& j) {
  std::vector<std::string> out;
  for (const auto& e : j.array()) out.push_back(e.asString());
  return out;
}
Json wv(const std::vector<std::string>& v) {
  Json j;
  for (const auto& s : v) j.push(s);
  return j;
}

} // namespace

void Settings::load() {
  const auto path = configDir() / "settings.json";
  std::ifstream f(path);
  if (!f) return;
  std::stringstream ss;
  ss << f.rdbuf();
  std::string err;
  const Json j = Json::parse(ss.str(), &err);
  if (!err.empty()) { VIDRA_LOGW("settings invalido: %s", err.c_str()); return; }
  dark = j["dark"].asBool(dark);
  sound = j["sound"].asBool(sound);
  reduceMotion = j["reduceMotion"].asBool(reduceMotion);
  density = float(j["density"].asNumber(density));
  uiScale = float(j["uiScale"].asNumber(uiScale));
  seed = uint32_t(j["seed"].asNumber(double(seed)));
  renderer = j["renderer"].asString(renderer);
  autoScroll = j["autoScroll"].asBool(autoScroll);
  favorites = rv(j["favorites"]);
  history = rv(j["history"]);
  main = rs(j["main"]); settings = rs(j["settings"]); console = rs(j["console"]);
  editor = rs(j["editor"]); explorer = rs(j["explorer"]); appearance = rs(j["appearance"]);
  VIDRA_LOGI("settings carregados");
}

void Settings::save() const {
  Json j;
  j["dark"] = dark; j["sound"] = sound; j["reduceMotion"] = reduceMotion;
  j["density"] = double(density); j["uiScale"] = double(uiScale);
  j["seed"] = int(seed); j["renderer"] = renderer; j["autoScroll"] = autoScroll;
  j["favorites"] = wv(favorites); j["history"] = wv(history);
  j["main"] = ws(main); j["settings"] = ws(settings); j["console"] = ws(console);
  j["editor"] = ws(editor); j["explorer"] = ws(explorer); j["appearance"] = ws(appearance);
  std::ofstream f(configDir() / "settings.json");
  if (!f) { VIDRA_LOGW("nao gravou settings"); return; }
  f << j.dump();
}

Settings& Settings::get() {
  static Settings s;
  return s;
}

} // namespace vidra::core
