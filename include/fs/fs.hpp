#pragma once

// std
#include <filesystem>
#include <fstream>

// zol
#include "singleton/singleton.hpp"
#include "exceptions/exceptions.hpp"

namespace zol::fs
{
	class file
	{
		std::fstream _stream;

	public:
		explicit file(const std::filesystem::path& path,
			const std::ios::openmode mode = std::ios::in | std::ios::binary)
		{
			_stream.open(path, mode);
			if (!_stream.is_open())
				_zol_throw("fs", "failed to open file: " + path.string());
		}

		~file()
		{
			if (_stream.is_open())
				_stream.close();
		}

		auto is_open() const -> bool
		{
			return _stream.is_open();
		}

		auto read_all() -> std::string
		{
			if (!_stream.is_open())
				_zol_throw("fs", "File not open");

			return std::string(
				std::istreambuf_iterator(_stream),
				std::istreambuf_iterator<char>());
		}

		auto write(const std::string_view data) -> void
		{
			if (!_stream.is_open())
				_zol_throw("fs", "File not open");

			_stream << data;
			if (!_stream.good())
				_zol_throw("fs", "Write failed");
		}
	};

	class manager: public singleton<manager>
	{
		friend class singleton;

	public:
		inline auto open(const std::filesystem::path& path, const std::ios::openmode mode = std::ios::in | std::ios::binary) -> file
		{
			return file(path, mode);
		}
	};
}
