#pragma once

#include <functional>

#include "exceptions/exceptions.hpp"
#include "handles/handles.hpp"
#include "types/types.hpp"

namespace zol::memory
{
	// This is our write function that can be manipulated by a developer for any reason they want, they may use it in conjunction with a Kernel Driver.
	template<typename t_buffer>
	inline std::function<bool(const handles::handle&, uptr, usize, const t_buffer*, bool)> _write_fn =
		[](const handles::handle& handle, const uptr address, const usize size, const t_buffer* buffer, const bool force = true) -> bool {
		if (!buffer)
			_zol_throw("write", "buffer is null");

		DWORD old_protection = 0;

		if (force)
		{
			MEMORY_BASIC_INFORMATION memory_info { };
			if (!VirtualQueryEx(handle.get(), reinterpret_cast<void*>(address), &memory_info, sizeof(memory_info)))
				//_zol_throw("write", "Couldn't query memory page");
				return false;

			const auto new_protection = (memory_info.Protect == PAGE_EXECUTE_READ || memory_info.Protect == PAGE_EXECUTE || memory_info.Protect == PAGE_EXECUTE_WRITECOPY)
				? PAGE_EXECUTE_READWRITE
				: PAGE_READWRITE;

			if (!VirtualProtectEx(handle.get(), reinterpret_cast<void*>(address), size, new_protection, &old_protection))
				//_zol_throw("write", "Couldn't unprotect memory page");
				return false;
		}

		if (GetProcessId(handle.get()) != GetCurrentProcessId())
		{
			SIZE_T bytes_written = 0;
			if (!WriteProcessMemory(handle.get(), reinterpret_cast<void*>(address), reinterpret_cast<const void*>(buffer), size, &bytes_written) || bytes_written != size)
				//_zol_throw("write", "Failed to write all bytes to target process");
				return false;
		}
		else
			std::memcpy(reinterpret_cast<void*>(address), reinterpret_cast<const void*>(buffer), size);

		if (force && !VirtualProtectEx(handle.get(), reinterpret_cast<void*>(address), size, old_protection, &old_protection))
			//_zol_throw("write", "Couldn't restore memory protection");
			return false;

		return true;
	};

	// safe in-process write
	template<typename t_buffer>
	auto write(uptr address, usize size, const t_buffer* buffer, bool force = true) -> bool
	{
		handles::handle self(OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId()));
		return _write_fn<t_buffer>(self, address, size, buffer, force);
	}

	// safe external write
	template<typename t_buffer>
	auto write(const handles::handle& handle, uptr address, usize size, const t_buffer* buffer, bool force = true) -> bool
	{
		return _write_fn<t_buffer>(handle, address, size, buffer, force);
	}
}
