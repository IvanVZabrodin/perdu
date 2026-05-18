#include "perdu/renderer/area_manager.hpp"

#include "perdu/core/assert.hpp"

#include <cstdint>
#include <iterator>

namespace perdu {
	AreaManager::AreaManager(uint32_t __size) :
		_size(__size), _areas(1, Area(_size, 0, true)) {}

	AreaManager::AreaIt AreaManager::find_free(uint32_t __size) {
		for (auto it = _areas.begin(); it != _areas.end(); ++it) {
			if (it->free && it->size >= __size) return it;
		}
		if (_areas.back().free) return extend(std::prev(_areas.end()), __size);
		return create_size(__size);
	}

	AreaManager::AreaIt AreaManager::allocate(AreaIt __area, uint32_t __size) {
		PERDU_ASSERT(__area->size >= __size,
					 "trying to allocate "
					   + std::to_string(__size)
					   + " in an area of size "
					   + std::to_string(__area->size));
		if (__area->size == __size) {
			__area->free = false;
			return __area;
		}

		auto a			 = split(__area, __size);
		a->free			 = false;
		a->key			 = get_handle();
		_handles[a->key] = a;
		free_area(std::next(a));

		return a;
	}

	void AreaManager::free_area(AreaIt __area) {
		PERDU_ASSERT(!__area->free, "trying to free already free area");
		auto nbase = __area;
		auto nend  = __area;

		if (__area != _areas.begin() && std::prev(__area)->free) {
			nbase = std::prev(__area);
		}

		if (std::next(__area) != _areas.end() && std::next(__area)->free) {
			nend = std::next(__area);
		}

		__area->free = true;
		if (nbase == nend) return;
		uint32_t nsize = nend->offset + nend->size - nbase->offset;
		_areas.erase(std::next(nbase), nend);
		nbase->size = nsize;
	}

	AreaManager::AreaIt AreaManager::resize(AreaIt __area, uint32_t __size) {
		if (__area->size == __size) return __area;
		if (can_extend(__area, __size)) return extend(__area, __size);
		AreaIt nloc = find_free(__size);
		copies.push_back({ __area->offset, nloc->offset, __area->size });
		nloc->key			  = __area->key;
		_handles[__area->key] = nloc;
		free_area(__area);
		nloc->free = false;
		return nloc;
	};

	AreaManager::AreaIt AreaManager::create_size(uint32_t __size) {
		auto& i			= _areas.emplace_back(__size, _size, true);
		i.key			= get_handle();
		_handles[i.key] = std::prev(_areas.end());
		_size		   += __size;
		return std::prev(_areas.end());
	}

	AreaManager::AreaIt AreaManager::extend(AreaIt __area, uint32_t __size) {
		if (__area->size == __size) return __area;
		if (__size < __area->size) {
			AreaIt n		 = split(__area, __size);
			n->key			 = __area->key;
			_handles[n->key] = n;
			free_area(std::next(n));
			return n;
		}
		if (__area == std::prev(_areas.end())) {
			_size		 = __area->offset + __size;
			__area->size = __size;
			return __area;
		}
		auto	 n	   = std::next(__area);
		uint32_t extra = __size - __area->size;
		n->offset	  += extra;
		n->size		  -= extra;
		if (n->size == 0) { _areas.erase(n); }
		return __area;
	}

	bool AreaManager::can_extend(AreaIt __area, uint32_t __size) const {
		if (__area->size <= __size) return true;
		if (__area == std::prev(_areas.end())) return true;
		auto n = std::next(__area);
		return n->free && __area->size + n->size >= __size;
	}

	AreaManager::AreaIt AreaManager::split(AreaIt __area, uint32_t __size) {
		_areas.insert(
		  __area,
		  Area{ __area->size - __size, __area->offset + __size, __area->free });
		__area->size = __size;
		return __area;
	}

	AreaManager::HandleType AreaManager::get_handle() {
		return _next_handle++;
	}

	AreaManager::HandleType AreaManager::create(uint32_t __size) {
		return allocate(find_free(__size), __size)->key;
	}

	void AreaManager::resize(HandleType __handle, uint32_t __size) {
		resize(_handles[__handle], __size);
	}

	void AreaManager::free(HandleType __handle) {
		free_area(_handles[__handle]);
	}

	AreaManager::Area AreaManager::get(HandleType __handle) {
		return *_handles[__handle];
	}

	void AreaManager::ensure(HandleType __handle, uint32_t __size) {
		auto a = _handles[__handle];
		if (a->size < __size) resize(a, __size);
	}

	UnfragmentedArea::UnfragmentedArea(uint32_t __size) : AreaManager(__size) {}

	void UnfragmentedArea ::defragment() {}
}
