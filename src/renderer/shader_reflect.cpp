#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/shader.hpp"

#include <algorithm>
#include <cstdint>
#include <spirv_cross.hpp>
#include <utility>

static uint32_t format_size(perdu::VertexAttribute::Format format) {
	switch (format) {
		case perdu::VertexAttribute::Format::Float : return 4;
		case perdu::VertexAttribute::Format::Float2: return 8;
		case perdu::VertexAttribute::Format::Float3: return 12;
		case perdu::VertexAttribute::Format::Float4: return 16;
	}
	return 0;
}

static perdu::VertexAttribute::Format
  to_perduformat(const spirv_cross::SPIRType& type) {
	if (type.basetype == spirv_cross::SPIRType::Float) {
		switch (type.vecsize) {
			case 1: return perdu::VertexAttribute::Format::Float;
			case 2: return perdu::VertexAttribute::Format::Float2;
			case 3: return perdu::VertexAttribute::Format::Float3;
			case 4: return perdu::VertexAttribute::Format::Float4;
		}
	}
	PERDU_LOG_ERROR("Unsupported vertex attribute type, treating as float");
	return perdu::VertexAttribute::Format::Float;
}

static std::string to_formatstring(perdu::VertexAttribute::Format format) {
	switch (format) {
		case perdu::VertexAttribute::Format::Float : return "Float";
		case perdu::VertexAttribute::Format::Float2: return "Float2";
		case perdu::VertexAttribute::Format::Float3: return "Float3";
		case perdu::VertexAttribute::Format::Float4: return "Float4";
	}
	return "Unsupported";
}

static std::string attr_to_string(std::vector<perdu::VertexAttribute> attrs,
								  std::string pref = "") {
	std::string res{};
	for (auto& a : attrs) {
		res += pref
			 + "{ loc: "
			 + std::to_string(a.location)
			 + ", format: "
			 + to_formatstring(a.format)
			 + ", offset: "
			 + std::to_string(a.offset)
			 + " }\n";
	}
	return res;
}

namespace perdu {
	void reflect(CPUShader& cpu) {
		PERDU_ASSERT(cpu.spirv.size() % 4 == 0, "invalid shader spirv size");
		PERDU_LOG_DEBUG("reflecting shader");

		spirv_cross::Compiler comp(
		  reinterpret_cast<const uint32_t*>(cpu.spirv.data()),
		  cpu.spirv.size() / sizeof(uint32_t));
		auto res = comp.get_shader_resources();

		if (cpu.stage == ShaderStage::Vertex) {
			std::vector<std::pair<uint32_t, spirv_cross::Resource>>
			  sorted_inputs;

			for (auto& i : res.stage_inputs) {
				uint32_t loc
				  = comp.get_decoration(i.id, spv::DecorationLocation);
				sorted_inputs.push_back({ loc, i });
			}

			std::sort(sorted_inputs.begin(),
					  sorted_inputs.end(),
					  [](auto& a, auto& b) { return a.first < b.first; });

			uint32_t offset = 0;
			for (auto& [loc, input] : sorted_inputs) {
				auto& type	 = comp.get_type(input.base_type_id);
				auto  format = to_perduformat(type);
				cpu.attributes.push_back({ loc, format, offset });
				offset += format_size(format);
			}
			cpu.vertex_strides = offset;
			PERDU_LOG_DEBUG("parsed shader as:\n\tstrides: "
							+ std::to_string(offset)
							+ "\n\tAttributes:\n"
							+ attr_to_string(cpu.attributes, "\t\t"));
		}

		cpu.uniform_buffers = (uint32_t) res.uniform_buffers.size();
		cpu.storage_buffers = (uint32_t) res.storage_buffers.size();
		cpu.samplers		= (uint32_t) res.sampled_images.size();
	}
}
