#ifndef VENTURA_READ_FROM_ANONYMOUS_PIPE_HPP
#define VENTURA_READ_FROM_ANONYMOUS_PIPE_HPP

#include <silicium/asio/process_output.hpp>
#include <silicium/observable/thread.hpp>
#include <silicium/sink/buffering_sink.hpp>
#include <boost/thread/future.hpp>

namespace ventura
{
#define VENTURA_HAS_EXPERIMENTAL_READ_FROM_ANONYMOUS_PIPE                                                              \
    (SILICIUM_HAS_THREAD_OBSERVABLE && SILICIUM_HAS_BUFFERING_SINK)

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
        // TODO: find a more generic API for reading from a pipe portably
        template <class CharSink>
        boost::unique_future<void> read_from_anonymous_pipe(boost::asio::io_service &io, CharSink &&destination,
                                                            Si::file_handle file,
                                                            boost::shared_future<void> stop_polling)
        {
            auto finished = std::make_shared<boost::promise<void>>();
#ifdef _WIN32
            auto copyable_file = Si::to_shared(std::move(file));
            auto work = std::make_shared<boost::asio::io_service::work>(io);
            auto stop_polling_shared = Si::to_shared(std::move(stop_polling));
            Si::spawn_observable(Si::asio::make_posting_observable(
                io, Si::make_thread_observable<Si::std_threading>(
                        [work, copyable_file, destination, stop_polling_shared, finished]()
                        {
                            win32::copy_whole_pipe(copyable_file->handle, destination, std::move(*stop_polling_shared));
                            finished->set_value();
                            return Si::unit();
                        })));
#elif SILICIUM_HAS_SPAWN_COROUTINE
            boost::ignore_unused_variable_warning(stop_polling);
            auto copyable_file = Si::to_shared(std::move(file));
            Si::spawn_coroutine([&io, destination, copyable_file, finished](Si::spawn_context yield)
                                {
                                    Si::process_output output_reader(
                                        Si::make_unique<Si::process_output::stream>(io, copyable_file->handle));
                                    copyable_file->release();
                                    for (;;)
                                    {
                                        auto piece = yield.get_one(Si::ref(output_reader));
                                        assert(piece);
                                        if (piece->is_error())
                                        {
                                            break;
                                        }
                                        Si::memory_range data = piece->get();
                                        if (data.empty())
                                        {
                                            break;
                                        }
                                        Si::append(destination, data);
                                    }
                                    finished->set_value();
                                });
#else
            boost::ignore_unused_variable_warning(stop_polling);
            typedef typename std::decay<CharSink>::type clean_destination;
            struct pipe_reader : std::enable_shared_from_this<pipe_reader>
            {
                pipe_reader(boost::asio::io_service &io, Si::file_handle file, clean_destination destination)
                    : m_output(Si::make_unique<Si::process_output::stream>(io, file.handle))
                    , m_destination(std::move(destination))
                {
                    file.release();
                }

                void start()
                {
                    auto this_ = this->shared_from_this();
                    m_output.async_get_one(Si::make_function_observer([this_](optional<error_or<memory_range>> piece)
                                                                      {
                                                                          assert(piece);
                                                                          if (piece->is_error())
                                                                          {
                                                                              return;
                                                                          }
                                                                          Si::memory_range data = piece->get();
                                                                          if (data.empty())
                                                                          {
                                                                              return;
                                                                          }
                                                                          Si::append(this_->m_destination, data);
                                                                          this_->start();
                                                                      }));
                }

            private:
                Si::process_output m_output;
                clean_destination m_destination;
            };
            auto reader = std::make_shared<pipe_reader>(io, std::move(file), std::forward<CharSink>(destination));
            reader->start();

            // TODO whatever.. rewrite this mess
            finished->set_value();
#endif
            return finished->get_future();
        }
#endif
    }
}

#endif
