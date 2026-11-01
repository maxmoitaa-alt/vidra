#include "vidra/core/json.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace vidra::core {

const Json Json::kNull{};
Json::Json(bool b) : type_(Type::Bool), bool_(b) {}
Json::Json(double n) : type_(Type::Number), num_(n) {}
Json::Json(int n) : type_(Type::Number), num_(double(n)) {}
Json::Json(const char* s) : type_(Type::String), str_(s) {}
Json::Json(std::string s) : type_(Type::String), str_(std::move(s)) {}
Json Json::makeArray() { Json j; j.type_ = Type::Array; return j; }
Json Json::makeObject() { Json j; j.type_ = Type::Object; return j; }

namespace {
struct P {
  const std::string& s; size_t i = 0; std::string err;
  void ws() { while (i < s.size() && std::isspace((unsigned char)s[i])) ++i; }
  bool lit(const char* t) {
    size_t n = std::strlen(t);
    if (s.compare(i, n, t) != 0) return false;
    i += n; return true;
  }
  std::string str() {
    std::string o;
    if (s[i] != '"') { err = "aspas"; return o; }
    ++i;
    while (i < s.size() && s[i] != '"') {
      if (s[i] == '\\' && i + 1 < s.size()) {
        ++i;
        switch (s[i]) {
          case 'n': o += '\n'; break; case 't': o += '\t'; break; case 'r': o += '\r'; break;
          default: o += s[i];
        }
      } else o += s[i];
      ++i;
    }
    if (i < s.size()) ++i;
    return o;
  }
  Json value() {
    ws();
    if (i >= s.size()) { err = "fim"; return Json(); }
    if (s[i] == '{') return object();
    if (s[i] == '[') return array();
    if (s[i] == '"') return Json(str());
    if (lit("true")) return Json(true);
    if (lit("false")) return Json(false);
    if (lit("null")) return Json();
    size_t a = i;
    if (s[i] == '-') ++i;
    while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == 'e' ||
                            s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
    if (a == i) { err = "num"; return Json(); }
    try {
      size_t used = 0;
      const double n = std::stod(s.substr(a, i - a), &used);
      if (used != i - a || !std::isfinite(n)) { err = "numero invalido"; return Json(); }
      return Json(n);
    } catch (...) { err = "numero invalido"; return Json(); }
  }
  Json array() {
    Json a = Json::makeArray(); ++i; ws();
    if (i < s.size() && s[i] == ']') { ++i; return a; }
    for (;;) {
      a.push(value()); ws();
      if (i < s.size() && s[i] == ',') { ++i; continue; }
      if (i < s.size() && s[i] == ']') { ++i; break; }
      err = "array"; break;
    }
    return a;
  }
  Json object() {
    Json o = Json::makeObject(); ++i; ws();
    if (i < s.size() && s[i] == '}') { ++i; return o; }
    for (;;) {
      ws(); std::string k = str(); ws();
      if (i >= s.size() || s[i] != ':') { err = ":"; break; }
      ++i; o[k] = value(); ws();
      if (i < s.size() && s[i] == ',') { ++i; continue; }
      if (i < s.size() && s[i] == '}') { ++i; break; }
      err = "objeto"; break;
    }
    return o;
  }
};
void esc(std::string& o, const std::string& s) {
  o += '"';
  for (char c : s) {
    if (c == '"') o += "\\\"";
    else if (c == '\\') o += "\\\\";
    else if (c == '\n') o += "\\n";
    else if (c == '\t') o += "\\t";
    else o += c;
  }
  o += '"';
}
void dumpTo(std::string& o, const Json& j, int ind, int d) {
  const std::string pad(size_t(ind * (d + 1)), ' '), pe(size_t(ind * d), ' ');
  switch (j.type()) {
    case Json::Type::Null: o += "null"; break;
    case Json::Type::Bool: o += j.asBool() ? "true" : "false"; break;
    case Json::Type::Number: { char b[64]; std::snprintf(b, sizeof(b), "%.10g", j.asNumber()); o += b; break; }
    case Json::Type::String: esc(o, j.asString()); break;
    case Json::Type::Array: {
      o += "[\n";
      const auto& a = j.array();
      for (size_t i = 0; i < a.size(); ++i) {
        o += pad; dumpTo(o, a[i], ind, d + 1);
        if (i + 1 < a.size()) o += ',';
        o += '\n';
      }
      o += pe + "]"; break;
    }
    case Json::Type::Object: {
      o += "{\n";
      size_t i = 0;
      for (const auto& [k, v] : j.object()) {
        o += pad; esc(o, k); o += ": "; dumpTo(o, v, ind, d + 1);
        if (++i < j.size()) o += ',';
        o += '\n';
      }
      o += pe + "}"; break;
    }
  }
}
} // namespace

Json Json::parse(const std::string& t, std::string* e) {
  P p{t};
  Json j = p.value();
  p.ws();
  if (p.err.empty() && p.i != t.size()) p.err = "conteudo apos o JSON";
  if (e) *e = p.err;
  return j;
}
std::string Json::dump(int ind) const { std::string o; dumpTo(o, *this, ind, 0); return o; }
bool Json::asBool(bool d) const { return type_ == Type::Bool ? bool_ : d; }
double Json::asNumber(double d) const { return type_ == Type::Number ? num_ : d; }
int Json::asInt(int d) const { return type_ == Type::Number ? int(std::lround(num_)) : d; }
std::string Json::asString(const std::string& d) const { return type_ == Type::String ? str_ : d; }
Json& Json::operator[](const std::string& k) { if (type_ != Type::Object) { type_ = Type::Object; obj_.clear(); } return obj_[k]; }
const Json& Json::operator[](const std::string& k) const { auto it = obj_.find(k); return it == obj_.end() ? kNull : it->second; }
Json& Json::operator[](size_t i) { if (type_ != Type::Array) { type_ = Type::Array; arr_.clear(); } if (i >= arr_.size()) arr_.resize(i + 1); return arr_[i]; }
const Json& Json::operator[](size_t i) const { return i < arr_.size() ? arr_[i] : kNull; }
void Json::push(Json v) { if (type_ != Type::Array) { type_ = Type::Array; arr_.clear(); } arr_.push_back(std::move(v)); }
size_t Json::size() const { return type_ == Type::Array ? arr_.size() : obj_.size(); }
bool Json::has(const std::string& k) const { return obj_.count(k) != 0; }

} // namespace vidra::core
