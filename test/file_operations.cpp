#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <ventura/file_operations.hpp>
#include <ventura/write_file.hpp>

#if VENTURA_HAS_ABSOLUTE_PATH_OPERATIONS

#ifdef _WIN32
#define SILICIUM_TEST_ROOT L"C:/"
#else
#define SILICIUM_TEST_ROOT "/"
#endif

namespace
{
    ventura::absolute_path const absolute_root = *ventura::absolute_path::create(SILICIUM_TEST_ROOT);
}

BOOST_AUTO_TEST_CASE(get_current_executable_path)
{
    Si::error_or<ventura::absolute_path> const p = ventura::get_current_executable_path();
    BOOST_REQUIRE(!p.is_error());
    auto const expected = "tests"
#ifdef _WIN32
                          ".exe"
#endif
        ;
    BOOST_CHECK_EQUAL(expected, p.get().to_boost_path().leaf());
}

BOOST_AUTO_TEST_CASE(get_current_executable_path_throw)
{
    ventura::absolute_path p = ventura::get_current_executable_path(Si::throw_);
    auto const expected = "tests"
#ifdef _WIN32
                          ".exe"
#endif
        ;
    BOOST_CHECK_EQUAL(expected, p.to_boost_path().leaf());
}

BOOST_AUTO_TEST_CASE(get_current_executable_path_variant)
{
    Si::error_or<ventura::absolute_path> const p = ventura::get_current_executable_path(Si::variant_);
    BOOST_REQUIRE(!p.is_error());
    auto const expected = "tests"
#ifdef _WIN32
                          ".exe"
#endif
        ;
    BOOST_CHECK_EQUAL(expected, p.get().to_boost_path().leaf());
}

BOOST_AUTO_TEST_CASE(test_file_exists_true)
{
    Si::error_or<bool> const exists = ventura::file_exists(absolute_root);
    BOOST_CHECK(exists.get());
}

BOOST_AUTO_TEST_CASE(test_file_exists_true_throw)
{
    bool exists = ventura::file_exists(absolute_root, Si::throw_);
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_file_exists_true_variant)
{
    Si::error_or<bool> const exists = ventura::file_exists(absolute_root, Si::variant_);
    BOOST_CHECK(exists.get());
}

BOOST_AUTO_TEST_CASE(test_file_exists_false)
{
    Si::error_or<bool> const exists =
        ventura::file_exists(absolute_root / *ventura::path_segment::create("does-not-exist"));
    BOOST_CHECK(!exists.get());
}

BOOST_AUTO_TEST_CASE(test_file_exists_false_throw)
{
    bool exists = ventura::file_exists(absolute_root / *ventura::path_segment::create("does-not-exist"), Si::throw_);
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_file_exists_false_variant)
{
    Si::error_or<bool> const exists =
        ventura::file_exists(absolute_root / *ventura::path_segment::create("does-not-exist"), Si::variant_);
    BOOST_CHECK(!exists.get());
}

BOOST_AUTO_TEST_CASE(test_rename)
{
    ventura::absolute_path const from = ventura::temporary_directory().get() / ventura::unique_path();
    ventura::create_file(from).move_value();
    ventura::absolute_path const to = ventura::temporary_directory().get() / ventura::unique_path();
    BOOST_CHECK(!ventura::file_exists(to).get());
    boost::system::error_code const error = ventura::rename(from, to);
    BOOST_CHECK(ventura::file_exists(to).get());
    BOOST_CHECK(!error);
}

BOOST_AUTO_TEST_CASE(test_temporary_directory)
{
    Si::error_or<ventura::absolute_path> const p = ventura::temporary_directory();
    BOOST_REQUIRE(!p.is_error());
    BOOST_CHECK(!p.get().empty());
}

BOOST_AUTO_TEST_CASE(test_temporary_directory_throw)
{
    ventura::absolute_path const p = ventura::temporary_directory(Si::throw_);
    BOOST_CHECK(!p.empty());
}

BOOST_AUTO_TEST_CASE(test_temporary_directory_variant)
{
    Si::error_or<ventura::absolute_path> const p = ventura::temporary_directory(Si::variant_);
    BOOST_REQUIRE(!p.is_error());
    BOOST_CHECK(!p.get().empty());
}

BOOST_AUTO_TEST_CASE(test_get_home)
{
    ventura::absolute_path const home = ventura::get_home();
#ifdef _WIN32
    BOOST_CHECK(boost::algorithm::starts_with(to_os_string(home), "C:/Users/"));
    BOOST_CHECK(boost::algorithm::ends_with(to_os_string(home), "/AppData/Local"));
#elif defined(__linux__)
    BOOST_CHECK(boost::algorithm::starts_with(ventura::to_os_string(home), "/home/"));
#else
    // OSX
    BOOST_CHECK(boost::algorithm::starts_with(to_os_string(home), "/Users/"));
#endif
}

