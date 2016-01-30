#ifndef VENTURA_FILE_OPERATIONS_HPP
#define VENTURA_FILE_OPERATIONS_HPP

#include <silicium/error_handler.hpp>
#include <ventura/run_process.hpp>
#include <ventura/file_size.hpp>
#include <ventura/open.hpp>
#include <silicium/read_file.hpp>
#include <silicium/identity.hpp>
#include <silicium/arithmetic/add.hpp>
#ifdef _WIN32
#include <silicium/win32/win32.hpp>
#include <Shellapi.h>
#include <shlobj.h>
#undef interface
#else
#include <pwd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Boost filesystem requires exceptions
#define VENTURA_HAS_ABSOLUTE_PATH_OPERATIONS SILICIUM_HAS_EXCEPTIONS

#if VENTURA_HAS_ABSOLUTE_PATH_OPERATIONS
#include <boost/filesystem/operations.hpp>
#endif

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace ventura
{
#if VENTURA_HAS_ABSOLUTE_PATH_OPERATIONS
	template <class ErrorHandler>
	inline auto get_current_working_directory(ErrorHandler &&handle_error)
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<absolute_path>()))
	{
		boost::system::error_code ec;
		auto result = boost::filesystem::current_path(ec);
		if (!ec)
		{
			return *absolute_path::create(std::move(result));
		}
		return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<absolute_path>());
	}

	template <class ErrorHandler>
	inline auto remove_file(absolute_path const &name, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
		boost::system::error_code ec;
		boost::filesystem::remove(name.to_boost_path(), ec);
		return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
	}

	template <class ErrorHandler>
	inline auto create_directories(absolute_path const &directories, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
		boost::system::error_code ec;
		boost::filesystem::create_directories(directories.to_boost_path(), ec);
		return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
	}

	template <class ErrorHandler>
	inline auto remove_all(absolute_path const &directories, ErrorHandler &&handle_error)
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<boost::uintmax_t>()))
	{
		boost::system::error_code ec;
		auto count = boost::filesystem::remove_all(directories.to_boost_path(), ec);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<boost::uintmax_t>());
		}
		return count;
	}

	namespace detail
	{
		SILICIUM_USE_RESULT
		inline boost::system::error_code recreate_directories(absolute_path const &directories)
		{
			boost::system::error_code error = create_directories(directories, Si::return_);
			if (!!error)
			{
				return error;
			}
			boost::filesystem::directory_iterator i(directories.to_boost_path(), error);
			if (!!error)
			{
				return error;
			}
			for (; i != boost::filesystem::directory_iterator();)
			{
				boost::filesystem::remove_all(i->path(), error);
				if (!!error)
				{
					return error;
				}
				i.increment(error);
				if (!!error)
				{
					return error;
				}
			}
			return error;
		}
	}

	template <class ErrorHandler>
	auto recreate_directories(absolute_path const &directories, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
		boost::system::error_code error = detail::recreate_directories(directories);
		return std::forward<ErrorHandler>(handle_error)(error, Si::identity<void>());
	}

	template <class ErrorHandler>
	auto copy(absolute_path const &from, absolute_path const &to, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
		boost::system::error_code ec;
		boost::filesystem::copy(from.to_boost_path(), to.to_boost_path(), ec);
		return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
	}

#ifdef _WIN32
	namespace detail
	{
		template <class String>
		std::vector<wchar_t> double_zero_terminate(String const &str)
		{
			std::vector<wchar_t> result(str.begin(), str.end());
			// wow, SHFileOperationW does not work with slashes
			std::replace(result.begin(), result.end(), L'/', L'\\');
			result.push_back(0);
			result.push_back(0);
			return result;
		}
	}
