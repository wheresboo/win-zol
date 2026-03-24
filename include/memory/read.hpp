#pragma once

#include <functional>

#include "exceptions/exceptions.hpp"
#include "handles/handles.hpp"
#include "types/types.hpp"

namespace zol::memory
{
	// This is our read function that can be manipulated by a developer for any reason they want, they may use it in conjunction with a Kernel Driver.
	template<typename t_buffer>
	static std::function<bool(const handles::handle&, uptr, usize, t_buffer*, bool)> _read_fn =
		[](const handles::handle& handle, const uptr address, const usize size, t_buffer* buffer, const bool force = true) -> bool {
		if (!buffer)
			_zol_throw("read", "Buffer is null");

		u32 old_protection = 0;
		if (force)
		{
			MEMORY_BASIC_INFORMATION mem_info { };
			if (!VirtualQueryEx(handle.get(), reinterpret_cast<void*>(address), &mem_info, sizeof(mem_info)))
				_zol_throw("read", "Couldn't query memory page");

			const auto new_protection = (mem_info.Protect == PAGE_EXECUTE_READ || mem_info.Protect == PAGE_EXECUTE || mem_info.Protect == PAGE_EXECUTE_WRITECOPY)
				? PAGE_EXECUTE_READWRITE
				: PAGE_READWRITE;

			if (!VirtualProtectEx(handle.get(), reinterpret_cast<void*>(address), size, new_protection, reinterpret_cast<PDWORD>(&old_protection)))
				_zol_throw("read", "Couldn't unprotect memory page");
		}

		if (GetProcessId(handle.get()) == GetCurrentProcessId())
			std::memcpy(buffer, reinterpret_cast<void*>(address), size);
		else
		{
			SIZE_T bytes_read = 0;
			if (!ReadProcessMemory(handle.get(), reinterpret_cast<void*>(address), buffer, size, &bytes_read) || bytes_read != size)
				_zol_throw("read", "Failed to read all bytes from target process");
		}

		if (force && !VirtualProtectEx(handle.get(), reinterpret_cast<void*>(address), size, old_protection, reinterpret_cast<PDWORD>(&old_protection)))
			_zol_throw("read", "Couldn't restore memory protection");

		return true;
	};

	template<typename t_buffer>
	inline auto read(uptr address, usize size, t_buffer* buffer, bool force = true) -> bool
	{
		handles::handle self(OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId()));
		return _read_fn<t_buffer>(self, address, size, buffer, force);
	}

	template<typename t_buffer>
	inline auto read(const handles::handle& handle, uptr address, usize size, t_buffer* buffer, bool force = true) -> bool
	{
		return _read_fn<t_buffer>(handle, address, size, buffer, force);
	}
}
