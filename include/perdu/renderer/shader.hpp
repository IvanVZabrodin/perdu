#pragma once

#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct SDL_GPUShader;


namespace perdu {
	enum class ShaderStage { Vertex, Fragment };

	struct VertexAttribute
	{
		uint32_t location;
		enum class Format { Float, Float2, Float3, Float4 } format;
		uint32_t offset;
	};

	struct CPUShader
	{
		std::vector<uint8_t>		 spirv;
		ShaderStage					 stage;
		std::vector<VertexAttribute> attributes		= {};
		uint32_t					 vertex_strides = 0;
		uint32_t uniform_buffers = 0, storage_buffers = 0, samplers = 0;

		CPUShader(std::string path, ShaderStage stage);
		CPUShader(std::vector<uint8_t> code, ShaderStage stage);
		CPUShader(std::vector<uint8_t>		   code,
				  ShaderStage				   stage,
				  std::vector<VertexAttribute> attributes,
				  uint32_t					   strides,
				  uint32_t					   ubufs,
				  uint32_t					   sbufs,
				  uint32_t					   samplers);
	};

	class GPUShader {
	  public:
		GPUShader(GPUContext& ctx, const CPUShader& cpu);
		~GPUShader();

		GPUShader(const GPUShader&)			   = delete;
		GPUShader& operator=(const GPUShader&) = delete;

		SDL_GPUShader* handle() const { return _shader; }
		bool		   valid() const { return _shader != nullptr; }

	  private:
		GPUContext&	   _ctx;
		SDL_GPUShader* _shader;
	};

}
