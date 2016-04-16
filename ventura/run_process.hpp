#ifndef VENTURA_RUN_PROCESS_HPP
#define VENTURA_RUN_PROCESS_HPP

#include <ventura/async_process.hpp>
#include <silicium/write.hpp>
#include <silicium/sink/multi_sink.hpp>
#include <silicium/sink/iterator_sink.hpp>
#include <silicium/sink/copy.hpp>
#include <silicium/source/range_source.hpp>
#include <silicium/source/transforming_source.hpp>
#include <boost/range/algorithm/transform.hpp>

namespace ventura
{
	template <class T>
	bool extract(T &destination, Si::optional<T> &&source)
	{
		if (source)
		{
			destination = std::forward<T>(*source);
			return true;
		}
		return false;
	}
}

#define VENTURA_HAS_RUN_PROCESS                                                                                        \
	(SILICIUM_HAS_EXCEPTIONS && VENTURA_HAS_EXPERIMENTAL_READ_FROM_ANONYMOUS_PIPE && VENTURA_HAS_LAUNCH_PROCESS)

#if VENTURA_HAS_RUN_PROCESS
#include <future>

namespace ventura
{
	namespace detail
	{
		inline Si::error_or<Si::pipe> make_pipe() BOOST_NOEXCEPT
		{
#ifdef _WIN32
			SECURITY_ATTRIBUTES security = {};
			security.nLength = sizeof(security);
			security.bInheritHandle = FALSE;
			HANDLE read, write;
			if (!CreatePipe(&read, &write, &security, 0))
			{
				return Si::get_last_error();
			}
			Si::pipe result;
			result.read = Si::file_handle(read);
			result.write = Si::file_handle(write);
			return std::move(result);
#else
			return Si::make_pipe();
#endif
		}

		inline Si::file_handle enable_inheritance(Si::file_handle original)
		{
#ifdef _WIN32
			HANDLE enabled = INVALID_HANDLE_VALUE;
			if (!DuplicateHandle(GetCurrentProcess(), original.release(), GetCurrentProcess(), &enabled, 0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE))
			{
				assert(enabled == INVALID_HANDLE_VALUE);
				Si::throw_last_error();
			}
			return Si::file_handle(enabled);
#else
			return original;
#endif
		}
	}

	inline Si::error_or<int> run_process(process_parameters const &parameters)
	{
		async_process_parameters async_parameters;
		async_parameters.executable = parameters.executable;
		async_parameters.arguments = parameters.arguments;
		async_parameters.current_path = parameters.current_path;
		auto input = detail::make_pipe().move_value();
		auto std_output = detail::make_pipe().move_value();
		auto std_error = detail::make_pipe().move_value();

		Si::file_handle inheritable_stdin_read = detail::enable_inheritance(std::move(input.read));
		Si::file_handle inheritable_stdout_write = detail::enable_inheritance(std::move(std_output.write));
		Si::file_handle inheritable_stderr_write = detail::enable_inheritance(std::move(std_error.write));
		async_process process =
		    launch_process(async_parameters, inheritable_stdin_read.handle, inheritable_stdout_write.handle, inheritable_stderr_write.handle,
		                   parameters.additional_environment, parameters.inheritance)
		        .move_value();

		inheritable_stdin_read.close();
		inheritable_stdout_write.close();
		inheritable_stderr_write.close();

		boost::asio::io_service io;

		boost::promise<void> stop_polling;
		boost::shared_future<void> stopped_polling = stop_polling.get_future().share();

		auto std_output_consumer = Si::make_multi_sink<char, Si::success>(
		    [&parameters]()
		    {
			    return Si::make_iterator_range(&parameters.out, &parameters.out + (parameters.out != nullptr));
			});
		auto stdout_finished = experimental::read_from_anonymous_pipe(io, std_output_consumer, std::move(std_output.read), stopped_polling);

		auto std_error_consumer = Si::make_multi_sink<char, Si::success>(
		    [&parameters]()
		    {
			    return Si::make_iterator_range(&parameters.err, &parameters.err + (parameters.err != nullptr));
			});
		auto stderr_finished = experimental::read_from_anonymous_pipe(io, std_error_consumer, std::move(std_error.read), stopped_polling);

		auto copy_input = std::async(std::launch::async, [&input, &parameters]()
		                             {
			                             if (!parameters.in)
			                             {
				                             return;
			                             }
			                             for (;;)
			                             {
				                             Si::optional<char> const c = Si::get(*parameters.in);
				                             if (!c)
				                             {
					                             break;
				                             }
				                             Si::error_or<size_t> written =
				                                 write(input.write.handle, Si::make_memory_range(&*c, 1));
				                             if (written.is_error())
				                             {
					                             // process must have exited
					                             break;
				                             }
				                             assert(written.get() == 1);
			                             }
#ifdef _WIN32
										 //do not know whether this is necessary
										 BOOL flushed = FlushFileBuffers(input.write.handle);
#endif
			                             input.write.close();
			                         });

#ifdef _WIN32
		io.run();
		copy_input.get();
		stdout_finished.get();
		stderr_finished.get();
		return process.wait_for_exit();
#else
		io.run();
		copy_input.get();
		return process.wait_for_exit();
#endif
	}

#ifdef _WIN32
	SILICIUM_USE_RESULT
	inline Si::error_or<int>
	run_process(absolute_path executable, std::vector<Si::os_string> arguments, absolute_path current_directory,
	            Si::Sink<char, Si::success>::interface &output,
	            std::vector<std::pair<Si::os_char const *, Si::os_char const *>> additional_environment,
	            environment_inheritance inheritance)
	{
		process_parameters parameters;
		parameters.executable = std::move(executable);
		parameters.arguments = std::move(arguments);
		parameters.current_path = std::move(current_directory);
		parameters.out = &output;
		parameters.err = &output;
		parameters.additional_environment = std::move(additional_environment);
		parameters.inheritance = inheritance;
		return run_process(parameters);
	}
#endif

	SILICIUM_USE_RESULT
	inline std::vector<Si::os_string> arguments_to_os_strings(std::vector<Si::noexcept_string> const &arguments)
	{
		std::vector<Si::os_string> new_arguments;
		{
			auto new_arguments_sink = Si::make_container_sink(new_arguments);
			auto arguments_encoder =
			    Si::make_transforming_source(Si::make_range_source(Si::make_contiguous_range(arguments)),
			                                 [](Si::noexcept_string const &in) -> Si::os_string
			                                 {
				                                 // TODO: mvoe the string if possible
				                                 return Si::to_os_string(in);
				                             });
			Si::copy(arguments_encoder, new_arguments_sink);
		}
		return new_arguments;
	}

	SILICIUM_USE_RESULT
	inline Si::error_or<int>
	run_process(absolute_path executable, std::vector<Si::noexcept_string> arguments, absolute_path current_directory,
	            Si::Sink<char, Si::success>::interface &output,
	            std::vector<std::pair<Si::os_char const *, Si::os_char const *>> additional_environment,
	            environment_inheritance inheritance)
	{
		process_parameters parameters;
		parameters.executable = std::move(executable);
		std::vector<Si::os_string> new_arguments = arguments_to_os_strings(arguments);
		parameters.arguments = std::move(new_arguments);
		parameters.current_path = std::move(current_directory);
		parameters.out = &output;
		parameters.err = &output;
		parameters.additional_environment = std::move(additional_environment);
		parameters.inheritance = inheritance;
		return run_process(parameters);
	}
}
#endif

#endif
