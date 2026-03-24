#pragma once

namespace zol
{
	template<typename t_>
	class singleton
	{
	public:
		inline auto instance() noexcept(noexcept(t_ { })) -> t_&
		{
			static t_ instance { };
			return instance;
		}

		singleton(const singleton&)			   = delete;
		singleton& operator=(const singleton&) = delete;

		singleton(singleton&&)			  = delete;
		singleton& operator=(singleton&&) = delete;

	protected:
		singleton()	 = default;
		~singleton() = default;
	};
}
