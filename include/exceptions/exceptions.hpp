#pragma once

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace zol::exceptions
{
	class error: public std::runtime_error
	{
	public:
		error(const std::string& system, const std::string& message, const char* func, const char* file, int line)
			: std::runtime_error(generate(system, message, func, file, line))
			, system(system)
			, func(func)
			, file(file)
			, line(line)
		{
		}

		const std::string system;
		const std::string func;
		const std::string file;
		const int line;

	private:
		auto generate(const std::string& system, const std::string& message, const char* func, const char* file, int line) -> std::string
		{
			std::ostringstream ss;
			ss << "[" << system << "] " << message
			   << " (at " << func << " in " << file << ":" << line << ")";

			return ss.str();
		}
	};

	class handler
	{
	public:
		using fn_type = std::function<void(const error&)>;

		auto instance() -> handler&
		{
			static handler inst;
			return inst;
		}

		auto set(fn_type fn) -> void
		{
			_fn = std::move(fn);
		}

		auto handle(const error& e) const -> void
		{
			if (_fn)
				_fn(e);
			else
				std::cerr << e.what() << std::endl;
		}

	private:
		handler() = default;
		fn_type _fn;
	};

	template<typename... Args>
	auto build_message(Args&&... args) -> std::string
	{
		std::ostringstream ss;
		(ss << ... << std::forward<Args>(args));
		return ss.str();
	}
}

// throw
#define _zol_throw(system, fmt, ...)      \
	throw zol::exceptions::error(         \
		system,                           \
		std::format(fmt, __VA_ARGS__),    \
		__FUNCTION__, __FILE__, __LINE__)

// try
#define _(system, function)                          \
	try                                              \
	{                                                \
		function;                                    \
	}                                                \
	catch (const zol::exceptions::error& _exception) \
	{                                                \
		_zol_throw(system, _exception.what())        \
	}
