#include "core/core.hpp"
#include "core/events.hpp"

#include "utils/check.hpp"
#include "registry/registry.hpp"

#include "link/link.hpp"
#include "node/node.hpp"
#include "proxy/proxy.hpp"

#include <format>
#include <pipewire/pipewire.h>

namespace pipewire
{
    struct core::impl
    {
        raw_type *core;

      public:
        std::shared_ptr<pipewire::context> context;
        std::shared_ptr<pipewire::registry> registry;
    };

    core::~core()
    {
        pw_core_disconnect(m_impl->core);
    }

    core::core() : m_impl(std::make_unique<impl>()) {}

    void *core::create(factory factory) const
    {
        const auto &[name, props, type, version] = factory;
        const auto *dict                         = &props.get()->dict;

        // NOLINTNEXTLINE(*-optional-access)
        return pw_core_create_object(get(), name.c_str(), type->c_str(), version.value(), dict, 0);
    }

    template <>
    void core::update<update_strategy::none>()
    {
    }

    template core_listener core::listen<core_listener>();

    template <>
    void core::update<update_strategy::sync>()
    {
        auto done    = false;
        auto pending = sync(0);

        auto listener = listen<core_listener>();
        auto loop     = m_impl->context->loop();

        listener.on<core_event::done>([&](auto id, auto seq) {
            if (id != PW_ID_CORE || seq != pending)
            {
                return;
            }

            done = true;
            loop->quit();
        });

        while (!done)
        {
            loop->run();
        }
    }

    void core::update(update_strategy strategy)
    {
        switch (strategy)
        {
        case update_strategy::none:
            return update<update_strategy::none>();
        case update_strategy::sync:
            return update<update_strategy::sync>();
        }
    }

    int core::sync(int seq)
    {
        return pw_core_sync(m_impl->core, PW_ID_CORE, seq);
    }

    template <class Listener>
        requires valid_listener<Listener, core::raw_type>
    Listener core::listen()
    {
        return {get()};
    }

    template <>
    lazy<expected<proxy>> core::create(factory factory, update_strategy strategy)
    {
        if (!factory.version.has_value() || !factory.type.has_value())
        {
            return make_lazy<expected<proxy>>([]() -> expected<proxy> {
                return tl::make_unexpected(error{.message = "Bad Factory"});
            });
        }

        auto *proxy = create(std::move(factory));
        auto rtn    = proxy::bind(reinterpret_cast<proxy::raw_type *>(proxy));

        update(strategy);

        return rtn;
    }

    template <>
    lazy<expected<link>> core::create(link_factory factory, update_strategy strategy)
    {
        auto props = properties{{"link.input.port", std::to_string(factory.input)},
                                {"link.output.port", std::to_string(factory.output)}};

        return create<link>({.name = "link-factory", .props = std::move(props)}, strategy);
    }

    template <>
    lazy<expected<node>> core::create(null_sink_factory factory, update_strategy strategy)
    {
        std::string positions;

        for (const auto &position : factory.positions)
        {
            positions += std::format("{},", position);
        }

        positions.pop_back();

        auto props = properties{{"node.name", factory.name},
                                {"media.class", "Audio/Source/Virtual"},
                                {"factory.name", "support.null-audio-sink"},
                                {"audio.channels", std::to_string(factory.positions.size())},
                                {"audio.position", positions}};

        return create<node>({.name = "adapter", .props = std::move(props)}, strategy);
    }

    std::shared_ptr<pipewire::registry> core::registry()
    {
        if (!m_impl->registry)
        {
            m_impl->registry = pipewire::registry::create(shared_from_this());
        }

        return m_impl->registry;
    }

    core::raw_type *core::get() const
    {
        return m_impl->core;
    }

    std::shared_ptr<pipewire::context> core::context() const
    {
        return m_impl->context;
    }

    core::operator raw_type *() const &
    {
        return get();
    }

    std::shared_ptr<core> core::create(std::shared_ptr<pipewire::context> context)
    {
        auto *core = pw_context_connect(context->get(), nullptr, 0);
        check(core, "Failed to connect core");

        if (!core)
        {
            return nullptr;
        }

        auto rtn = std::unique_ptr<pipewire::core>(new pipewire::core);

        rtn->m_impl->core    = core;
        rtn->m_impl->context = std::move(context);

        return rtn;
    }
} // namespace pipewire
