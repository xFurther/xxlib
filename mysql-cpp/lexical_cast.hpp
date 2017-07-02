#ifndef __LEXICAL_CAST_HPP__
#define __LEXICAL_CAST_HPP__
#include <string>
#include <stddef.h>
#include <stdint.h>
#include <sstream>
#include <stdexcept>

namespace detail {
	// https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
	unsigned facebook_uint64_to_str(uint64_t value, char* dst);
}

namespace detail {
	// https://code.google.com/p/stringencoders/source/browse/trunk/src/modp_numtoa.h
	// New BSD License

	/** \brief convert a floating point number to char buffer with
	*         fixed-precision format
	*
	* This is similar to "%.[0-9]f" in the printf style.  It will include
	* trailing zeros
	*
	* If the input value is greater than 1<<31, then the output format
	* will be switched exponential format.
	*
	* \param[in] value
	* \param[out] buf  The allocated output buffer.  Should be 32 chars or more.
	* \param[in] precision  Number of digits to the right of the decimal point.
	*    Can only be 0-9.
	*/
	size_t modp_dtoa(double value, char* str, int prec);
}

namespace detail {
	template < typename T1, typename T2 > struct is_same { static const bool value = false; };
	template < typename T> struct is_same < T, T > { static const bool value = true; };

	template < typename T > struct is_unsigned { static const bool value = false; };
	template <> struct is_unsigned < unsigned char > { static const bool value = true; };
	template <> struct is_unsigned < unsigned short > { static const bool value = true; };
	template <> struct is_unsigned < unsigned int > { static const bool value = true; };
	template <> struct is_unsigned < unsigned long > { static const bool value = true; };
	template <> struct is_unsigned < unsigned long long > { static const bool value = true; };

	template < typename T > struct is_integral { static const bool value = is_unsigned<T>::value; };
	template <> struct is_integral < char > { static const bool value = true; };
	template <> struct is_integral < short > { static const bool value = true; };
	template <> struct is_integral < int > { static const bool value = true; };
	template <> struct is_integral < long > { static const bool value = true; };
	template <> struct is_integral < long long > { static const bool value = true; };

	template < typename T > struct is_numeric { static const bool value = is_integral<T>::value; };
	template <> struct is_numeric < float > { static const bool value = true; };
	template <> struct is_numeric < double > { static const bool value = true; };


	template<class Source,
		bool = detail::is_numeric<Source>::value,
		bool = detail::is_integral<Source>::value,
		bool = detail::is_unsigned<Source>::value>
	struct to_string_helper { };

	template<class Source>
	struct to_string_helper < Source, true, true, true >
	{
		static std::string to_string(Source v)
		{
			std::string s(32, 0);
			unsigned len = facebook_uint64_to_str(v, &s.front());
			s.resize(len);
			return s;
		}
	};

	template<class Source>
	struct to_string_helper < Source, true, true, false >
	{
		static std::string to_string(const Source& v)
		{
			if (v >= 0)
				return to_string_helper<Source, true, true, true>::to_string(v);

			// negative case:
			std::string s(32, 0);
			s[0] = '-';
			unsigned len = facebook_uint64_to_str(-v, &s[1]);
			s.resize(len + 1);
			return s;
		}
	};

	template<class Source>
	struct to_string_helper < Source, true, false, false >
	{
		static std::string to_string(const Source& v)
		{
			std::string s(32, 0);
			size_t len = modp_dtoa(v, &s.front(), 8);
			s.resize(len);
			return s;
		}
	};

	template<class Source>
	struct to_string_helper < Source, false, false, false >
	{
		static std::string to_string(const Source& v)
		{
			std::ostringstream ss;
			ss << v;
			return ss.str();
		}
	};



	template<class Target,
		bool = detail::is_numeric<Target>::value,
		bool = detail::is_integral<Target>::value,
		bool = detail::is_unsigned<Target>::value>
	struct from_string_helper { };

	template<class Target>
	struct from_string_helper < Target, true, true, true >
	{
		static Target from_string(const char* str)
		{
			return static_cast<Target>(atoi(str));
		}
	};

	template<class Target>
	struct from_string_helper < Target, true, true, false >
	{
		static Target from_string(const char* str)
		{
			return static_cast<Target>(atoi(str));
		}
	};

	template<class Target>
	struct from_string_helper < Target, true, false, false >
	{
		static Target from_string(const char* str)
		{
			return static_cast<Target>(atof(str));
		}
	};

	template<class Target>
	struct from_string_helper < Target, false, false, false >
	{
		static Target from_string(const char* str)
		{
			Target t;
			std::istringstream ss(str);
			ss >> t;
			return t;
		}
	};

	struct bool_helper {
		static const std::string __true;
		static const std::string __false;

		static std::string to_string(bool v)
		{
			return v ? __true : __false;
		}

		static bool from_string(const char* v)
		{
			if (__true.compare(v) == 0)
				return true;
			if (__false.compare(v) == 0)
				return false;
			throw std::runtime_error(std::string("cant convert string to bool: ") + v);
		}
		static bool from_string(const std::string& v)
		{
			if (v == __true)
				return true;
			if (v == __false)
				return false;
			throw std::runtime_error(std::string("cant convert string to bool: ") + v);
		}
	};
	
}

namespace detail {
	template <typename Target, typename Source, bool = is_same<Target, Source>::value>
	struct convert
	{

	};



	template <typename Target>
	struct convert < Target, std::string, false >
	{
		static Target apply(const std::string& v)
		{
			return detail::from_string_helper<Target>::from_string(v.c_str());
		}
	};

	template <typename Target>
	struct convert < Target, char*, false >
	{
		static Target apply(const char* v)
		{
			return detail::from_string_helper<Target>::from_string(v);
		}
	};

	template < typename Target, unsigned N >
	struct convert < Target, const char[N], false >
	{
		static Target apply(const char(&v)[N])
		{
			return detail::from_string_helper<Target>::from_string(static_cast<const char*>(&v[0]));
		}
	};

	template <typename Source>
	struct convert < std::string, Source, false >
	{
		static std::string apply(const Source& v)
		{
			return detail::to_string_helper<Source>::to_string(v);
		}
	};

	template <>
	struct convert < std::string, bool, false >
	{
		static std::string apply(bool v)
		{
			return bool_helper::to_string(v);
		}
	};

	template <>
	struct convert < bool, std::string, false >
	{
		static bool apply(const std::string& v)
		{
			return bool_helper::from_string(v);
		}
	};

	template <>
	struct convert < bool, const char*, false >
	{
		static bool apply(const char* v)
		{
			return bool_helper::from_string(v);
		}
	};

	template <unsigned N>
	struct convert < bool, const char[N], false >
	{
		static bool apply(const char(&v)[N])
		{
			return bool_helper::from_string(static_cast<const char*>(&v[0]));
		}
	};

	template <typename T>
	struct convert < T, T, true >
	{
		static T apply(const T& v)
		{
			return v;
		}
	};
}

template <typename Target, typename Source>
Target lexical_cast(const Source& v)
{
	return detail::convert<Target, Source, detail::is_same<Target, Source>::value>::apply(v);
}


#endif