BOOST_AUTO_TEST_CASE(test_copy_recursively_exists)
{
    ventura::absolute_path const temp =
        ventura::temporary_directory(Si::throw_) / ventura::relative_path("silicium-test_copy_recursively");
    ventura::recreate_directories(temp, Si::throw_);
    ventura::absolute_path const from = temp / ventura::relative_path("from");
    ventura::absolute_path const to = temp / ventura::relative_path("to");
    ventura::create_directories(from, Si::throw_);
    auto const expected = Si::make_c_str_range("Hello");
    Si::throw_if_error(ventura::write_file(
        Si::native_path_string(to_os_string(from / ventura::relative_path("file.txt")).c_str()), expected));
    ventura::create_directories(to, Si::throw_);
    ventura::copy_recursively(from, to, nullptr, Si::throw_);
    BOOST_REQUIRE(ventura::file_exists(to).get());
    std::ifstream file((to / ventura::relative_path("file.txt")).c_str(), std::ios::binary);
    BOOST_REQUIRE(file);
    std::vector<char> const file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    BOOST_CHECK_EQUAL_COLLECTIONS(file_content.begin(), file_content.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_copy_recursively_does_not_exist)
{
    ventura::absolute_path const temp =
        ventura::temporary_directory(Si::throw_) / ventura::relative_path("silicium-test_copy_recursively");
    ventura::recreate_directories(temp, Si::throw_);
    ventura::absolute_path const from = temp / ventura::relative_path("from");
    ventura::absolute_path const to = temp / ventura::relative_path("to");
    ventura::create_directories(from, Si::throw_);
    auto const expected = Si::make_c_str_range("Hello");
    Si::throw_if_error(ventura::write_file(
        Si::native_path_string(to_os_string(from / ventura::relative_path("file.txt")).c_str()), expected));
    BOOST_REQUIRE(!ventura::file_exists(to).get());
    ventura::copy_recursively(from, to, nullptr, Si::throw_);
    BOOST_REQUIRE(ventura::file_exists(to).get());
    std::ifstream file((to / ventura::relative_path("file.txt")).c_str(), std::ios::binary);
    BOOST_REQUIRE(file);
    std::vector<char> const file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    BOOST_CHECK_EQUAL_COLLECTIONS(file_content.begin(), file_content.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_copy_recursively_parent_does_not_exist)
{
    ventura::absolute_path const temp =
        ventura::temporary_directory(Si::throw_) / ventura::relative_path("silicium-test_copy_recursively");
    ventura::recreate_directories(temp, Si::throw_);
    ventura::absolute_path const from = temp / ventura::relative_path("from");
    ventura::absolute_path const to = temp / ventura::relative_path("parent/to");
    ventura::create_directories(from, Si::throw_);
    auto const expected = Si::make_c_str_range("Hello");
    Si::throw_if_error(ventura::write_file(
        Si::native_path_string(to_os_string(from / ventura::relative_path("file.txt")).c_str()), expected));
    BOOST_REQUIRE(!ventura::file_exists(to).get());
    ventura::copy_recursively(from, to, nullptr, Si::throw_);
    BOOST_REQUIRE(ventura::file_exists(to).get());
    std::ifstream file((to / ventura::relative_path("file.txt")).c_str(), std::ios::binary);
    BOOST_REQUIRE(file);
    std::vector<char> const file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    BOOST_CHECK_EQUAL_COLLECTIONS(file_content.begin(), file_content.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_cursor_position)
{
    Si::file_handle file =
        ventura::overwrite_file(
            ventura::safe_c_str(ventura::to_native_range(ventura::temporary_directory(Si::throw_) /
                                                         ventura::relative_path("test_cursor_position"))))
            .move_value();
    BOOST_REQUIRE_EQUAL(0u, ventura::cursor_position(file.handle).get());
    Si::write(file.handle, Si::make_c_str_range("0123456789")).get();
    BOOST_REQUIRE_EQUAL(10u, ventura::cursor_position(file.handle).get());
    Si::throw_if_error(ventura::seek_absolute(file.handle, 4));
    BOOST_REQUIRE_EQUAL(4u, ventura::cursor_position(file.handle).get());
    Si::write(file.handle, Si::make_c_str_range("abc")).get();
    BOOST_REQUIRE_EQUAL(7u, ventura::cursor_position(file.handle).get());
    Si::throw_if_error(ventura::seek_absolute(file.handle, 0));
    std::vector<char> const content = ventura::read_file(file.handle).get();
    std::string const expected = "0123abc789";
    BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), content.begin(), content.end());
}

BOOST_AUTO_TEST_CASE(test_copy)
{
    auto to = ventura::temporary_directory(Si::throw_) / ventura::relative_path("to.txt");
    auto from = ventura::temporary_directory(Si::throw_) / ventura::relative_path("from.txt");
    Si::throw_if_error(
        ventura::write_file(ventura::safe_c_str(ventura::to_native_range(from)), Si::make_c_str_range("new")));
    ventura::copy(from, to, Si::throw_);
    auto written = ventura::read_file(to).get();
    std::string const expected("new");
    BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), written.begin(), written.end());
}

BOOST_AUTO_TEST_CASE(test_copy_exists)
{
    auto to = ventura::temporary_directory(Si::throw_) / ventura::relative_path("to.txt");
    Si::throw_if_error(
        ventura::write_file(ventura::safe_c_str(ventura::to_native_range(to)), Si::make_c_str_range("old")));
    auto from = ventura::temporary_directory(Si::throw_) / ventura::relative_path("from.txt");
    Si::throw_if_error(
        ventura::write_file(ventura::safe_c_str(ventura::to_native_range(from)), Si::make_c_str_range("new")));
    ventura::copy(from, to, Si::throw_);
    auto written = ventura::read_file(to).get();
    std::string const expected("new");
    BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), written.begin(), written.end());
}
#endif
