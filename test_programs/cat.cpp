#include <iostream>
#include <silicium/read.hpp>
#include <silicium/write.hpp>
#include <ventura/standard_streams.hpp>

int main(int argc, char **)
{
    if (argc > 1)
    {
        std::cerr << "Command line arguments are not supported\n";
        return 1;
    }
    std::array<char, 4096> buffer;
    for (;;)
    {
        Si::error_or<std::size_t> r = Si::read(ventura::get_standard_input(), Si::make_memory_range(buffer));
        if (r.is_error())
        {
            std::cerr << "read from standard input failed: " << r.error() << '\n';
            return 1;
        }

        if (r.get() == 0)
        {
            break;
        }

        Si::error_or<std::size_t> w =
            Si::write(ventura::get_standard_output(), Si::make_memory_range(buffer.data(), buffer.data() + r.get()));
        if (w.is_error())
        {
            std::cerr << "write to standard output failed: " << w.error() << '\n';
            return 1;
        }
    }
}
