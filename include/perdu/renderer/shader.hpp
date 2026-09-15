#pragma once

#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct SDL_GPUShader;


namespace perdu {
	enum class ShaderStage { Vertex, Fragment, Compute };

	struct VertexAttribute
	{
		uint32_t location;
		enum class Format { Float, Float2, Float3, Float4 } format;
		uint32_t offset;
	};

	struct DescriptorBinding
	{
		uint32_t set;
		uint32_t binding;
		enum class Type { UniformBuffer, StorageBuffer } type;
		uint32_t count;
	};

	struct CPUShader
	{
		std::vector<uint8_t>		   spirv;
		ShaderStage					   stage;
		std::vector<VertexAttribute>   attributes	  = {};
		uint32_t					   vertex_strides = 0;
		std::vector<DescriptorBinding> bindings		  = {};

		CPUShader(std::string path, ShaderStage stage);
		CPUShader(std::vector<uint8_t> code, ShaderStage stage);
		CPUShader(std::vector<uint8_t>			 code,
				  ShaderStage					 stage,
				  std::vector<VertexAttribute>	 attributes,
				  uint32_t						 strides,
				  std::vector<DescriptorBinding> bindings);
	};

	struct GPUShader;

	// class GPUShader {
	//   public:
	// 	GPUShader(GPUContext& ctx, const CPUShader& cpu);
	// 	~GPUShader();
	//
	// 	GPUShader(const GPUShader&)			   = delete;
	// 	GPUShader& operator=(const GPUShader&) = delete;
	//
	// 	SDL_GPUShader* handle() const { return _shader; }
	// 	bool		   valid() const { return _shader != nullptr; }
	//
	//   private:
	// 	GPUContext&	   _ctx;
	// 	SDL_GPUShader* _shader;
	// };

}
