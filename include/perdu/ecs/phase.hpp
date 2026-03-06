#pragma once

namespace perdu {
	enum class Phase {
		Input,
		PreUpdate,
		Update,
		PostUpdate,
		PreRender,
		Render,
		UI,
		PostRender
	};
}
