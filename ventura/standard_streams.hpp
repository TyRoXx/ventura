#ifndef VENTURA_STANDARD_STREAMS_HPP
#define VENTURA_STANDARD_STREAMS_HPP

#include <silicium/file_handle.hpp>

namespace ventura
{
    inline Si::native_file_descriptor get_standard_input()
    {
#ifdef _WIN32
        return GetStdHandle(STD_INPUT_HANDLE);
#else
        return 0;
#endif
    }

    inline Si::native_file_descriptor get_standard_output()
    {
#ifdef _WIN32
        return GetStdHandle(STD_OUTPUT_HANDLE);
#else
        return 1;
#endif
    }

    inline Si::native_file_descriptor get_standard_error()
    {
#ifdef _WIN32
        return GetStdHandle(STD_ERROR_HANDLE);
#else
        return 2;
#endif
    }
}

#endif
