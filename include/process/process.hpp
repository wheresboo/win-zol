#pragma once

#include <Windows.h>
#include <string>
#include <psapi.h>
#include <TlHelp32.h>
#include <vector>

// mem ops
#include "memory/write.hpp"
#include "memory/read.hpp"

#include "types/types.hpp"
#include "module/module.hpp"
#include "handles/handles.hpp"
#include "exceptions/exceptions.hpp"

namespace zol::process
{
	class t_process;

	// libraries
	class t_modules
	{
	protected:
		const t_process& process;

	public:
		explicit t_modules(const t_process& p);

		auto get_module(const std::wstring& module_name) const -> std::unique_ptr<module::t_module>;
		auto get_module(uptr address) const -> std::unique_ptr<module::t_module>;
	};

	class t_memory
	{
	protected:
		const t_process& process;

	public:
		explicit t_memory(const t_process& p);

		auto allocate(usize size, u32 allocation_type = MEM_COMMIT | MEM_RESERVE, u32 protection = PAGE_EXECUTE_READWRITE) const -> uptr;
		auto free(uptr address, usize size = 0, u32 free_type = MEM_RELEASE) const -> void;
		auto protect(uptr address, usize size, u32 new_protection) const -> u32;

		template<typename t_>
		auto read(uintptr_t address) const -> t_;

		template<typename T>
		auto write(uintptr_t address, const T& value) const -> void;
	};

	class t_process: public t_modules,
					 public t_memory
	{
	protected:
		u32 _process_id = 0;
		handles::handle _handle;

	public:
		explicit t_process(u32 process_id)
			: t_modules(*this)
			, t_memory(*this)
			, _process_id(process_id)
			, _handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id))
		{
			if (!_handle.get())
				_zol_throw(__FUNCTION__, "failed to open process id ", process_id);
		}

		~t_process() = default;

		t_process(const t_process&)			   = delete;
		t_process& operator=(const t_process&) = delete;

		t_process(t_process&&) noexcept			   = default;
		t_process& operator=(t_process&&) noexcept = delete;

		[[nodiscard]] u32 id() const
		{
			return _process_id;
		}

		[[nodiscard]] HANDLE handle() const
		{
			return _handle.get();
		}

		[[nodiscard]] const handles::handle& handle_wrapper() const
		{
			return _handle;
		}

		[[nodiscard]] bool valid() const
		{
			return _handle.get() != nullptr;
		}

		auto name() const -> std::wstring
		{
			wchar_t buffer[MAX_PATH] { };

			if (GetModuleBaseNameW(_handle.get(), nullptr, buffer, MAX_PATH))
				return buffer;

			_zol_throw(__FUNCTION__, "failed to get process name");
			return L"";
		}
	};
}

inline zol::process::t_modules::t_modules(const t_process& p)
	: process(p)
{
}

inline zol::process::t_memory::t_memory(const t_process& p)
	: process(p)
{
}

inline auto zol::process::t_modules::get_module(const std::wstring& module_name) const -> std::unique_ptr<module::t_module>
{
	const handles::handle snapshot(CreateToolhelp32Snapshot(
		TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
		process.id()));

	if (snapshot.get() == INVALID_HANDLE_VALUE)
		_zol_throw(__FUNCTION__, "failed to create module snapshot");

	MODULEENTRY32W mod { };
	mod.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(snapshot.get(), &mod))
	{
		do
		{
			if (_wcsicmp(module_name.c_str(), mod.szModule) == 0)
				return std::make_unique<module::t_module>(process.id(), mod.szModule);
		}
		while (Module32NextW(snapshot.get(), &mod));
	}

	_zol_throw(__FUNCTION__, "module not found: {}", std::string(module_name.begin(), module_name.end()));
	return nullptr;
}

inline auto zol::process::t_modules::get_module(const uptr address) const -> std::unique_ptr<module::t_module>
{
	const handles::handle snapshot(CreateToolhelp32Snapshot(
		TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
		process.id()));

	if (snapshot.get() == INVALID_HANDLE_VALUE)
		_zol_throw(__FUNCTION__, "failed to create module snapshot");

	MODULEENTRY32W mod { };
	mod.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(snapshot.get(), &mod))
	{
		do
		{
			const auto base = reinterpret_cast<uptr>(mod.modBaseAddr);
			if (const auto end = base + mod.modBaseSize; address >= base && address < end)
				return std::make_unique<module::t_module>(process.id(), mod.szModule);
		}
		while (Module32NextW(snapshot.get(), &mod));
	}

	return nullptr;
}

inline auto zol::process::t_memory::allocate(const usize size, const u32 allocation_type, const u32 protection) const -> uptr
{
	const auto address = VirtualAllocEx(process.handle(), nullptr, size, allocation_type, protection);
	if (!address)
		_zol_throw(__FUNCTION__, "failed to allocate memory");

	return reinterpret_cast<uptr>(address);
}

inline auto zol::process::t_memory::free(const uptr address, const usize size, const u32 free_type) const -> void
{
	if (!VirtualFreeEx(process.handle(), reinterpret_cast<void*>(address), size, free_type))
		_zol_throw(__FUNCTION__, "failed to free memory");
}

inline auto zol::process::t_memory::protect(const uptr address, const usize size, const u32 new_protection) const -> u32
{
	DWORD old = 0;

	if (!VirtualProtectEx(process.handle(), reinterpret_cast<void*>(address), size, new_protection, &old))
		_zol_throw(__FUNCTION__, "failed to protect memory");

	return static_cast<u32>(old);
}

template<typename t_>
auto zol::process::t_memory::read(uintptr_t address) const -> t_
{
	t_ buffer { };

	if (!memory::read(process.handle_wrapper(), address, sizeof(t_), &buffer, true))
		_zol_throw("process", "read failed at address ", address);

	return buffer;
}

template<typename t_>
auto zol::process::t_memory::write(uintptr_t address, const t_& value) const -> void
{
	if (!memory::write(process.handle_wrapper(), address, sizeof(t_), &value, true))
		_zol_throw("process", "write failed at address ", address);
}
