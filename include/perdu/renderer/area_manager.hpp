#pragma once

#include <cstdint>
#include <list>
#include <queue>
#include <unordered_map>
#include <vector>
namespace perdu {
	class AreaManager {
	  public:
		AreaManager(uint32_t size);

		using HandleType = uint32_t;

		struct Area
		{
			uint32_t   size;
			uint32_t   offset;
			bool	   free;
			HandleType key;
		};

		virtual HandleType create(uint32_t size);
		virtual void	   ensure(HandleType handle, uint32_t size);
		virtual void	   resize(HandleType handle, uint32_t size);
		virtual void	   free(HandleType handle);
		virtual Area	   get(HandleType handle);

		struct AreaCopy
		{
			uint32_t src;
			uint32_t dst;
			uint32_t size;
		};

		std::vector<AreaCopy> copies;

	  protected:
		HandleType _next_handle = 1;


		using AreaIt = std::list<Area>::iterator;

		std::unordered_map<HandleType, AreaIt> _handles;

		uint32_t		_size;
		std::list<Area> _areas;

		AreaIt find_free(uint32_t size);
		AreaIt allocate(AreaIt area, uint32_t size);
		void   free_area(AreaIt area);
		AreaIt resize(AreaIt area, uint32_t size);
		AreaIt extend(AreaIt area, uint32_t size);
		AreaIt split(AreaIt area, uint32_t size);
		AreaIt create_size(uint32_t size);

		HandleType get_handle();

		bool can_extend(AreaIt area, uint32_t size) const;
	};

	class UnfragmentedArea : public AreaManager {
	  public:
		UnfragmentedArea(uint32_t size);

		void defragment();

	  private:
	};
}
