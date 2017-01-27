#ifndef VENTURA_READ_FROM_ANONYMOUS_PIPE_HPP
#define VENTURA_READ_FROM_ANONYMOUS_PIPE_HPP

#include <silicium/sink/buffering_sink.hpp>
#include <silicium/sink/append.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <silicium/make_unique.hpp>
#include <silicium/optional.hpp>
#include <silicium/to_shared.hpp>

namespace ventura
{
#define VENTURA_HAS_EXPERIMENTAL_READ_FROM_ANONYMOUS_PIPE SILICIUM_HAS_BUFFERING_SINK

#if VENTURA_HAS_EXPERIMENTAL_READ_FROM_ANONYMOUS_PIPE && defined(_WIN32)
    namespace win32
    {
        template <class ByteSink>
        void copy_whole_pipe(HANDLE pipe_in, ByteSink &&sink_out, boost::shared_future<void> stop_polling)
        {
            auto buffered_out = make_buffering_sink(std::forward<ByteSink>(sink_out));
            for (;;)
            {
                auto buffer = buffered_out.make_append_space((std::numeric_limits<DWORD>::max)());
                DWORD read_bytes = 0;
                DWORD available = 0;
                DWORD left = 0;
                BOOL const peeked = PeekNamedPipe(pipe_in, buffer.begin(), static_cast<DWORD>(buffer.size()),
                                                  &read_bytes, &available, &left);
                if (!peeked)
                {
                    auto error = ::GetLastError();
                    if (error == ERROR_BROKEN_PIPE)
                    {
                        buffered_out.make_append_space(read_bytes);
                        buffered_out.flush_append_space();
                        break;
                    }
                    throw boost::system::system_error(error, boost::system::native_ecat);
                }
                if (available == 0)
                {
                    switch (stop_polling.wait_for(boost::chrono::milliseconds(300)))
                    {
                    case std::future_status::ready:
                        return;

                    case std::future_status::deferred:
                    case std::future_status::timeout:
                        break;
                    }
                    continue;
                }
                if (ReadFile(pipe_in, buffer.begin(), available, &read_bytes, nullptr))
                {
                    assert(available == read_bytes);
                    buffered_out.make_append_space(read_bytes);
                    buffered_out.flush_append_space();
                }
                else
                {
                    Si::throw_last_error();
                }
            }
            buffered_out.flush();
        }
    }
#endif
    namespace experimental
    {
#if VENTURA_HAS_EXPERIMENTAL_READ_FROM_ANONYMOUS_PIPE
        struct pipe_reading_state
        {
            boost::asio::posix::stream_descriptor reader;
            std::array<char, 4096> buffer;

            explicit pipe_reading_state(boost::asio::io_service &io, Si::file_handle file)
                : reader(io, file.release())
            {
            }
        };

        template <class CharSink>
        void read_all(std::shared_ptr<pipe_reading_state> state, CharSink &&destination)
        {
            state->reader.async_read_some(
                boost::asio::buffer(state->buffer), [state, &destination](boost::system::error_code ec, size_t bytes)
                {
                    if (!!ec)
                    {
                        return;
                    }
                    Si::append(destination,
                               Si::make_iterator_range(state->buffer.begin(), state->buffer.begin() + bytes));
                    read_all(state, destination);
                });
        }

        // TODO: find a more generic API for reading from a pipe portably
        template <class CharSink>
        boost::unique_future<void> read_from_anonymous_pipe(boost::asio::io_service &io, CharSink &&destination,
                                                            Si::file_handle file,
                                                            boost::shared_future<void> stop_polling)
        {
            // TODO: implement for Windows
            (void)stop_polling;
            auto state = std::make_shared<pipe_reading_state>(io, std::move(file));
            read_all(state, destination);
            return boost::make_ready_future();
        }
#endif
    }
}

#endif
