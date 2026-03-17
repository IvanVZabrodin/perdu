#pragma once

#include "perdu/core/assert.hpp"

#include <any>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace perdu {
	class EventManager {
	  public:
		using SubscriptionID = uint32_t;

		template <typename T>
		using Handler = std::function<void(T)>;

		template <typename... Ts>
		EventManager(Handler<Ts>... handlers) {
			(subscribe<Ts>(handlers), ...);
		}


		template <typename T>
		SubscriptionID subscribe(this auto&&	   self,
								 Handler<const T&> handler,
								 int			   priority = 50) {
			SubscriptionID sid = self._nextid++;
			self._handlers[typeid(T)].push(
			  { sid, priority, [h = std::move(handler)](const std::any& e) {
				   h(std::any_cast<const T&>(e));
			   } });
			self._subhook(typeid(T));
			return sid;
		}

		SubscriptionID subscribe_by_id(this auto&&				self,
									   std::type_index			id,
									   Handler<const std::any&> handler,
									   int						priority = 50) {
			SubscriptionID sid = self._nextid++;
			self._handlers[id].push(
			  { sid, priority, [h = std::move(handler)](const std::any& e) {
				   h(e);
			   } });
			self._subhook(id);
			return sid;
		}


		template <typename T>
		void unsubscribe(SubscriptionID sid) {
			unsubscribe_by_id(typeid(T), sid);
		}

		void unsubscribe_by_id(this auto&&	   self,
							   std::type_index id,
							   SubscriptionID  sid) {
			auto it = self._handlers.find(id);
			if (it == self._handlers.end()) return;
			auto& ev = it->second;
			auto  e	 = ev;
			ev		 = {};
			for (; !e.empty(); e.pop()) {
				if (e.top().id == sid) continue;
				ev.push(e.top());
			}
			self._unsubhook(id);
		}


		template <typename T>
		void emit(T event) const {
			emit_by_id(typeid(T), std::move(event));
		}

		void emit_by_id(std::type_index id, std::any event) const {
			auto it = _handlers.find(id);
			if (it == _handlers.end()) return;
			auto events = it->second;
			for (; !events.empty(); events.pop()) events.top().handler(event);
		}

	  protected:
		struct Entry
		{
			SubscriptionID	  id;
			int				  priority;
			Handler<std::any> handler;

			friend bool operator<(const Entry& a, const Entry& b) {
				return a.priority < b.priority;
			}

			friend bool operator>(const Entry& a, const Entry& b) {
				return a.priority > b.priority;
			}
		};

		void _subhook(std::type_index){};
		void _unsubhook(std::type_index){};

		std::unordered_map<
		  std::type_index,
		  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>>>
					   _handlers;
		SubscriptionID _nextid = 0;
	};

	using EventBus = EventManager;

	class EventQueue : public EventManager {
	  public:
		using BusID = uint32_t;

		template <typename... Ts>
		EventQueue(EventBus* bus, Handler<Ts>... handlers) :
			EventManager(handlers...) {
			bind(bus);
		}

		~EventQueue() { unbind_all(); }

		BusID bind(EventBus* bus) {
			PERDU_ASSERT(bus, "cannot bind nullptr bus");
			BusID bid	= _nextbid++;
			_buses[bid] = { bus, {} };
			for (auto [id, _] : _handlers) listen(bid, id);
			return bid;
		}


		void unbind(BusID bid) {
			auto it = _buses.find(bid);
			if (it == _buses.end()) return;
			QueueBus& qbus = it->second;
			for (auto [id, sid] : qbus.listeners)
				qbus.bus->unsubscribe_by_id(id, sid);
			_buses.erase(it);
		}

		void unbind_all() {
			std::vector<BusID> ids;
			ids.reserve(_buses.size());
			for (auto& [id, _] : _buses) ids.push_back(id);
			for (BusID id : ids) unbind(id);
		}


		void listen(BusID bid, std::type_index id) {
			QueueBus&	   qbus = _buses[bid];
			SubscriptionID lid	= qbus.bus->subscribe_by_id(
			   id,
			   [bid, id, this](const std::any& e) {
				   _events.push({ bid, id, e });
			   },
			   49);
			qbus.listeners[id] = lid;
		}

		void unlisten(BusID bid, std::type_index id) {
			QueueBus& qbus = _buses[bid];
			auto	  it   = qbus.listeners.find(id);
			if (it == qbus.listeners.end()) return;
			qbus.bus->unsubscribe_by_id(id, it->second);
		}


		bool digest_one() {
			if (_events.empty()) return false;
			Event e = _events.front();
			_events.pop();
			emit_by_id(e.type, e.data);
			return !_events.empty();
		};

		bool digest(size_t n = std::numeric_limits<size_t>::max()) {
			size_t i = 0;
			while (i < n && digest_one()) ++i;
			return !_events.empty();
		}

		void clear() { _events = {}; }

	  private:
		friend class EventManager;
		void _subhook(std::type_index id) {
			for (auto b : _buses) listen(b.first, id);
		}
		void _unsubhook(std::type_index id) {
			for (auto b : _buses) unlisten(b.first, id);
		}


		struct Event
		{
			BusID			origin;
			std::type_index type;
			std::any		data;
		};

		struct QueueBus
		{
			EventBus*											bus;
			std::unordered_map<std::type_index, SubscriptionID> listeners;
		};

		std::queue<Event> _events;

		std::unordered_map<BusID, QueueBus> _buses;
		BusID								_nextbid = 0;
	};
}
