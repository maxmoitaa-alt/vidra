#include "vidra/runtime/vm.hpp"
#include "vidra/core/log.hpp"
#include <chrono>
#include <luacode.h>
#include <lualib.h>

namespace vidra::runtime {
namespace {

thread_local Vm* g_vm = nullptr;
thread_local long g_left = 0;
thread_local bool g_expired = false;

void interrupt(lua_State* L, int) {
  if (g_expired) return;
  if (--g_left <= 0) {
    g_expired = true;
    lua_callbacks(L)->interrupt = nullptr;
    luaL_error(L, "limite de instrucoes excedido");
  }
}

int apiPrint(lua_State* L) {
  std::string out;
  const int n = lua_gettop(L);
  for (int i = 1; i <= n; ++i) {
    size_t len = 0;
    const char* s = luaL_tolstring(L, i, &len);
    if (i > 1) out += '\t';
    out.append(s, len);
    lua_pop(L, 1);
  }
  if (g_vm) VIDRA_LOGI("%s", out.c_str());
  return 0;
}

int apiWait(lua_State* L) {
  const double s = luaL_optnumber(L, 1, 0.0);
  lua_pushnumber(L, s);
  return 1;
}

int apiSpawn(lua_State* L) {
  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_pushvalue(L, 1);
  lua_call(L, 0, 0);
  return 0;
}

} // namespace

struct Vm::Impl {
  lua_State* L = nullptr;
};

Vm::Vm(Limits l) : impl_(new Impl), lim_(l) {
  impl_->L = luaL_newstate();
  luaL_openlibs(impl_->L);
  luaL_sandbox(impl_->L);
  lua_register(impl_->L, "print", apiPrint);
  lua_newtable(impl_->L);
  lua_setglobal(impl_->L, "workspace");
  lua_newtable(impl_->L);
  lua_setglobal(impl_->L, "game");
  lua_newtable(impl_->L);
  lua_pushcfunction(impl_->L, apiWait);
  lua_setfield(impl_->L, -2, "wait");
  lua_pushcfunction(impl_->L, apiSpawn);
  lua_setfield(impl_->L, -2, "spawn");
  lua_setglobal(impl_->L, "task");
  th_ = std::thread([this] { loop(); });
}

Vm::~Vm() {
  {
    std::lock_guard<std::mutex> g(mu_);
    down_ = true;
  }
  cv_.notify_all();
  if (th_.joinable()) th_.join();
  if (impl_->L) lua_close(impl_->L);
}

void Vm::stop() { stop_ = true; }

Result Vm::execute(const std::string& src) {
  {
    std::lock_guard<std::mutex> g(mu_);
    q_.push_back(src);
  }
  cv_.notify_all();
  Result r{};
  r.ok = true;
  return r;
}

void Vm::loop() {
  for (;;) {
    std::string src;
    {
      std::unique_lock<std::mutex> lk(mu_);
      cv_.wait(lk, [this] { return down_ || !q_.empty(); });
      if (down_ && q_.empty()) return;
      src = std::move(q_.front());
      q_.erase(q_.begin());
    }
    busy_ = true;
    stop_ = false;
    g_vm = this;
    g_left = lim_.instructions;
    g_expired = false;
    const auto t0 = std::chrono::steady_clock::now();
    Result r{};
    size_t bc = 0;
    char* code = luau_compile(src.c_str(), src.size(), nullptr, &bc);
    if (!code) {
      r.error = "compilacao falhou";
    } else {
      lua_callbacks(impl_->L)->interrupt = interrupt;
      const int st = luau_load(impl_->L, "=chunk", code, bc, 0);
      free(code);
      if (st != 0) {
        r.error = lua_tostring(impl_->L, -1) ? lua_tostring(impl_->L, -1) : "load";
        lua_pop(impl_->L, 1);
      } else if (lua_pcall(impl_->L, 0, 0, 0) != 0) {
        r.error = lua_tostring(impl_->L, -1) ? lua_tostring(impl_->L, -1) : "erro";
        lua_pop(impl_->L, 1);
      } else {
        r.ok = true;
      }
      lua_callbacks(impl_->L)->interrupt = nullptr;
    }
    r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (out_) out_(r.ok, r.ok ? "ok" : ("erro: " + r.error));
    core::VIDRA_LOGI("vm %.2fms ok=%d", r.ms, int(r.ok));
    busy_ = false;
  }
}

} // namespace vidra::runtime
