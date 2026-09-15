#include "renderer/pipeline.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "perdu/renderer/shader.hpp"
#include "renderer/gpu_context.hpp"
#include "renderer/shader.hpp"
#include "vulkan/vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

static SDL_GPUVertexElementFormat
  to_sdlformat(perdu::VertexAttribute::Format format) {
	switch (format) {
		case (perdu::VertexAttribute::Format::Float2):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		case (perdu::VertexAttribute::Format::Float3):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		case (perdu::VertexAttribute::Format::Float4):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		case (perdu::VertexAttribute::Format::Float):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	}
}

static SDL_GPUPrimitiveType to_sdlprimitive(perdu::PrimitiveType type) {
	switch (type) {
		case (perdu::PrimitiveType::Triangles):
			return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		case (perdu::PrimitiveType::Lines):
			return SDL_GPU_PRIMITIVETYPE_LINELIST;
		case (perdu::PrimitiveType::Points):
			return SDL_GPU_PRIMITIVETYPE_POINTLIST;
	}
}

vk::Format to_vkformat(perdu::VertexAttribute::Format format) {
	switch (format) {
		case perdu::VertexAttribute::Format::Float:
			return vk::Format::eR32Sfloat;
		case perdu::VertexAttribute::Format::Float2:
			return vk::Format::eR32G32Sfloat;
		case perdu::VertexAttribute::Format::Float3:
			return vk::Format::eR32G32B32Sfloat;
		case perdu::VertexAttribute::Format::Float4:
			return vk::Format::eR32G32B32A32Sfloat;
	}
}

vk::DescriptorType to_vkdescriptortype(perdu::DescriptorBinding::Type type) {
	switch (type) {
		case perdu::DescriptorBinding::Type::UniformBuffer:
			return vk::DescriptorType::eUniformBuffer;
		case perdu::DescriptorBinding::Type::StorageBuffer:
			return vk::DescriptorType::eStorageBuffer;
	}
}

namespace perdu {
	Pipeline::Pipeline(GPUContext*				 ctx,
					   PipelineType				 type,
					   std::vector<ShaderHandle> shaders,
					   Swapchain*				 swp) :
		_ctx(ctx), _shaders(shaders), _type(type) {
		for (auto& shader : _shaders) {
			_shaderstages.push_back(shader->gpu->to_pipelineinfo());
		}

		switch (_type) {
			case PipelineType::Graphics:
				create_graphics_pipeline(&swp->format.format);
				break;
			case PipelineType::Compute: create_compute_pipeline(); break;
		}
		// std::vector<SDL_GPUVertexAttribute> attrs;
		// for (auto& a : desc.attributes) {
		// 	attrs.push_back({ .location = a.location,
		// 					  .format	= to_sdlformat(a.format),
		// 					  .offset	= a.offset });
		// }
		//
		// SDL_GPUVertexBufferDescription vbd{ .slot  = 0,
		// 									.pitch = desc.vertex_stride,
		// 									.input_rate
		// 									= SDL_GPU_VERTEXINPUTRATE_VERTEX };
		//
		// SDL_GPUVertexInputState vertex_input{ .vertex_buffer_descriptions
		// 									  = &vbd,
		// 									  .num_vertex_buffers = 1,
		// 									  .vertex_attributes = attrs.data(),
		// 									  .num_vertex_attributes
		// 									  = (uint32_t) attrs.size() };
		//
		// SDL_GPUColorTargetDescription color_target{ .format
		// 											= desc.colour_format };
		//
		// SDL_GPUGraphicsPipelineCreateInfo info {
		// 	.vertex_shader = desc.vert->gpu.handle(),
		// 	.fragment_shader = desc.frag->gpu.handle(),
		// 	.vertex_input_state = vertex_input,
		// 	.primitive_type = to_sdlprimitive(desc.primitive_type),
		// 	.rasterizer_state = {
		// 		.cull_mode = SDL_GPU_CULLMODE_NONE
		// 	},
		// 	.depth_stencil_state = {
		// 		.compare_op = SDL_GPU_COMPAREOP_GREATER,
		// 		.enable_depth_test = desc.depth_test,
		// 		.enable_depth_write = desc.depth_write,
		// 	},
		// 	.target_info = {
		// 		.color_target_descriptions = &color_target,
		// 		.num_color_targets = 1,
		// 		.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		// 		.has_depth_stencil_target = true
		// 	},
		// };
		//
		// _pipeline = SDL_CreateGPUGraphicsPipeline(ctx.device, &info);
		// PERDU_ASSERT(_pipeline, "failed to create pipeline");
		//
		// PERDU_LOG_INFO("pipeline created");
	}

