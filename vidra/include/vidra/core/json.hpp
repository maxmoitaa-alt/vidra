#pragma once
#include <map>
#include <string>
#include <vector>

namespace vidra::core {

class Json {
 public:
  enum class Type { Null, Bool, Number, String, Array, Object };
  Json() = default;
  Json(bool b); Json(double n); Json(int n); Json(const char* s); Json(std::string s);
  static Json parse(const std::string& t, std::string* err = nullptr);
  static Json makeArray();
  static Json makeObject();
  std::string dump(int indent = 2) const;
  bool asBool(bool d = false) const;
  double asNumber(double d = 0) const;
  int asInt(int d = 0) const;
  std::string asString(const std::string& d = "") const;
  Json& operator[](const std::string& k);
  const Json& operator[](const std::string& k) const;
  Json& operator[](size_t i);
  const Json& operator[](size_t i) const;
  void push(Json v);
  size_t size() const;
  bool has(const std::string& k) const;
  bool isObject() const { return type_ == Type::Object; }
  bool isArray() const { return type_ == Type::Array; }
  const std::map<std::string, Json>& object() const { return obj_; }
  const std::vector<Json>& array() const { return arr_; }
  Type type() const { return type_; }

 private:
  Type type_ = Type::Null;
  bool bool_ = false;
  double num_ = 0;
  std::string str_;
  std::vector<Json> arr_;
  std::map<std::string, Json> obj_;
  static const Json kNull;
};

} // namespace vidra::core
