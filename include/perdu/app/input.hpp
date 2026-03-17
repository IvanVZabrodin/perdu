#pragma once
#include "perdu/core/event.hpp"

#include <bitset>
#include <cstdint>
#include <entt/entt.hpp>
#include <limits>

namespace perdu {
	enum class Key : uint16_t {
		A = 4,
		B = 5,
		C = 6,
		D = 7,
		E = 8,
		F = 9,
		G = 10,
		H = 11,
		I = 12,
		J = 13,
		K = 14,
		L = 15,
		M = 16,
		N = 17,
		O = 18,
		P = 19,
		Q = 20,
		R = 21,
		S = 22,
		T = 23,
		U = 24,
		V = 25,
		W = 26,
		X = 27,
		Y = 28,
		Z = 29,

		Num1 = 30,
		Num2 = 31,
		Num3 = 32,
		Num4 = 33,
		Num5 = 34,
		Num6 = 35,
		Num7 = 36,
		Num8 = 37,
		Num9 = 38,
		Num0 = 39,

		Enter	  = 40,
		Escape	  = 41,
		Backspace = 42,
		Tab		  = 43,
		Space	  = 44,

		F1	= 58,
		F2	= 59,
		F3	= 60,
		F4	= 61,
		F5	= 62,
		F6	= 63,
		F7	= 64,
		F8	= 65,
		F9	= 66,
		F10 = 67,
		F11 = 68,
		F12 = 69,

		Right = 79,
		Left  = 80,
		Down  = 81,
		Up	  = 82,

		LCtrl  = 224,
		LShift = 225,
		LAlt   = 226,
		LSuper = 227,
		RCtrl  = 228,
		RShift = 229,
		RAlt   = 230,
		RSuper = 231,

		Unknown = 0
	};

	enum class MouseButton : uint8_t {
		Left	= 1,
		Middle	= 2,
		Right	= 3,
		X1		= 4,
		X2		= 5,
		Unknown = 0
	};

	class InputHandler {
	  public:
		struct InputState
		{
			bool is_key_down(Key k) const {
				return _keys[static_cast<uint16_t>(k)];
			}
			bool is_mouse_down(MouseButton b) const {
				return _mouse[static_cast<uint8_t>(b)];
			}

			struct MouseState
			{
				float x = 0, y = 0;
				float dx = 0, dy = 0;
			} mouse{};

		  private:
			friend class InputHandler;
			std::bitset<512> _keys{};
			std::bitset<8>	 _mouse{};
		};


		InputHandler(entt::registry& reg);

		const EventBus&	  bus() const { return _bus; }
		EventBus&		  bus() { return _bus; }
		const EventQueue& queue() const { return _queue; }
		EventQueue&		  queue() { return _queue; }

		void poll();

	  private:
		EventBus   _bus;
		EventQueue _queue;
	};

	namespace events {
		enum class PressAction : bool { pressed = true, released = false };

		struct KeyEvent
		{
			Key			key;
			PressAction action = PressAction::pressed;
		};

		struct MouseEvent
		{
			MouseButton button;
			PressAction action = PressAction::pressed;
		};

		struct MouseMoved
		{
			float x = 0, y = 0;
			float dx = 0, dy = 0;
		};

		struct WindowQuit
		{};
		struct WindowResized
		{
			uint32_t width, height;
		};

	}
}
