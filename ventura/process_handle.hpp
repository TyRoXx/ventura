#ifndef VENTURA_PROCESS_HANDLE_HPP
#define VENTURA_PROCESS_HANDLE_HPP

#include <silicium/error_or.hpp>
#include <silicium/get_last_error.hpp>
#include <boost/swap.hpp>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace ventura
{
#ifdef _WIN32
    struct process_handle
    {
        process_handle() BOOST_NOEXCEPT : m_id(INVALID_HANDLE_VALUE)
        {
        }

        explicit process_handle(HANDLE id) BOOST_NOEXCEPT : m_id(id)
        {
        }

        ~process_handle() BOOST_NOEXCEPT
        {
            if (m_id == INVALID_HANDLE_VALUE)
            {
                return;
            }
            wait_for_exit();
        }

        process_handle(process_handle &&other) BOOST_NOEXCEPT : m_id(INVALID_HANDLE_VALUE)
        {
            swap(other);
        }

        process_handle &operator=(process_handle &&other) BOOST_NOEXCEPT
        {
            swap(other);
            return *this;
        }

        void swap(process_handle &other) BOOST_NOEXCEPT
        {
            boost::swap(m_id, other.m_id);
        }

        SILICIUM_USE_RESULT
        Si::error_or<int> wait_for_exit() BOOST_NOEXCEPT
        {
            WaitForSingleObject(m_id, INFINITE);
            DWORD exit_code = 1;
            if (!GetExitCodeProcess(m_id, &exit_code))
            {
                return Si::get_last_error();
            }
            CloseHandle(m_id);
            m_id = INVALID_HANDLE_VALUE;
            return static_cast<int>(exit_code);
        }

    private:
        HANDLE m_id;

        SILICIUM_DELETED_FUNCTION(process_handle(process_handle const &))
        SILICIUM_DELETED_FUNCTION(process_handle &operator=(process_handle const &))
    };
#else
    struct process_handle
    {
        process_handle() BOOST_NOEXCEPT : m_id(-1)
        {
        }

        explicit process_handle(pid_t id) BOOST_NOEXCEPT : m_id(id)
        {
        }

        ~process_handle() BOOST_NOEXCEPT
        {
            if (m_id < 0)
            {
                return;
            }
            wait_for_exit().get();
        }

        process_handle(process_handle &&other) BOOST_NOEXCEPT : m_id(-1)
        {
            swap(other);
        }

        process_handle &operator=(process_handle &&other) BOOST_NOEXCEPT
        {
            swap(other);
            return *this;
        }

        void swap(process_handle &other) BOOST_NOEXCEPT
        {
            boost::swap(m_id, other.m_id);
        }

        SILICIUM_USE_RESULT
        Si::error_or<int> wait_for_exit() BOOST_NOEXCEPT
        {
            int status = 0;
            int wait_id = Si::exchange(m_id, -1);
            assert(wait_id >= 1);
            if (waitpid(wait_id, &status, 0) < 0)
            {
                return Si::get_last_error();
            }
            int const exit_status = WEXITSTATUS(status);
            return exit_status;
        }

    private:
        pid_t m_id;

        SILICIUM_DELETED_FUNCTION(process_handle(process_handle const &))
        SILICIUM_DELETED_FUNCTION(process_handle &operator=(process_handle const &))
    };
#endif
}

#endif
