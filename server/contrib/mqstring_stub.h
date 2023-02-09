#pragma once

#include <string_view>
#include <charconv>

namespace mq {
struct ci_less
{
	struct nocase_compare
	{
		bool operator() (const unsigned char& c1, const unsigned char& c2) const noexcept
		{
			if (c1 == c2)
				return false;
			return ::tolower(c1) < ::tolower(c2);
		}
	};

	struct nocase_equals
	{
		bool operator() (const unsigned char& c1, const unsigned char& c2) const noexcept
		{
			if (c1 == c2)
				return true;

			return ::tolower(c1) == ::tolower(c2);
		}
	};

	struct nocase_equals_w
	{
		bool operator() (const wchar_t& c1, const wchar_t& c2) const noexcept
		{
			if (c1 == c2)
				return true;

			return ::towlower(c1) == ::towlower(c2);
		}
	};

	bool operator()(std::string_view s1, std::string_view s2) const noexcept
	{
		return std::lexicographical_compare(
			s1.begin(), s1.end(),
			s2.begin(), s2.end(),
			nocase_compare());
	}

	using is_transparent = void;
};

[[nodiscard]]
inline std::string_view ltrim(std::string_view s)
{
	s.remove_prefix(std::min(s.find_first_not_of(" \t\n"), s.size()));
	return s;
}

[[nodiscard]]
inline std::string_view rtrim(std::string_view s)
{
	s.remove_suffix(std::min(s.size() - s.find_last_not_of(" \t\n") - 1, s.size()));
	return s;
}

[[nodiscard]]
inline std::string_view trim(std::string_view s)
{
	return rtrim(ltrim(s));
}


/**
 * @fn ci_equals
 *
 * @brief Case Insensitive Compare for two strings
 *
 * Determines if two strings are the same without regard to case.
 *
 * First makes sure the strings are the same size, then sends each character
 * through the equal function passing them to the @ref nocase_compare function.
 *
 * @param sv1 The first string to Compare
 * @param sv2 The second string to Compare
 *
 * @return bool The result of the comparison
 *
 **/
inline bool ci_equals(std::string_view sv1, std::string_view sv2)
{
	return sv1.size() == sv2.size()
		&& std::equal(sv1.begin(), sv1.end(), sv2.begin(), ci_less::nocase_equals());
}

inline bool ci_equals(std::wstring_view sv1, std::wstring_view sv2)
{
	return sv1.size() == sv2.size()
		&& std::equal(sv1.begin(), sv1.end(), sv2.begin(), ci_less::nocase_equals_w());
}

/**
 * @fn GetIntFromString
 *
 * @brief Gets the int value from a well formatted string
 *
 * Takes the input of a string and a value that should be returned if conversion fails.
 * Attempts to convert the string to an int and returns the converted value on success
 * or the failure value on fail.
 *
 * Suitable replacement for atoi (removing the undefined behavior) and faster than strtol.
 *
 * @see GetInt64FromString
 * @see GetDoubleFromString
 * @see GetFloatFromString
 *
 * @param svString The string to convert to an integer
 * @param iReturnOnFail The integer that should be returned if conversion fails
 *
 * @return int The converted integer or the "failure" value
 **/
inline int GetIntFromString(const std::string_view svString, int iReturnOnFail)
{
	std::string_view trimmed = trim(svString);
	auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), iReturnOnFail);
	// Could error check here, but failures don't modify the value and we're not returning meaningful errors.
	return iReturnOnFail;
}
}
