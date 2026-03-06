#pragma once

#include "perdu/core/assert.hpp"
#include "perdu/core/hash.hpp"
#include "perdu/core/log.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace perdu {
	template <typename T>
	class ResourceCache {
	  public:
		using DestroyFunction = void (*)(T*);
		size_t chunk_size	  = 8;

		class Handle {
		  public:
			Handle() = default;

			// Can't copy handles. Might just link to clone
			Handle(const Handle&)			= delete;
			Handle operator=(const Handle&) = delete;

			Handle(Handle&& other) noexcept {
				_cache		 = other._cache;
				_id			 = other._id;
				other._cache = nullptr;
				other._id	 = 0;
			};

			Handle& operator=(Handle&& other) noexcept {
				release();
				_cache		 = other._cache;
				_id			 = other._id;
				other._cache = nullptr;
				other._id	 = 0;
				return *this;
			}

			~Handle() {
				if (_cache == nullptr) return;
				release();
			}

			bool valid() const {
				if (_cache == nullptr) return false;
				return _cache->valid(_id);
			}

			void release() {
				if (!valid()) {
					PERDU_LOG_WARN("Trying to release invalid handle");
					return;
				}

				_cache->release(_id);
				_cache = nullptr;
				_id	   = 0;
			};

			const T& get() const { return _cache->get(_id); }
			T&		 get() { return _cache->get(_id); }

			const T* operator->() const { return &get(); }
			T*		 operator->() { return &get(); }

			uint32_t	   get_id() const { return _id; }
			ResourceCache* get_cache() const { return _cache; }

			Handle clone() const {
				PERDU_ASSERT(valid(), "Trying to clone invalid handle");
				_cache->_data[_id].refcount++;
				return Handle{ _cache, _id };
			}

		  private:
			friend class ResourceCache;

			Handle(ResourceCache* cache, uint32_t id) :
				_cache(cache), _id(id) {}

			ResourceCache* _cache = nullptr;
			uint32_t	   _id	  = 0;
		};

		ResourceCache() = default;
		ResourceCache(DestroyFunction destroyer) : _destroyer(destroyer) {}

		void set_destroyer(DestroyFunction destroyer) {
			_destroyer = destroyer;
		}
		DestroyFunction get_destroyer() const { return _destroyer; }

		template <typename... Args>
		Handle emplace(std::string_view name,
					   bool				persistent = false,
					   Args... args) {
			auto ptr = new T(std::forward<Args>(args)...);
			return store(name, ptr, persistent);
		};

		Handle
		  store(std::string_view name, T* resource, bool persistent = false) {
			uint32_t h = hash(name);
			return manage(h, resource, persistent);
		}

		Handle get(std::string_view name) {
			if (!_ntid.contains(hash(name))) {
				PERDU_LOG_WARN("Trying to get unknown resource with name "
							   + std::string(name));
				return { nullptr, 0 };
			}
			uint32_t id = _ntid.at(hash(name));
			return get(id);
		}

	  private:
		friend class Handle;

		struct Slot
		{
			T*	 data  = nullptr;
			bool alive = false;
			bool persistent
			  = false; // Should it stay alive even with zero refs?
			size_t refcount = 0;
		};

		uint32_t							   _next_id = 0;
		std::vector<uint32_t>				   _free;
		std::vector<Slot>					   _data;
		std::unordered_map<uint32_t, uint32_t> _ntid;
		DestroyFunction _destroyer = [](T* d) { delete d; };

		[[nodiscard]] uint32_t get_id() {
			if (!_free.empty()) {
				uint32_t id = _free.back();
				_free.pop_back();
				return id;
			}
			return _next_id++;
		}

		Handle manage(uint32_t hash, T* resource, bool persistent) {
			uint32_t id = get_id();
			if (id >= _data.size()) {
				PERDU_LOG_DEBUG("Increasing resource cache capacity");
				_data.resize(((size_t) ((id + 1) / chunk_size) + 1)
							 * chunk_size);
			}
			_ntid[hash] = id;
			PERDU_ASSERT(!_data[id].alive,
						 "Trying to overwrite already alive slot");
			_data[id] = { .data		  = resource,
						  .alive	  = true,
						  .persistent = persistent,
						  .refcount	  = 1 };

			return Handle{ this, id };
		}

		void release(uint32_t id) {
			Slot& s = _data[id];
			PERDU_ASSERT(s.alive, "Trying to free dead resource");
			s.refcount--;
			if (!s.persistent && s.refcount == 0) destroy(id);
		}

		void destroy(uint32_t id) {
			PERDU_ASSERT(_data[id].alive, "Trying to destroy dead resource");
			PERDU_LOG_INFO("Freeing resource id " + std::to_string(id));
			Slot& s = _data[id];

			_destroyer(s.data);
			s.data		 = nullptr;
			s.persistent = false;
			s.alive		 = false;

			_free.push_back(id);
		}

		bool valid(uint32_t id) {
			if (_data.size() <= id) return false;
			return _data[id].alive;
		}

		const T& get(uint32_t id) const {
			PERDU_ASSERT(valid(id), "Trying to get invalid resource");
			return *_data[id].data;
		}

		T& get(uint32_t id) {
			PERDU_ASSERT(valid(id), "Trying to get invalid resource");
			return *_data[id].data;
		}
	};
}
