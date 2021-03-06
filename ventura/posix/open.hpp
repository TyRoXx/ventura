#ifndef VENTURA_LINUX_OPEN_HPP
#define VENTURA_LINUX_OPEN_HPP

#include <silicium/c_string.hpp>
#include <silicium/error_or.hpp>
#include <silicium/file_handle.hpp>
#include <ventura/absolute_path.hpp>

#if defined(__unix) || defined(__APPLE__)
#include <fcntl.h>
#endif

namespace ventura
{
    inline Si::error_or<Si::file_handle> open_reading(Si::native_path_string name)
    {
        Si::native_file_descriptor const fd = ::open(name.c_str(), O_RDONLY);
        if (fd < 0)
        {
            return Si::get_last_error();
        }
        return Si::file_handle(fd);
    }

    inline Si::error_or<Si::file_handle> create_file(Si::native_path_string name)
    {
        Si::native_file_descriptor const fd =
            ::open(name.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            return Si::get_last_error();
        }
        return Si::file_handle(fd);
    }

    inline Si::error_or<Si::file_handle> create_file(absolute_path const &name)
    {
        return create_file(name.safe_c_str());
    }

    inline Si::error_or<Si::file_handle> overwrite_file(Si::native_path_string name)
    {
        Si::native_file_descriptor const fd =
            ::open(name.c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            return Si::get_last_error();
        }
        return Si::file_handle(fd);
    }

    inline Si::error_or<Si::file_handle> open_read_write(Si::native_path_string name)
    {
        Si::native_file_descriptor const fd =
            ::open(name.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            return Si::get_last_error();
        }
        return Si::file_handle(fd);
    }
}

#endif
