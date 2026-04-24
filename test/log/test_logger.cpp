#include <catch2/catch_test_macros.hpp>
#include <zk/log/logger.hpp>

using namespace zk::log;

// Helper: resets singleton state before each test
struct LoggerFixture {
    LoggerFixture()  { Logger::instance().reset(); }
    ~LoggerFixture() { Logger::instance().reset(); }
};

// ---------------------------------------------------------------------------
// Level filtering — messages below min_level must NOT reach the sink
// ---------------------------------------------------------------------------

TEST_CASE_METHOD(LoggerFixture, "Level filter: message at or above min_level reaches sink", "[logger][level]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });
    Logger::instance().set_min_level(Level::Warn);

    Logger::instance().log<Level::Warn,  Domain::Core>("warn msg");
    Logger::instance().log<Level::Error, Domain::Core>("error msg");
    Logger::instance().log<Level::Fatal, Domain::Core>("fatal msg");

    REQUIRE(call_count == 3);
}

TEST_CASE_METHOD(LoggerFixture, "Level filter: message below min_level does NOT reach sink", "[logger][level]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });
    Logger::instance().set_min_level(Level::Warn);

    Logger::instance().log<Level::Trace, Domain::Core>("trace msg");
    Logger::instance().log<Level::Debug, Domain::Core>("debug msg");
    Logger::instance().log<Level::Info,  Domain::Core>("info msg");

    REQUIRE(call_count == 0);
}

TEST_CASE_METHOD(LoggerFixture, "Level filter: ZK_LOG macro respects min_level", "[logger][level][macro]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });
    Logger::instance().set_min_level(Level::Error);

    ZK_LOG(Level::Info,  Domain::UI, "below threshold");
    ZK_LOG(Level::Error, Domain::UI, "at threshold");

    REQUIRE(call_count == 1);
}

// ---------------------------------------------------------------------------
// Domain filtering — disabled domains must NOT call the sink
// ---------------------------------------------------------------------------

TEST_CASE_METHOD(LoggerFixture, "Domain filter: disabled domain does NOT call sink", "[logger][domain]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });
    Logger::instance().set_domain_filter(Domain::Render, false);

    Logger::instance().log<Level::Info, Domain::Render>("render msg");

    REQUIRE(call_count == 0);
}

TEST_CASE_METHOD(LoggerFixture, "Domain filter: enabled domain calls sink", "[logger][domain]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });
    Logger::instance().set_domain_filter(Domain::Render, false);

    Logger::instance().log<Level::Info, Domain::Core>("core msg");
    Logger::instance().log<Level::Info, Domain::UI>("ui msg");

    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(LoggerFixture, "Domain filter: re-enabling domain restores sink calls", "[logger][domain]") {
    int call_count = 0;
    Logger::instance().add_sink([&](const LogEntry&) { ++call_count; });

    Logger::instance().set_domain_filter(Domain::IO, false);
    Logger::instance().log<Level::Info, Domain::IO>("disabled");
    REQUIRE(call_count == 0);

    Logger::instance().set_domain_filter(Domain::IO, true);
    Logger::instance().log<Level::Info, Domain::IO>("enabled");
    REQUIRE(call_count == 1);
}

// ---------------------------------------------------------------------------
// Multiple sinks — all registered sinks receive the same LogEntry
// ---------------------------------------------------------------------------

TEST_CASE_METHOD(LoggerFixture, "Multiple sinks: both sinks receive the same entry", "[logger][sink]") {
    LogEntry captured_a{};
    LogEntry captured_b{};

    Logger::instance().add_sink([&](const LogEntry& e) { captured_a = e; });
    Logger::instance().add_sink([&](const LogEntry& e) { captured_b = e; });

    Logger::instance().log<Level::Warn, Domain::Event>("multi-sink test");

    REQUIRE(captured_a.level   == Level::Warn);
    REQUIRE(captured_a.domain  == Domain::Event);
    REQUIRE(captured_a.message == "multi-sink test");

    REQUIRE(captured_b.level   == captured_a.level);
    REQUIRE(captured_b.domain  == captured_a.domain);
    REQUIRE(captured_b.message == captured_a.message);
}

TEST_CASE_METHOD(LoggerFixture, "Multiple sinks: each sink is called exactly once per log call", "[logger][sink]") {
    int count_a = 0;
    int count_b = 0;

    Logger::instance().add_sink([&](const LogEntry&) { ++count_a; });
    Logger::instance().add_sink([&](const LogEntry&) { ++count_b; });

    Logger::instance().log<Level::Info, Domain::Parser>("msg");

    REQUIRE(count_a == 1);
    REQUIRE(count_b == 1);
}

TEST_CASE_METHOD(LoggerFixture, "Multiple sinks: filtered message reaches no sink", "[logger][sink]") {
    int count_a = 0;
    int count_b = 0;

    Logger::instance().add_sink([&](const LogEntry&) { ++count_a; });
    Logger::instance().add_sink([&](const LogEntry&) { ++count_b; });
    Logger::instance().set_min_level(Level::Fatal);

    Logger::instance().log<Level::Debug, Domain::Core>("filtered");

    REQUIRE(count_a == 0);
    REQUIRE(count_b == 0);
}
