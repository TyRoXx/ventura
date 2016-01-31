#include <ventura/read_file.hpp>
#include <ventura/write_file.hpp>
#include <ventura/file_operations.hpp>
#include <boost/test/unit_test.hpp>

#if SILICIUM_HAS_EXCEPTIONS
#include <boost/filesystem/operations.hpp>

namespace
{
	ventura::absolute_path test_root()
	{
		return ventura::get_current_working_directory(Si::throw_);
	}
}

BOOST_AUTO_TEST_CASE(read_file)
{
	Si::os_string const file = to_os_string(test_root() / "read_file.txt");
	Si::memory_range const expected_content = Si::make_c_str_range("Hello");
	Si::native_path_string const file_name(file.c_str());
	Si::throw_if_error(ventura::write_file(file_name, expected_content));
	std::vector<char> const read =
	    Si::visit<std::vector<char>>(ventura::read_file(file_name),
	                                 [](std::vector<char> content)
	                                 {
		                                 return content;
		                             },
	                                 [](boost::system::error_code ec) -> std::vector<char>
	                                 {
		                                 Si::throw_error(ec);
		                             },
	                                 [](ventura::read_file_problem problem) -> std::vector<char>
	                                 {
		                                 std::cerr << static_cast<int>(problem) << '\n';
		                                 BOOST_FAIL("unexpected read_file_problem");
		                                 SILICIUM_UNREACHABLE();
		                             });
	BOOST_CHECK_EQUAL_COLLECTIONS(expected_content.begin(), expected_content.end(), read.begin(), read.end());
}

#endif
