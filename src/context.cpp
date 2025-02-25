#include "loop.hpp"
#include "context.hpp"
#include "core/core.hpp"
#include "utils/check.hpp"

#include <pipewire/pipewire.h>

namespace pipewire
{
    struct context::impl
    {
        raw_type *context;
        std::shared_ptr<main_loop> loop;
        std::shared_ptr<pipewire::core> core;
    };

    context::~context()
    {
        pw_context_destroy(m_impl->context);
    }

    context::context() : m_impl(std::make_unique<impl>()) {}

    std::shared_ptr<core> context::core()
    {
        if (!m_impl->core)
        {
            m_impl->core = core::create(shared_from_this());
        }

        return m_impl->core;
    }

    context::raw_type *context::get() const
    {
        return m_impl->context;
    }

    std::shared_ptr<main_loop> context::loop() const
    {
        return m_impl->loop;
    }

    context::operator raw_type *() const &
    {
        return get();
    }

    std::shared_ptr<context> context::create(std::shared_ptr<main_loop> loop)
    {
        auto *ctx = pw_context_new(loop->loop(), nullptr, 0);
        check(ctx, "Failed to create context");

        if (!ctx)
        {
            return nullptr;
        }

        auto rtn = std::unique_ptr<context>(new context);

        rtn->m_impl->context = ctx;
        rtn->m_impl->loop    = std::move(loop);

        return rtn;
    }
} // namespace pipewire
