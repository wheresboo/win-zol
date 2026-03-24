#pragma once

#include <string>
#include <memory>
#include <psapi.h>
#include <TlHelp32.h>

#include "process.hpp"
#include "types/types.hpp"
#include "handles/handles.hpp"
#include "exceptions/exceptions.hpp"

namespace zol::process
{
	inline auto open_with_id(const u32 process_id) -> std::unique_ptr<t_process>
	{
		return std::make_unique<t_process>(process_id);
	}

	// ts case-sensitive asf icl
	inline auto open_with_name(const std::wstring& process_name) -> std::unique_ptr<t_process>
	{
		handles::handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
		if (snapshot.get() == INVALID_HANDLE_VALUE)
			return nullptr;

		PROCESSENTRY32W process_entry;
		process_entry.dwSize = sizeof(PROCESSENTRY32W);

		u32 process_id = 0;
		if (Process32FirstW(snapshot.get(), &process_entry))
		{
			do
			{
				// if (process_name == process_entry.szExeFile)
				if (process_name.find(process_entry.szExeFile) != std::wstring::npos)
				{
					process_id = process_entry.th32ProcessID;
					break;
				}
			}
			while (Process32NextW(snapshot.get(), &process_entry));
		}

		snapshot.release();

		if (!process_id)
			_zol_throw(__FUNCTION__, "couldn't find a process with the name \"{}\"", process_name.c_str());

		return open_with_id(process_id);
	}
}
