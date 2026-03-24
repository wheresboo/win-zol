#pragma once

#include <string>
#include <memory>
#include <psapi.h>
#include <TlHelp32.h>
#include <functional>
#include <vector>

#include "types/types.hpp"
#include "handles/handles.hpp"
#include "memory/read.hpp"
#include "exceptions/exceptions.hpp"

namespace zol::module
{
	template<typename t_>
	using t_clip_call = std::function<t_(MODULEENTRY32W)>;

	template<typename t_>
	static auto clip_module(const u32 process_id, const std::wstring& module_name, t_clip_call<t_> callback) -> t_
	{
		t_ data = { };
		const handles::handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id));

		if (!snapshot)
			return data;

		MODULEENTRY32W module_entry = { };
		module_entry.dwSize			= sizeof(MODULEENTRY32W);

		if (Module32FirstW(snapshot.get(), &module_entry))
		{
			do
			{
				if (lstrcmpiW(module_name.c_str(), module_entry.szModule) == 0)
					return callback(module_entry);
			}
			while (Module32NextW(snapshot.get(), &module_entry));
		}

		return data;
	}

	class t_module
	{
	protected:
		u32 _process_id;
		std::wstring _module_name;

	public:
		explicit t_module(const u32 process_id, const std::wstring& module_name)
			: _process_id(process_id)
			, _module_name(module_name)
		{
		}

		[[nodiscard]] auto size() const -> usize
		{
			return clip_module<usize>(_process_id, _module_name, [](const MODULEENTRY32W& e) -> usize {
				return e.modBaseSize;
			});
		}

		[[nodiscard]] auto path() const -> std::wstring
		{
			return clip_module<std::wstring>(_process_id, _module_name, [](const MODULEENTRY32W& e) -> std::wstring {
				return e.szExePath;
			});
		}

		[[nodiscard]] auto base_address() const -> uptr
		{
			return clip_module<uptr>(_process_id, _module_name, [](const MODULEENTRY32W& e) -> uptr {
				return reinterpret_cast<uptr>(e.modBaseAddr);
			});
		}

		[[nodiscard]] auto rebase(const uptr address) const -> uptr
		{
			return base_address() + address;
		}

		[[nodiscard]] auto find_export(const std::string& export_name) const -> uptr
		{
			const handles::handle proc_handle(OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, _process_id));
			if (!proc_handle)
				return 0;

			const auto base = base_address();
			if (!base)
				return 0;

			IMAGE_DOS_HEADER dos_header = { };
			if (!memory::read(proc_handle, base, sizeof(dos_header), &dos_header, false))
				return 0;

			IMAGE_NT_HEADERS nt_headers = { };
			if (!memory::read(proc_handle, base + dos_header.e_lfanew, sizeof(nt_headers), &nt_headers, false))
				return 0;

			const auto& export_directory = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			if (export_directory.VirtualAddress == 0 || export_directory.Size == 0)
				return 0;

			IMAGE_EXPORT_DIRECTORY exports = { };
			if (!memory::read(proc_handle, base + export_directory.VirtualAddress, sizeof(exports), &exports, false))
				return 0;

			std::vector<DWORD> names(exports.NumberOfNames);
			if (!memory::read(proc_handle, base + exports.AddressOfNames, sizeof(DWORD) * names.size(), names.data(), false))
				return 0;

			std::vector<WORD> ordinals(exports.NumberOfNames);
			if (!memory::read(proc_handle, base + exports.AddressOfNameOrdinals, sizeof(WORD) * ordinals.size(), ordinals.data(), false))
				return 0;

			std::vector<DWORD> functions(exports.NumberOfFunctions);
			if (!memory::read(proc_handle, base + exports.AddressOfFunctions, sizeof(DWORD) * functions.size(), functions.data(), false))
				return 0;

			for (usize idx = 0; idx < names.size(); ++idx)
			{
				char name_buffer[256] = { };
				if (!memory::read(proc_handle, base + names[idx], sizeof(name_buffer) - 1, name_buffer, false))
					continue;

				if (export_name == name_buffer)
				{
					const auto ordinal = ordinals[idx];
					if (ordinal >= functions.size())
						return 0;

					const auto rva = functions[ordinal];
					if (rva >= export_directory.VirtualAddress && rva < export_directory.VirtualAddress + export_directory.Size)
						return 0;

					return base + rva;
				}
			}

			return 0;
		}
	};
}