	void Pipeline::create_graphics_pipeline(const vk::Format* swapchainformat) {
		std::vector<vk::DynamicState> dynstates
		  = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dyninfo{
			.dynamicStateCount = static_cast<uint32_t>(dynstates.size()),
			.pDynamicStates	   = dynstates.data()
		};

		vk::PipelineViewportStateCreateInfo viewportstate{ .viewportCount = 1,
														   .scissorCount  = 1 };

		struct LayoutBindingInfo
		{
			vk::DescriptorType	 type;
			uint32_t			 count;
			vk::ShaderStageFlags stages;
		};

		ShaderHandle												 vert;
		std::vector<std::unordered_map<uint32_t, LayoutBindingInfo>> sets;
		uint32_t													 maxset = 0;

		for (auto& shader : _shaders) {
			if (shader->cpu.stage == ShaderStage::Vertex) { vert = shader; }

			auto stage = to_vkshaderstage(shader->cpu.stage);

			for (auto& r : shader->cpu.bindings) {
				if (sets.size() <= r.set) sets.resize(r.set + 1);
				auto& binding = sets[r.set][r.binding];
				maxset		  = std::max(maxset, r.set);

				binding.type	= to_vkdescriptortype(r.type);
				binding.count	= r.count;
				binding.stages |= stage;
			}
		}

		std::vector<vk::DescriptorSetLayout> descs;

		for (auto& set : sets) {
			std::vector<vk::DescriptorSetLayoutBinding> bindings;

			for (auto& [idx, info] : set) {
				bindings.push_back({ .binding		  = idx,
									 .descriptorType  = info.type,
									 .descriptorCount = info.count,
									 .stageFlags	  = info.stages });
			}

			vk::DescriptorSetLayoutCreateInfo descinfo{
				.bindingCount = static_cast<uint32_t>(bindings.size()),
				.pBindings	  = bindings.data()
			};

			_descriptors.emplace_back(_ctx->device, descinfo);
			descs.push_back(*_descriptors.back());
		}

		PERDU_ASSERT(vert.valid(), "could not find vertex shader");

		vk::VertexInputBindingDescription binding{
			.binding   = 0,
			.stride	   = vert->cpu.vertex_strides,
			.inputRate = vk::VertexInputRate::eVertex
		};

		std::vector<vk::VertexInputAttributeDescription> attrs{};
		for (auto& attr : vert->cpu.attributes) {
			attrs.push_back({ .location = attr.location,
							  .binding	= 0,
							  .format	= to_vkformat(attr.format),
							  .offset	= attr.offset });
		}


		vk::PipelineVertexInputStateCreateInfo vertinputinfo{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions	   = &binding,
			.vertexAttributeDescriptionCount
			= static_cast<uint32_t>(attrs.size()),
			.pVertexAttributeDescriptions = attrs.data()
		};

		vk::PipelineInputAssemblyStateCreateInfo inputassembly{
			.topology = vk::PrimitiveTopology::eTriangleList
		};

		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable		 = false,
			.rasterizerDiscardEnable = false,
			.polygonMode			 = vk::PolygonMode::eFill,
			.cullMode				 = vk::CullModeFlagBits::eNone,
			.frontFace				 = vk::FrontFace::eClockwise,
			.depthBiasEnable		 = false,
			.lineWidth				 = 1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable  = false
		};

		vk::PipelineColorBlendAttachmentState colorblendatt{
			.blendEnable	= false,
			.colorWriteMask = vk::ColorComponentFlagBits::eR
							| vk::ColorComponentFlagBits::eG
							| vk::ColorComponentFlagBits::eB
							| vk::ColorComponentFlagBits::eA
		};

		vk::PipelineColorBlendStateCreateInfo colorblendinfo{
			.logicOpEnable	 = false,
			.logicOp		 = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments	 = &colorblendatt
		};

		vk::PipelineLayoutCreateInfo layoutinfo{
			.setLayoutCount			= static_cast<uint32_t>(descs.size()),
			.pSetLayouts			= descs.data(),
			.pushConstantRangeCount = 0
		};

		_layout = vk::raii::PipelineLayout(_ctx->device, layoutinfo);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo,
						   vk::PipelineRenderingCreateInfo>
		  pipelineinfochain = {
			  { .stageCount = static_cast<uint32_t>(_shaderstages.size()),
				.pStages	= _shaderstages.data(),
				.pVertexInputState	 = &vertinputinfo,
				.pInputAssemblyState = &inputassembly,
				.pViewportState		 = &viewportstate,
				.pRasterizationState = &rasterizer,
				.pMultisampleState	 = &multisampling,
				.pColorBlendState	 = &colorblendinfo,
				.pDynamicState		 = &dyninfo,
				.layout				 = *_layout,
				.renderPass			 = nullptr },
			  { .colorAttachmentCount	 = 1,
				.pColorAttachmentFormats = swapchainformat }
		  };

		_pipeline = vk::raii::Pipeline(
		  _ctx->device,
		  nullptr,
		  pipelineinfochain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void Pipeline::create_compute_pipeline() {};

	// Pipeline::~Pipeline() {
	// 	if (_pipeline) SDL_ReleaseGPUGraphicsPipeline(_ctx.device, _pipeline);
	// }
	//
	// void Pipeline::bind(SDL_GPURenderPass* pass) const {
	// 	PERDU_ASSERT(_pipeline, "trying to bind to invalid pipeline");
	// 	SDL_BindGPUGraphicsPipeline(pass, _pipeline);
	// }
	//
	// Pipeline::Pipeline(Pipeline&& other) noexcept :
	// 	_ctx(other._ctx), _pipeline(other._pipeline) {
	// 	other._pipeline = nullptr;
	// }
	//
	// Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
	// 	if (this == &other) return *this;
	// 	if (_pipeline) SDL_ReleaseGPUGraphicsPipeline(_ctx.device, _pipeline);
	// 	_pipeline		= other._pipeline;
	// 	other._pipeline = nullptr;
	// 	return *this;
	// }

}
