#pragma once

#include <format>
#include <functional>
#include <gsl/pointers>
#include <map>
#include <span>
#include <string_view>

namespace arhodigp {

using state_t = gsl::not_null< void* >;

using callback_t =
    std::function< bool( int _key, std::string_view _value, state_t _state ) >;

// Example
// {
//     .name = "input",
//     .key = 'i',
//     .arg = "[FILE...]",
//     .flags = option_t::flag_t::optional,
//     .doc = "Input files",
//     .group = 5,
// },
using option_t = struct option {
    using flag_t = enum class flag : uint8_t {
        none = 0,

        // The argument associated with this option is optional.
        optional = 0x1,

        // This option is an alias for the closest previous non-alias option.
        // This means that it will be displayed in the same help entry, and will
        // inherit fields other than NAME and KEY from the aliased option.
        alias = 0x4,
    };

    option( std::string_view _name,
            callback_t _callback,
            std::string_view _argument = "",
            std::string_view _documentation = "",
            flag_t _flag = flag_t::none,
            int _group = 0 )
        : name( _name ),
          argument( _argument ),
          flag( _flag ),
          documentation( _documentation ),
          group( _group ),
          callback( std::move( _callback ) ) {}

    option() = delete;
    option( const option& ) = default;
    option( option&& ) = default;
    ~option() = default;
    auto operator=( const option& ) -> option& = default;
    auto operator=( option&& ) -> option& = default;

    // The long option name. For more than one name for the
    // same option, you can use following options with the alias flag
    // set.
    std::string name;

    // If not empty, this is the name of the argument associated with this
    // option, which is required unless the optional flag is set.
    // [NAME] - optional
    // NAME - required
    // NAME... - at least one and more
    std::string argument;

    flag_t flag;

    // The documentation string for this option. If both NAME and KEY are 0,
    // This string will be printed outdented from the normal option column,
    // making it useful as a group header (it will be the first thing printed in
    // its group); in this usage, it's conventional to end the string with a
    // `:'.
    std::string documentation;

    // The group this option is in. In a long help message, options are sorted
    // alphabetically within each group, and the groups presented in the order
    // 0, 1, 2, ..., n, -m, ..., -2, -1. Every entry in an options array with
    // if this field 0 will inherit the group number of the previous entry, or
    // zero if it's the first one, unless its a group header (NAME and KEY both
    // 0), in which case, the previous entry + 1 is the default. Automagic
    // options such as --help are put into group -1.
    int group;

    callback_t callback;
};

// [NAME] - optional
// NAME - required
// NAME... - at least one and more
auto parseArguments( std::string_view _format,
                     std::span< const std::string_view > _arguments,
                     std::string_view _applicationIdentifier,
                     std::string_view _applicationDescription,
                     float _applicationVersion,
                     std::string_view _contactAddress,
                     std::map< int, option_t >& _options ) -> bool;

void error( const state_t& _state, const std::string& _message );

template < typename... Arguments >
void error( const state_t& _state,
            std::format_string< Arguments... > _format = "",
            Arguments&&... _arguments ) {
    const std::string l_message =
        std::format( _format, std::forward< Arguments >( _arguments )... );

    error( _state, l_message );
}

} // namespace arhodigp