#endif

	template <class ErrorHandler>
	auto copy_recursively(absolute_path const &from, absolute_path const &to,
	                      Si::Sink<char, Si::success>::interface *output, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
#ifdef _WIN32
		boost::system::error_code ec = create_directories(to, Si::return_);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
		}

		boost::filesystem::directory_iterator i(from.to_boost_path(), ec);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
		}

		for (; i != boost::filesystem::directory_iterator();)
		{
			switch (i->status().type())
			{
			case boost::filesystem::directory_file:
			{
				boost::filesystem::path leaf = i->path().leaf();
				ec = copy_recursively(*absolute_path::create(i->path()), to / relative_path(leaf), output, Si::return_);
				if (!!ec)
				{
					return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
				}
				break;
			}

			case boost::filesystem::regular_file:
			{
				boost::filesystem::path leaf = i->path().leaf();
				boost::filesystem::copy_file(i->path(), to.to_boost_path() / leaf,
				                             boost::filesystem::copy_option::fail_if_exists);
				break;
			}
			}
			i.increment(ec);
			if (!!ec)
			{
				return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
			}
		}
		return std::forward<ErrorHandler>(handle_error)(boost::system::error_code(), Si::identity<void>());
#else
		boost::system::error_code ec = create_directories(to, Si::return_);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
		}

		ec = remove_all(to, Si::variant_).error();
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
		}

		std::vector<Si::noexcept_string> arguments;
		arguments.push_back(SILICIUM_SYSTEM_LITERAL("-Rv"));
		arguments.push_back(to_utf8_string(from));
		arguments.push_back(to_utf8_string(to));
		auto null_output = Si::Sink<char, Si::success>::erase(Si::null_sink<char, Si::success>());
		if (!output)
		{
			output = &null_output;
		}
		Si::error_or<int> result = run_process(*absolute_path::create("/bin/cp"), arguments, from, *output,
		                                       std::vector<std::pair<Si::os_char const *, Si::os_char const *>>(),
		                                       environment_inheritance::inherit);
		if (!result.is_error() && (result.get() != 0))
		{
			throw std::runtime_error("cp failed"); // TODO: return a custom error_code for that
		}
		return std::forward<ErrorHandler>(handle_error)(result.error(), Si::identity<void>());
#endif
	}

	template <class ErrorHandler>
	SILICIUM_USE_RESULT auto file_exists(absolute_path const &file, ErrorHandler &&handle_error)
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<bool>()))
	{
		boost::system::error_code ec;
		boost::filesystem::file_status status = boost::filesystem::status(file.to_boost_path(), ec);
		if (status.type() == boost::filesystem::file_not_found)
		{
			return false;
		}
		if (ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<bool>());
		}
		return true;
	}

	inline Si::error_or<bool> file_exists(absolute_path const &file)
	{
		return file_exists(file, Si::variant_);
	}

	template <class ErrorHandler>
	auto rename(absolute_path const &from, absolute_path const &to, ErrorHandler &&handle_error)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<void>()))
#endif
	{
		boost::system::error_code ec;
		boost::filesystem::rename(from.to_boost_path(), to.to_boost_path(), ec);
		return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<void>());
	}

	inline boost::system::error_code rename(absolute_path const &from, absolute_path const &to)
	{
		return rename(from, to, Si::return_);
	}

	inline path_segment unique_path()
	{
		return *path_segment::create(path(boost::filesystem::unique_path()));
	}

	template <class ErrorHandler>
	auto get_current_executable_path(ErrorHandler &&handle_error)
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<absolute_path>()))
	{
#ifdef _WIN32
		// will be enough for most cases
		std::vector<wchar_t> buffer(MAX_PATH);
		for (;;)
		{
			DWORD const length = GetModuleFileNameW(NULL, buffer.data(), buffer.size());
			boost::system::error_code const ec = Si::get_last_error();
			switch (ec.value())
			{
			case ERROR_INSUFFICIENT_BUFFER:
			{
				Si::overflow_or<std::size_t> const new_buffer_size = Si::checked_add(buffer.size(), buffer.size());
				if (new_buffer_size.is_overflow() || (*new_buffer_size.value() > (std::numeric_limits<DWORD>::max)()))
				{
					// Win32 seems to return nonsense here. Better stop here entirely for safety reasons.
					std::terminate();
				}
				buffer.resize(*new_buffer_size.value());
				break;
			}

			case ERROR_SUCCESS:
			{
				boost::filesystem::path path(buffer.begin(), buffer.begin() + length);
				return *ventura::absolute_path::create(std::move(path));
			}

			default:
				return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<absolute_path>());
			}
		}
#elif defined(__linux__)
		boost::system::error_code ec;
		boost::filesystem::path result = boost::filesystem::read_symlink("/proc/self/exe", ec);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<absolute_path>());
		}
		return *absolute_path::create(std::move(result));
