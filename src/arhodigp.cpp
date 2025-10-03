#include "arhodigp.hpp"

#include <argp.h>

#include <ranges>
#include <string>
#include <vector>

static std::string g_version;
const char* argp_program_version;

static std::string g_contactAddress;
const char* argp_program_bug_address;

namespace arhodigp {

namespace {

inline auto parserForOption( int _key, char* _value, argp_state* _state )
    -> error_t {
    error_t l_returnValue = 0;

    using options_t = std::map< int, option_t >;

    gsl::not_null< options_t* > l_options =
        std::bit_cast< options_t* >( _state->input );

    switch ( _key ) {
        // Not flag/ option
        case ARGP_KEY_ARG: {
            gsl::not_null< char* > l_value = _value;

            if ( l_options->contains( _key ) ) [[likely]] {
                auto l_callback = l_options->at( _key ).callback;

                state_t l_state = _state;

                if ( !l_callback( _key, l_value.get(), l_state ) ) {
                    error( l_state );
                }
            }

            break;
        }

        // After all keys were processed
        case ARGP_KEY_END: {
            // TODO: Implement
            break;
        }

        // Unknown key
        default: {
            if ( l_options->contains( _key ) ) {
                auto l_callback = l_options->at( _key ).callback;

                state_t l_state = _state;

                const std::string_view l_value =
                    ( ( _value ) ? ( _value ) : ( "" ) );

                if ( !l_callback( _key, l_value, l_state ) ) [[likely]] {
                    error( l_state );
                }

            } else {
                l_returnValue = ARGP_ERR_UNKNOWN;
            }
        }
    }

    return ( l_returnValue );
}

} // namespace

auto parseArguments( std::string_view _format,
                     std::span< const std::string_view > _arguments,
                     std::string_view _applicationIdentifier,
                     std::string_view _applicationDescription,
                     float _applicationVersion,
                     std::string_view _contactAddress,
                     std::map< int, option_t >& _options ) -> bool {
    bool l_returnValue = false;

    do {
        g_version = std::format( "{}", _applicationVersion );
        argp_program_version = g_version.c_str();

        g_contactAddress = std::format( "<{}>", _contactAddress );
        argp_program_bug_address = g_contactAddress.c_str();

        {
            std::vector< argp_option > l_options;

            // Generate options
            {
                for ( const auto& [ _key, _value ] : _options ) {
                    const auto l_cStringOrNullptr =
                        []( const std::string& _string ) -> const char* {
                        return ( ( _string.empty() ) ? ( nullptr )
                                                     : ( _string.c_str() ) );
                    };

                    l_options.emplace_back( argp_option{
                        .name = l_cStringOrNullptr( _value.name ),
                        .key = _key,
                        .arg = l_cStringOrNullptr( _value.argument ),
                        .flags = static_cast<
                            std::underlying_type_t< option_t::flag_t > >(
                            _value.flag ),
                        .doc = l_cStringOrNullptr( _value.documentation ),
                        .group = _value.group,
                    } );
                }

                // Empty means last option
                l_options.emplace_back( argp_option{} );
            }

            const std::string l_format = std::string( _format );
            const std::string l_description = std::format(
                "{} - {}", _applicationIdentifier, _applicationDescription );

            argp l_argumentParser = {
                .options = l_options.data(),
                .parser = parserForOption,
                .args_doc = l_format.c_str(),
                .doc = l_description.c_str(),
                .children = nullptr,
                .help_filter = nullptr,
                .argp_domain = nullptr,
            };

            // TODO: Improve
            const auto l_arguments1 =
                _arguments |
                std::views::transform(
                    []( std::string_view _argument ) -> std::string {
                        return ( std::string( _argument ) );
                    } ) |
                std::ranges::to< std::vector< std::string > >();

            const auto l_arguments2 =
                l_arguments1 |
                std::views::transform(
                    []( const std::string& _argument ) -> const char* {
                        return ( _argument.c_str() );
                    } ) |
                std::ranges::to< std::vector< const char* > >();

            if ( argp_parse( &l_argumentParser, _arguments.size(),
                             std::bit_cast< char** >( l_arguments2.data() ), 0,
                             nullptr, &_options ) != 0 ) {
                break;
            }
        }

        l_returnValue = true;
    } while ( false );

    return ( l_returnValue );
}

void error( const state_t& _state, const std::string& _message ) {
    argp_error( std::bit_cast< const argp_state* >( _state ), "%s",
                _message.c_str() );
}

} // namespace arhodigp
