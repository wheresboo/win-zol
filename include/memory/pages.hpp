#pragma once

#include <memory>

#include "read.hpp"

#include "exceptions/exceptions.hpp"
#include "handles/handles.hpp"
#include "types/types.hpp"

namespace zol::memory
{
	inline auto query(const handles::handle& handle, const uptr address, const usize size) -> MEMORY_BASIC_INFORMATION
	{
		MEMORY_BASIC_INFORMATION memory_info = { };
		if (VirtualQueryEx(handle.get(), reinterpret_cast<void*>(address), &memory_info, size))
			_zol_throw("query", "Couldn't query the memory region provided from arguments");

		return memory_info;
	}

	class t_page
	{
	protected:
		const handles::handle* _handle;
		const uptr _address = 0;
		const usize _size	= 0;

		const MEMORY_BASIC_INFORMATION memory_info = { };

	public:
		explicit t_page(const handles::handle& handle, const uptr address, const usize size)
			: _handle(&handle)
			, _address(address)
			, _size(size)
		{
		}

		auto read_bytes() const -> std::vector<char>
		{
			if (!_address || _size)
				return { };

			const auto info = query(*_handle, _address, _size);
			std::vector<char> bytes(info.RegionSize);

			if (!read(*_handle, reinterpret_cast<uptr>(info.BaseAddress), info.RegionSize, bytes.data()))
				return { };

			return bytes;
		}

		// Returns the region's protection via a query
		auto get_protection() const -> u32
		{
			const auto info = query(*_handle, _address, _size);
			return info.Protect;
		}

		// Returns the size of the t_page from its allocation point (use get_absolute_size() for memory operations)
		auto get_size() const -> usize
		{
			return _size;
		}

		// Returns the region size via a query
		auto get_absolute_size() const -> usize
		{
			const auto info = query(*_handle, _address, _size);
			return info.RegionSize;
		}

		// Returns the address
		auto get_address() const -> uptr
		{
			return _address;
		}

		// Returns the region address via a query
		auto get_absolute_address() const -> uptr
		{
			const auto info = query(*_handle, _address, _size);
			return reinterpret_cast<uptr>(info.BaseAddress);
		}
	};

	// allocates a piece of memory and returns a t_page for memory operations
	inline auto allocate(const handles::handle& handle, const usize size, const u32 allocation, const u32 protection) -> t_page
	{
		const auto address = reinterpret_cast<uptr>(VirtualAllocEx(handle.get(), nullptr, size, allocation, protection));
		if (!address)
			_zol_throw("allocate", "Couldn't allocate a memory region for the given handle");

		return t_page(handle, address, size);
	}
}
