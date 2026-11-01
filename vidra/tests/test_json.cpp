#include "check.hpp"
#include "vidra/core/json.hpp"
int main() {
  using vidra::core::Json;
  std::string err;
  Json j = Json::parse("{\"a\":1,\"b\":[true,\"x\"]}", &err);
  CHECK(err.empty());
  CHECK_EQ(j["a"].asInt(), 1);
  CHECK(j["b"][0].asBool());
  CHECK_EQ(j["b"][1].asString(), "x");
  return report("json");
}
