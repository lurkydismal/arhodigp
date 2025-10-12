<!-- :toc: macro -->
<!-- :toc-title: -->
<!-- :toclevels: 99 -->

# arhodigp <!-- omit from toc -->

> C++ 23 callback-driven wrapper around GNU `argp`.

## Table of Contents <!-- omit from toc -->

* [General Information](#general-information)
* [Technologies Used](#technologies-used)
* [Features](#features)
* [Example output](#example-output)
* [Setup](#setup)
* [Usage](#usage)
* [Project Status](#project-status)
* [Room for Improvement](#room-for-improvement)
* [License](#license)

## General Information

A C++ 23 wrapper for GNU `argp` that gives you a modern API around option descriptors
and callback handlers. It exposes a simple `option_t` type, a `callback_t` type and
a single entry point `parseArguments(...)` that wires everything together and produces
consistent help/ error behavior.

Why this exists:

* GNU `argp` is fine but low-level and C-style. I wanted a C++-friendly, callback-first interface that:
  * uses `std::string/ std::string_view`, `std::span`, and `std::map` for clarity,
  * encourages small, testable callbacks,
  * keeps help and grouping semantics from argp but expressed in idiomatic C++ .

If you like predictable CLIs and hate writing argument parsing glue by hand, this does that job and nothing else.

## Technologies Used

<!--
clang version 20.1.8
Target: x86_64-pc-linux-gnu
Thread model: posix

Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
See https://llvm.org/LICENSE.txt for license information.
-->
* clang 20.1.8
* GNU `argp` (part of glibc) 2.42
* GSL for `gsl::not_null`

## Features

List of features:

* `option_t`
  * `name`, `argument`, `flag`, `documentation`, `group`, `callback`
  * flags: `none`, `optional`, `alias`
* `callback_t` — `std::function< bool( int key, std::string_view value, state_t state ) >`
  * return `true` on success, `false` on failure
* `parseArguments(...)` — single function to wire application metadata and the options map into `argp`
  * supports grouping and automatic help generation
* `error(...)` helpers for consistent, centralized error reporting
* Works with `std::span< const std::string_view >` (easy to integrate with `main( int _argumentCount, char** _argumentVector )`

## Example output

```bash
Usage: app [OPTION...]
app_identifier - description

  -v, --Verboe output
  -?, --help                 Give this help list
  --usage                    Give a short usage message
  -V, --version              Print program version

Report bugs to <@example.com>.
```

## Setup

Requirements/ dependencies:

* Linux with glibc (`argp` is a GNU extension — not portable to all platforms)
* A C++23-capable compiler
* A GSL implementation exposing `gsl::not_null` (Microsoft GSL or `gsl-lite`)

See `config.sh` for what to include and what to compile

## Usage

Overview of the API (already defined in the header):

```cpp
namespace arhodigp {
    using state_t = gsl::not_null<void*>;
    using callback_t = std::function<bool(int _key, std::string_view _value, state_t _state)>;

    struct option_t {
        enum class flag : uint8_t { none, optional, alias };
        // Fields:
        std::string name;          // Long option name
        std::string argument;      // "" | "[NAME]" | "NAME" | "NAME..."
        flag flag;                 // Optional/ alias semantics
        std::string documentation; // The documentation string for this option;
                                   // ( NAME + KEY == 0 ) => group header
        int group;                 // Grouping number
        callback_t callback;       // Called when option parsed
    };

    bool parseArguments(std::string_view _format,
                        std::span<const std::string_view> _arguments,
                        std::string_view _applicationIdentifier,
                        std::string_view _applicationDescription,
                        float _applicationVersion,
                        std::string_view _contactAddress,
                        std::map<int, option_t>& _options);

    void error(const state_t& _state, const std::string& _message);
    template<typename... Arguments>
    void error(const state_t& _state,
               std::format_string<Arguments...> _format = "",
               Arguments&&... _arguments);
}
```

Example usage (complete, pragmatic):

```cpp
#include <map>
#include <span>
#include <string_view>
#include <vector>
#include <ranges>
#include <algorithm>

#include "arhodigp.hpp"

auto main( int _argumentCount, char** _argumentVector ) -> int {
    // Parse arguments
    {
        auto l_argumentVector =
            std::span( _argumentVector, _argumentCount ) |
            std::ranges::views::transform(
                []( const char* _argument ) -> std::string_view {
                    return ( _argument );
                } ) |
            std::ranges::to< std::vector >();

        // Can be empty {}
        std::map< int, arhodigp::option_t > l_options{
            {
                'v',
                arhodigp::option_t(
                    "Verboe output",
                    []( [[maybe_unused]] int
                       _key /* Always 'v', more useful for aliases */,
                       [[maybe_unused]] std::string_view
                       _value /* Always "", if no argument required for option */,
                       [[maybe_unused]] arhodigp::state_t _state )
                    -> bool {
                        // arhodigp::error( _state, "Always fail" );
                        return ( true ); // Always success
                    },
                    "" /* Option arguments */,
                    "" /* Option documentation */,
                    arhodigp::option_t::flag_t::none ),
            },
        };

        if ( !arhodigp::parseArguments(
            "[FILE]", l_argumentVector, "app identifier", "app description",
            1.0 /* app version */, "contact address", l_options ) ) {
            return ( EXIT_FAILURE );
        }
    }

    /* ... */
}
```

Error reporting:

```cpp
arhodigp::error( _state, "Missing output file" );
arhodigp::error( _state, "Invalid value: {}", _value );
```

Notes on behavior:

* `option_t::flag::optional` marks the option's argument optional.
* `alias` makes the option an alias of the previous non-alias option (same help entry).
* If a callback returns `false`, parsing will treat that as a failure and `parseArguments` will return `false`.
* Use `group` numbers to organize help output. Group headers can be produced by an option entry with empty `NAME` and `KEY` (keeps `argp` semantics).

## Project Status

Project is: _in progress_.
Usable for standard CLI needs on glibc systems, but it's
intentionally minimal — not trying to be a full-featured
replacement for `boost::program_options` or `argparse` crates.

## Room for Improvement

Room for improvement:

* Implement callback call after all keys were processed.

To do:

* Add CI which compiles with Clang.
* Add a small example program in `examples/` demonstrating multi-value options and aliases.

## License

This project is open source and available under the
[GNU Affero General Public License v3.0](LICENSE).
