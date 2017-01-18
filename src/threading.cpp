#include "threading.h"

LuaThread::LuaThread(const sol::function& function, const sol::variadic_args& args) noexcept
{
    // 1. Dump function to string
    sol::state_view lua(function.lua_state());
    str_function_ = lua["string"]["dump"](function);

    // 2. Create new state
    p_state_.reset(new sol::state);
    assert(p_state_.get() != NULL);
    p_state_->open_libraries(
        sol::lib::base, sol::lib::string,
        sol::lib::package, sol::lib::io, sol::lib::os
    );
    auto thread_table = core::init_state(*p_state_);
    (void)thread_table;

    // 3. Save parameters
    validate_args(args);

    // 4. Run thread
    p_thread_.reset(new std::thread(&LuaThread::work, this));
    assert(p_thread_.get() != NULL);
}

LuaThread::~LuaThread()
{
    join();
}

void LuaThread::validate_args(const sol::variadic_args& args) noexcept
{
    const auto end = --args.end();
    for(auto iter = args.begin(); iter != end; iter++)
    {
        core::StoredObject store(iter->get<sol::object>());
        arguments_.push_back(store.unpack(sol::this_state{p_state_->lua_state()}));
    }
}

void LuaThread::join() noexcept
{
    if (p_thread_.get())
    {
        p_thread_->join();
        p_thread_.reset();
    }
    arguments_.clear();
    if (p_state_.get())
        p_state_.reset();
}

void LuaThread::work() noexcept
{
    sol::state& lua = *p_state_;
    sol::function_result func = lua["loadstring"](str_function_);
    func.get<sol::function>()(sol::as_args(arguments_));
}

std::string LuaThread::thread_id() noexcept
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}
