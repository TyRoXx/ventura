#ifndef VENTURA_ABSOLUTE_PATH_HPP
#define VENTURA_ABSOLUTE_PATH_HPP

#include <ventura/relative_path.hpp>
#include <ventura/path_segment.hpp>
#include <silicium/optional.hpp>
#include <silicium/error_or.hpp>
#include <silicium/os_string.hpp>
#include <silicium/is_handle.hpp>
#include <silicium/c_string.hpp>
#include <silicium/get_last_error.hpp>
#include <silicium/sink/sink.hpp>
#ifdef _WIN32
#include <silicium/win32/win32.hpp>
#endif
#include <iostream>

namespace ventura
{
	struct absolute_path
	{
		typedef path_char char_type;
		typedef path::underlying_type underlying_type;

		absolute_path() BOOST_NOEXCEPT
		{
		}

		absolute_path(absolute_path &&other) BOOST_NOEXCEPT : m_value(std::move(other.m_value))
		{
		}

		absolute_path(absolute_path const &other)
		    : m_value(other.m_value)
		{
		}

		absolute_path &operator=(absolute_path &&other) BOOST_NOEXCEPT
		{
			m_value = std::move(other.m_value);
			return *this;
		}

		absolute_path &operator=(absolute_path const &other)
		{
			m_value = other.m_value;
			return *this;
		}

		void swap(absolute_path &other) BOOST_NOEXCEPT
		{
			m_value.swap(other.m_value);
		}

		SILICIUM_USE_RESULT
		boost::filesystem::path to_boost_path() const
		{
			return m_value.to_boost_path();
		}

		SILICIUM_USE_RESULT
		path::underlying_type const &underlying() const BOOST_NOEXCEPT
		{
			return m_value.underlying();
		}

		SILICIUM_USE_RESULT
		char_type const *c_str() const BOOST_NOEXCEPT
		{
			return m_value.c_str();
		}

		SILICIUM_USE_RESULT
		Si::c_string safe_c_str() const BOOST_NOEXCEPT
		{
			return Si::c_string(c_str());
		}

		void combine(relative_path const &back)
		{
			// TODO: optimize
			*this = absolute_path(to_boost_path() / back.to_boost_path());
		}

		template <class RelativePath>
		absolute_path &operator/=(RelativePath &&other)
		{
			combine(std::forward<RelativePath>(other));
			return *this;
		}

		SILICIUM_USE_RESULT
		bool empty() const BOOST_NOEXCEPT
		{
			return underlying().empty();
		}

		SILICIUM_USE_RESULT
		static Si::optional<absolute_path> create(boost::filesystem::path const &maybe_absolute)
		{
			if (maybe_absolute.is_absolute())
			{
				return absolute_path(maybe_absolute);
			}
			return Si::none;
		}

		SILICIUM_USE_RESULT
		static Si::optional<absolute_path> create(Si::noexcept_string const &maybe_absolute)
		{
			return create(boost::filesystem::path(maybe_absolute.c_str()));
		}

		SILICIUM_USE_RESULT
		static Si::optional<absolute_path> create(char const *maybe_absolute)
		{
			return create(boost::filesystem::path(maybe_absolute));
		}

#ifdef _WIN32
		SILICIUM_USE_RESULT
		static Si::optional<absolute_path> create(wchar_t const *maybe_absolute)
		{
			return create(boost::filesystem::path(maybe_absolute));
		}
#endif

		Si::optional<path_segment> name() const
		{
			auto &&boost_path = to_boost_path();
			if (!boost_path.has_filename())
			{
				return Si::none;
			}
			return path_segment::create(path(boost_path.filename()));
		}

	private:
		path m_value;

		explicit absolute_path(boost::filesystem::path const &value)
		    : m_value(value)
		{
		}
	};

#if SILICIUM_HAS_IS_HANDLE
	BOOST_STATIC_ASSERT(Si::is_handle<absolute_path>::value);
#endif

	inline std::ostream &operator<<(std::ostream &out, absolute_path const &p)
	{
		return out << p.underlying();
	}

