#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <silicium/environment_variables.hpp>
#include <silicium/sink/iterator_sink.hpp>
#include <ventura/run_process.hpp>
#include <ventura/file_operations.hpp>

#if VENTURA_HAS_ABSOLUTE_PATH_OPERATIONS && VENTURA_HAS_RUN_PROCESS
namespace
{
    void test_environment_variables(
        ventura::environment_inheritance const tested_inheritance,
        std::vector<std::pair<Si::os_char const *, Si::os_char const *>> const additional_variables)
    {
        std::vector<Si::os_string> arguments;
#ifdef _WIN32
        arguments.emplace_back(SILICIUM_SYSTEM_LITERAL("/C"));
        arguments.emplace_back(SILICIUM_SYSTEM_LITERAL("set"));
#endif

        BOOST_REQUIRE(!Si::set_environment_variable(SILICIUM_SYSTEM_LITERAL("silicium_parent_key"),
                                                    SILICIUM_SYSTEM_LITERAL("parent_value")));

        std::string output;
        auto output_sink = Si::Sink<char, Si::success>::erase(Si::make_container_sink(output));
        int const exit_code = ventura::run_process(*ventura::absolute_path::create(
#ifdef _WIN32
                                                       SILICIUM_SYSTEM_LITERAL("C:\\Windows\\System32\\cmd.exe")
#else
                                                       "/usr/bin/env"
#endif
                                                           ),
                                                   arguments, ventura::get_current_working_directory(Si::throw_),
                                                   output_sink, additional_variables, tested_inheritance)
                                  .get();
        BOOST_CHECK_EQUAL(0, exit_code);

        for (auto const &variable : additional_variables)
        {
            auto const needle = Si::to_utf8_string(variable.first) + '=' + Si::to_utf8_string(variable.second);
            BOOST_CHECK(std::search(output.begin(), output.end(), needle.begin(), needle.end()) != output.end());
        }

        auto const parent_key_found = output.find("silicium_parent_key=parent_value");
        switch (tested_inheritance)
        {
        case ventura::environment_inheritance::inherit:
            BOOST_CHECK_NE(std::string::npos, parent_key_found);
            break;

        case ventura::environment_inheritance::no_inherit:
            BOOST_CHECK_EQUAL(std::string::npos, parent_key_found);
            break;
        }
    }
}

namespace
{
    std::vector<std::pair<Si::os_char const *, Si::os_char const *>> const additional_variables =
        boost::assign::list_of(std::make_pair(SILICIUM_SYSTEM_LITERAL("key"), SILICIUM_SYSTEM_LITERAL("value")));
}

BOOST_AUTO_TEST_CASE(async_process_environment_variables_inherit_additional_vars)
{
    test_environment_variables(ventura::environment_inheritance::inherit, additional_variables);
}

BOOST_AUTO_TEST_CASE(async_process_environment_variables_no_inherit_additional_vars)
{
    test_environment_variables(ventura::environment_inheritance::no_inherit, additional_variables);
}

BOOST_AUTO_TEST_CASE(async_process_environment_variables_inherit)
{
    test_environment_variables(ventura::environment_inheritance::inherit,
                               std::vector<std::pair<Si::os_char const *, Si::os_char const *>>());
}

BOOST_AUTO_TEST_CASE(async_process_environment_variables_no_inherit)
{
    test_environment_variables(ventura::environment_inheritance::no_inherit,
                               std::vector<std::pair<Si::os_char const *, Si::os_char const *>>());
}
#endif
