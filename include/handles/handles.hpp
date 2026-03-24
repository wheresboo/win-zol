#pragma once

#include <cstdio>
#include <windows.h>

namespace zol::handles
{
	template<typename type>
	class t_base_handle
	{
	protected:
		type _handle;

	public:
		t_base_handle() noexcept
			: _handle(nullptr)
		{
		}

		explicit t_base_handle(type handle) noexcept
			: _handle(handle)
		{
		}

		t_base_handle(const t_base_handle&)			   = delete;
		t_base_handle& operator=(const t_base_handle&) = delete;

		t_base_handle(t_base_handle&& other) noexcept
			: _handle(other._handle)
		{
			other._handle = nullptr;
		}

		t_base_handle& operator=(t_base_handle&& other) noexcept
		{
			if (this != &other)
			{
				reset();

				_handle		  = other._handle;
				other._handle = nullptr;
			}

			return *this;
		}

		virtual ~t_base_handle()
		{
			reset();
		}

		virtual auto destructor() -> void = 0;

		// releases the handle
		auto release() noexcept -> type
		{
			type tmp = _handle;
			_handle	 = nullptr;

			return tmp;
		}

		// resets the handle
		auto reset(type handle = nullptr) noexcept -> void
		{
			if (_handle)
				destructor();

			_handle = handle;
		}

		// grabs the raw handle/pointer
		auto get() const noexcept -> type
		{
			return _handle;
		}

		// checks if the handle points to some location
		explicit operator bool() const noexcept
		{
			return _handle != nullptr;
		}
	};

	// replaces HANDLE
	struct handle: t_base_handle<HANDLE>
	{
		auto destructor() -> void override
		{
			CloseHandle(_handle);
		}

		handle() = default;
		explicit handle(HANDLE handle) : t_base_handle<HANDLE>(handle) {}
		handle& operator=(HANDLE handle) { reset(handle); return *this; }
	};

	// replaces HWND
	struct hwnd: t_base_handle<HWND>
	{
		auto destructor() -> void override
		{
			CloseWindow(_handle);
		}

		hwnd() = default;
		explicit hwnd(HWND handle) : t_base_handle<HWND>(handle) {}
		hwnd& operator=(HWND handle) { reset(handle); return *this; }
	};

	// replaces FILE*
	struct _file: t_base_handle<FILE*>
	{
		auto destructor() -> void override
		{
			fclose(_handle);
		}

		_file() = default;
		explicit _file(FILE* handle) : t_base_handle<FILE*>(handle) {}
		_file& operator=(FILE* handle) { reset(handle); return *this; }
	};

	// replaces HMODULE
	struct hmod: t_base_handle<HMODULE>
	{
		auto destructor() -> void override
		{
			FreeLibrary(_handle);
		}

		hmod() = default;
		explicit hmod(HMODULE handle) : t_base_handle<HMODULE>(handle) {}
		hmod& operator=(HMODULE handle) { reset(handle); return *this; }
	};
}