	inline std::istream &operator>>(std::istream &in, absolute_path &p)
	{
		absolute_path::underlying_type temp;
		in >> temp;
		if (!in)
		{
			return in;
		}
		Si::optional<absolute_path> checked = absolute_path::create(std::move(temp));
		if (!checked)
		{
			in.setstate(std::ios::failbit);
			return in;
		}
		p = std::move(*checked);
		return in;
	}

	template <class ComparableToPath>
	SILICIUM_USE_RESULT inline bool operator==(absolute_path const &left, ComparableToPath const &right)
	{
		return left.underlying() == right;
	}

	template <class ComparableToPath>
	SILICIUM_USE_RESULT inline bool operator==(ComparableToPath const &left, absolute_path const &right)
	{
		return left == right.underlying();
	}

	SILICIUM_USE_RESULT
	inline bool operator==(absolute_path const &left, boost::filesystem::path const &right)
	{
		return right == left.c_str();
	}

	SILICIUM_USE_RESULT
	inline bool operator==(boost::filesystem::path const &left, absolute_path const &right)
	{
		return left == right.c_str();
	}

	SILICIUM_USE_RESULT
	inline bool operator==(absolute_path const &left, absolute_path const &right)
	{
		return left.underlying() == right.underlying();
	}

	template <class ComparableToPath>
	SILICIUM_USE_RESULT inline bool operator!=(absolute_path const &left, ComparableToPath const &right)
	{
		return !(left == right);
	}

	template <class ComparableToPath>
	SILICIUM_USE_RESULT inline bool operator!=(ComparableToPath const &left, absolute_path const &right)
	{
		return !(left == right);
	}

	SILICIUM_USE_RESULT
	inline bool operator<(absolute_path const &left, absolute_path const &right)
	{
		return left.underlying() < right.underlying();
	}

	SILICIUM_USE_RESULT
	inline std::size_t hash_value(absolute_path const &value)
	{
		using boost::hash_value;
		return hash_value(value.underlying());
	}

	SILICIUM_USE_RESULT
	inline relative_path leaf(absolute_path const &whole)
	{
		// TODO: do this efficiently
		return relative_path(whole.to_boost_path().leaf());
	}

	SILICIUM_USE_RESULT
	inline Si::optional<absolute_path> parent(absolute_path const &whole)
	{
		// TODO: do this efficiently
		auto boosted = whole.to_boost_path();
		if (boosted.has_parent_path())
		{
			return *absolute_path::create(boosted.parent_path());
		}
		return Si::none;
	}

	SILICIUM_USE_RESULT
	inline absolute_path operator/(absolute_path const &front, relative_path const &back)
	{
		// TODO: do this efficiently
		absolute_path result = front;
		result.combine(back);
		return result;
	}

	template <std::size_t N>
	SILICIUM_USE_RESULT inline absolute_path operator/(absolute_path const &front,
	                                                   absolute_path::char_type const(&literal)[N])
	{
		return front / relative_path(boost::filesystem::path(&literal[0]));
	}

	inline absolute_path operator/(absolute_path const &front, path_segment const &back)
	{
		absolute_path result = front;
		result.combine(relative_path(back.to_boost_path()));
		return result;
	}

	template <class String = Si::noexcept_string>
	String to_utf8_string(boost::filesystem::path const &path)
	{
		return Si::to_utf8_string<String>(path.string());
	}

	template <class String = Si::noexcept_string>
	String to_utf8_string(absolute_path const &path)
	{
		return to_utf8_string<String>(path.to_boost_path());
	}

	inline Si::os_string to_os_string(absolute_path const &path)
	{
		return path.to_boost_path().
#ifdef _WIN32
		    wstring();
#else
		    c_str();
#endif
	}

#ifdef _WIN32
	inline Si::os_string to_native_range(absolute_path const &path)
	{
		return to_os_string(path);
	}
#else
	inline absolute_path to_native_range(absolute_path path)
	{
		return path;
	}
#endif

	inline Si::os_c_string safe_c_str(Si::os_string const &string)
	{
		return Si::os_c_string(string.c_str());
	}

	inline Si::c_string safe_c_str(absolute_path const &path)
	{
		return path.safe_c_str();
	}
}

namespace std
{
	template <>
	struct hash<ventura::absolute_path>
	{
		SILICIUM_USE_RESULT
		std::size_t operator()(ventura::absolute_path const &value) const
		{
			return hash_value(value);
		}
	};
}

#endif
