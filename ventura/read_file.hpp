#ifndef VENTURA_READ_FILE_HPP
#define VENTURA_READ_FILE_HPP

#include <silicium/read.hpp>
#include <silicium/variant.hpp>
#include <ventura/file_size.hpp>
#include <ventura/open.hpp>

namespace ventura
{
	enum class read_file_problem
	{
		file_too_large_for_memory,
		concurrent_write_detected
	};

	SILICIUM_USE_RESULT
	inline Si::variant<std::vector<char>, boost::system::error_code, read_file_problem>
	read_file(Si::native_path_string name)
	{
		Si::error_or<Si::file_handle> const file = open_reading(name);
		if (file.is_error())
		{
			return file.error();
		}
		Si::error_or<Si::optional<boost::uint64_t>> const size = file_size(file.get().handle);
		if (size.is_error())
		{
			return size.error();
		}
		std::vector<char> content;
		if (size.get())
		{
			if (*size.get() > content.max_size())
			{
				return read_file_problem::file_too_large_for_memory;
			}
			content.resize(static_cast<std::size_t>(*size.get()));
			Si::error_or<std::size_t> const read = Si::read(file.get().handle, Si::make_contiguous_range(content));
			if (read.is_error())
			{
				return read.error();
			}
			if (read.get() != content.size())
			{
				return read_file_problem::concurrent_write_detected;
			}
			return content;
		}
		else
		{
			boost::throw_exception(std::logic_error("not implemented yet"));
		}
	}
}

#endif
