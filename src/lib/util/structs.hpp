#pragma once

#define DEFAULT_CTOR(name) name() = default
#define DEFAULT_CT_CTOR(name) constexpr name() = default
#define DEFAULT_DTOR(name) ~name() = default

#define DEFAULT_CT_CTOR_DTOR(name) \
    DEFAULT_CT_CTOR(name);         \
    DEFAULT_DTOR(name)

#define DEFAULT_CTOR_DTOR(name) \
    DEFAULT_CTOR(name);         \
    DEFAULT_DTOR(name)

#define NON_COPYABLE(name)                 \
    name(const name&) = delete;            \
    name& operator=(name const&) = delete; \
    name(name&&) = delete;                 \
    name& operator=(name&&) = delete

#define DEFAULT_COPY(name)                  \
    name(const name&) = default;            \
    name& operator=(name const&) = default; \
    name(name&&) noexcept = default;        \
    name& operator=(name&&) noexcept = default
