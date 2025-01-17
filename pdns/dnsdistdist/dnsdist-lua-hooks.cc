
#include "dnsdist-lua-hooks.hh"
#include "dnsdist-lua.hh"
#include "lock.hh"
#include "tcpiohandler.hh"

namespace dnsdist::lua::hooks
{
using MaintenanceCallback = std::function<void()>;
using TicketsKeyAddedHook = std::function<void(const std::string&, size_t)>;

static LockGuarded<std::vector<MaintenanceCallback>> s_maintenanceHooks;

void runMaintenanceHooks(const LuaContext& context)
{
  (void)context;
  for (const auto& callback : *(s_maintenanceHooks.lock())) {
    callback();
  }
}

static void addMaintenanceCallback(const LuaContext& context, MaintenanceCallback callback)
{
  (void)context;
  s_maintenanceHooks.lock()->push_back(std::move(callback));
}

void clearMaintenanceHooks()
{
  s_maintenanceHooks.lock()->clear();
}

static void setTicketsKeyAddedHook(const LuaContext& context, const TicketsKeyAddedHook& hook)
{
  TLSCtx::setTicketsKeyAddedHook([hook](const std::string& key) {
    try {
      auto lua = g_lua.lock();
      hook(key, key.size());
    }
    catch (const std::exception& exp) {
      warnlog("Error calling the Lua hook after new tickets key has been added: %s", exp.what());
    }
  });
}

void setupLuaHooks(LuaContext& luaCtx)
{
  luaCtx.writeFunction("addMaintenanceCallback", [&luaCtx](const MaintenanceCallback& callback) {
    setLuaSideEffect();
    addMaintenanceCallback(luaCtx, callback);
  });
  luaCtx.writeFunction("setTicketsKeyAddedHook", [&luaCtx](const TicketsKeyAddedHook& hook) {
    setLuaSideEffect();
    setTicketsKeyAddedHook(luaCtx, hook);
  });
}

}
