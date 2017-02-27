#pragma once
#ifndef __COMM_DBG_LOCATION_H__
#define __COMM_DBG_LOCATION_H__

#include "str.h"

COID_NAMESPACE_BEGIN

namespace debug {

class location
{
public:
	inline location(
		const char *function_name,
		const char *file_name,
		const int line_number)
		: _function_name(function_name)
		, _file_name(file_name)
		, _line_number(line_number)
	{}

	inline location()
		: _function_name("unknown")
		, _file_name(_function_name)
		, _line_number(-1)
	{}

	inline charstr& get_str(charstr &str) const {
		return str << _function_name << '@' << _file_name << '(' << _line_number << ')';
	}

	inline charstr to_str() const {
		charstr tmp;
		return get_str(tmp);
	}

	inline const char* function_name() const { return _function_name; }
	inline const char* file_name() const { return _file_name; }
	inline int line_number() const { return _line_number; }

	void operator = (const location &loc) {
		_function_name = loc._function_name;
		_file_name = loc._file_name;
		_line_number = loc._line_number;
	}

private:
	const char* _function_name;
	const char* _file_name;
	int _line_number;
};

} // end of namespace dbg

COID_NAMESPACE_END

#define LOCATION coid::debug::location(__FUNCTION__, __FILE__, __LINE__)

#endif // __COMM_DBG_LOCATION_H__
