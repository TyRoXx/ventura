#ifndef VENTURA_PATH_CHAR_HPP
#define VENTURA_PATH_CHAR_HPP

namespace ventura
{
	typedef
#ifdef _WIN32
	    wchar_t
#else
	    char
#endif
	        native_path_char;
}

#endif
