#include "arhodigp.hpp"

#include "test.hpp"

using namespace arhodigp;

static auto makeSpanFromArgv( std::vector< std::string >& _storage )
    -> std::span< std::string_view > {
    static std::vector< std::string_view > l_tmp;
    l_tmp.clear();
    l_tmp.reserve( _storage.size() );
    for ( auto& l_s : _storage ) {
        l_tmp.emplace_back( l_s );
    }
    return { l_tmp.data(), l_tmp.size() };
}

TEST( ParseArguments, LongAndShortOptionCallsCallback ) {
    // Arrange
    std::vector< std::string > l_argv = { "program", "--input", "file1.txt",
                                          "-i", "file2.bin" };
    auto l_argsSpan = makeSpanFromArgv( l_argv );

    // Capture calls
    std::vector< std::pair< int, std::string > > l_calls;
    callback_t l_cb = [ & ]( int _key, std::string_view _value,
                             state_t /*_state*/ ) -> bool {
        l_calls.emplace_back( _key, std::string( _value ) );
        return ( true ); // important: don't trigger error()
    };

    option_t l_opt( "input", l_cb, "FILE", "Input files" );

    // put the option in the map under the short key 'i'
    std::map< int, option_t > l_options{
        { static_cast< int >( 'i' ), l_opt },
    };

    // Act
    std::string l_formatDoc = "--input FILE";
    bool l_ok = parseArguments( l_formatDoc, l_argsSpan, "myapp", "desc", 1.0f,
                                "me@example.com", l_options );

    // Assert
    EXPECT_TRUE( l_ok );
    // We expect two invocations: one for --input and one for -i
    ASSERT_EQ( l_calls.size(), 2u );
    // The values should be in the order they appeared on the command line
    EXPECT_EQ( l_calls[ 0 ].second, "file1.txt" );
    EXPECT_EQ( l_calls[ 1 ].second, "file2.bin" );
    // Keys should be equal to 'i'
    EXPECT_EQ( l_calls[ 0 ].first, static_cast< int >( 'i' ) );
    EXPECT_EQ( l_calls[ 1 ].first, static_cast< int >( 'i' ) );
}

TEST( ParseArguments, UnknownOptionTriggersError ) {
    // Arrange: pass an unknown option and provide no options map entries.
    std::vector< std::string > l_argv = { "program", "--i-do-not-exist", "x" };
    auto l_argsSpan = makeSpanFromArgv( l_argv );
    std::map< int, option_t > l_options; // empty

    // Act
    bool l_ok = false;

    EXPECT_DEATH(
        {
            l_ok = parseArguments( std::string( "--i-do-not-exist ARG" ),
                                   l_argsSpan, "app", "desc", 0.1f, "me@here",
                                   l_options );
        },
        ".*" );

    // Assert: parseArguments should detect argp_parse non-zero and return
    // false
    EXPECT_FALSE( l_ok );
}

TEST( ParseArguments, StringViewOverloadWorks ) {
    // Arrange
    std::vector< std::string > l_argv = { "program", "--flag" };
    auto l_argsSpan = makeSpanFromArgv( l_argv );

    bool l_called = false;
    callback_t l_cb = [ & ]( [[maybe_unused]] int _key,
                             [[maybe_unused]] std::string_view _value,
                             state_t ) -> bool {
        l_called = true;
        // _value may be empty for flag without arg
        return true;
    };

    option_t l_opt( "flag", l_cb );

    std::map< int, option_t > l_options{
        { static_cast< int >( 'f' ), l_opt },
    };

    // Act: call overload taking std::string_view for format
    bool l_ok = parseArguments( std::string_view( "--flag" ), l_argsSpan, "app",
                                "desc", 2.0f, "contact", l_options );

    // Assert
    EXPECT_TRUE( l_ok );
    EXPECT_TRUE( l_called );
}

TEST( ParseArgumentsDeath, CallbackReturningFalseTriggersError_ASSERT_DEATH ) {
    std::vector< std::string > l_argv = { "program", "--input", "file1.txt" };
    auto l_argsSpan = makeSpanFromArgv( l_argv );

    // callback returns false -> parserForOption will call error(_state) ->
    // argp_error -> abort (because of our override)
    callback_t l_cb = []( int /*_k*/, std::string_view /*_v*/,
                          state_t /*_s*/ ) -> bool { return false; };

    option_t l_opt( "input", l_cb, "FILE" );
    std::map< int, option_t > l_options{
        { static_cast< int >( 'i' ), l_opt },
    };

    // ASSERT_DEATH runs the statement in a child process and expects it to die.
    ASSERT_DEATH(
        {
            // calling parseArguments should end up calling argp_error -> abort
            parseArguments( std::string( "--input FILE" ), l_argsSpan, "app",
                            "desc", 1.0f, "me@example.com", l_options );
        },
        ".*" );
}

TEST( ParseArgumentsDeath, CallbackReturningFalseTriggersError_EXPECT_DEATH ) {
    std::vector< std::string > l_argv = { "program", "-i", "b" };
    auto l_argsSpan = makeSpanFromArgv( l_argv );

    // Another callback that returns false; this exercises the short-option
    // path.
    callback_t l_cb = []( int /*_k*/, std::string_view /*_v*/,
                          state_t /*_s*/ ) -> bool { return false; };

    option_t l_opt( "input", l_cb, "FILE" );
    std::map< int, option_t > l_options{
        { static_cast< int >( 'i' ), l_opt },
    };

    // EXPECT_DEATH is like ASSERT_DEATH but test continues on failure.
    EXPECT_DEATH(
        {
            parseArguments( std::string( "-i FILE" ), l_argsSpan, "app", "desc",
                            2.0f, "me@example.com", l_options );
        },
        ".*" );
}