#else
		std::vector<char> buffer(256);
		std::uint32_t length = static_cast<std::uint32_t>(buffer.size());
		if (_NSGetExecutablePath(buffer.data(), &length) != 0)
		{
			buffer.resize(length);
			int result = _NSGetExecutablePath(buffer.data(), &length);
			if (result != 0)
			{
				boost::throw_exception(std::logic_error("_NSGetExecutablePath failed unexpectedly"));
			}
		}
		return *absolute_path::create(buffer.data());
#endif
	}

	inline Si::error_or<absolute_path> get_current_executable_path()
	{
		return get_current_executable_path(Si::variant_);
	}

	template <class ErrorHandler>
	auto temporary_directory(ErrorHandler &&handle_error)
	    -> decltype(std::forward<ErrorHandler>(handle_error)(boost::declval<boost::system::error_code>(),
	                                                         Si::identity<absolute_path>()))
	{
		boost::system::error_code ec;
		boost::filesystem::path temp = boost::filesystem::temp_directory_path(ec);
		if (!!ec)
		{
			return std::forward<ErrorHandler>(handle_error)(ec, Si::identity<absolute_path>());
		}
		return *absolute_path::create(std::move(temp));
	}

	inline Si::error_or<absolute_path> temporary_directory()
	{
		return temporary_directory(Si::variant_);
	}

	inline boost::system::error_code seek_absolute(Si::native_file_descriptor file, boost::uint64_t destination)
	{
#ifdef _WIN32
		LARGE_INTEGER destination_converted;
		destination_converted.QuadPart = destination;
		if (!SetFilePointerEx(file, destination_converted, nullptr, FILE_BEGIN))
		{
			return Si::get_last_error();
		}
#else
		auto position = lseek(file, destination, SEEK_SET);
		if (static_cast<boost::uint64_t>(position) != destination)
		{
			return Si::get_last_error();
		}
#endif
		return boost::system::error_code();
	}

	inline Si::error_or<boost::uint64_t> cursor_position(Si::native_file_descriptor file)
	{
#ifdef _WIN32
		LARGE_INTEGER position;
		position.QuadPart = 0;
		if (!SetFilePointerEx(file, position, &position, FILE_CURRENT))
		{
			return Si::get_last_error();
		}
		return static_cast<boost::uint64_t>(position.QuadPart);
#else
		auto position = lseek(file, 0, SEEK_CUR);
		if (position < 0)
		{
			return Si::get_last_error();
		}
		return static_cast<boost::uint64_t>(position);
#endif
	}

	inline Si::error_or<std::vector<char>> read_file(Si::native_file_descriptor file)
	{
		std::vector<char> content;
		boost::uint64_t size =
		    ventura::file_size(file).get().or_throw([]
		                                            {
			                                            throw std::runtime_error("Expected file to have a size");
			                                        });
		if (size > content.max_size())
		{
			throw std::bad_alloc();
		}
		content.resize(static_cast<std::size_t>(size));
		if (Si::read(file, Si::make_memory_range(content)).get() != size)
		{
			throw std::runtime_error(boost::str(boost::format("Could not read %1% bytes from a file") % size));
		}
		return Si::error_or<std::vector<char>>(std::move(content));
	}

	inline Si::error_or<std::vector<char>> read_file(absolute_path const &path)
	{
		Si::file_handle file = ventura::open_reading(safe_c_str(to_native_range(path))).move_value();
		return read_file(file.handle);
	}

#ifdef _WIN32
	namespace detail
	{
		struct co_task_mem_deleter
		{
			void operator()(void *memory) const
			{
				CoTaskMemFree(memory);
			}
		};
	}

	inline ventura::absolute_path get_home()
	{
		PWSTR path;
		HRESULT rc = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &path);
		if (rc != S_OK)
		{
			throw std::runtime_error("Could not get home");
		}
		std::unique_ptr<wchar_t, detail::co_task_mem_deleter> raii_path(path);
		return absolute_path::create(raii_path.get())
		    .or_throw([]
		              {
			              throw std::runtime_error("Windows returned a non-absolute path for home");
			          });
	}
#else
	inline absolute_path get_home()
	{
		return *absolute_path::create(getpwuid(getuid())->pw_dir);
	}
#endif

#endif
}

#endif
