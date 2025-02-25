#pragma once

#include "../spa/dict.hpp"

#include "../utils/lazy.hpp"
#include "../utils/listener.hpp"

#include <string>
#include <memory>
#include <cstdint>

struct pw_proxy;

namespace pipewire
{
    class proxy_listener;

    class proxy
    {
        struct impl;

      public:
        using raw_type = pw_proxy;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        virtual ~proxy();

      public:
        proxy(proxy &&) noexcept;
        proxy(raw_type *, spa::dict);

      public:
        proxy &operator=(proxy &&) noexcept;

      public:
        [[nodiscard]] spa::dict props() const;
        [[nodiscard]] std::string type() const;
        [[nodiscard]] std::uint32_t id() const;
        [[nodiscard]] std::uint32_t version() const;

      public:
        [[nodiscard]] raw_type *get() const;

      public:
        template <class Listener = proxy_listener>
            requires valid_listener<Listener, raw_type>
        [[rk::needs_update]] [[nodiscard]] Listener listen();

      public:
        [[nodiscard]] operator raw_type *() const &;
        [[nodiscard]] operator raw_type *() const && = delete;

      public:
        [[rk::needs_update]] static lazy<expected<proxy>> bind(raw_type *);
    };
} // namespace pipewire
