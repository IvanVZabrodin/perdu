#include "renderer/shader.hpp"

#include "perdu/renderer/shader.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <vector>

namespace perdu {

	std::vector<uint8_t> load_spirv(std::string path) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		PERDU_ASSERT(file.is_open(), "failed to open shader file");
		size_t				 size = file.tellg();
		std::vector<uint8_t> data(size);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(data.data()), size);
		return data;
	}

	SDL_GPUShader* load_sdlshader(SDL_GPUDevice*	 device,
								  std::string		 path,
								  SDL_GPUShaderStage stage,
								  uint32_t			 uniform_buffers,
								  uint32_t			 storage_buffers,
								  uint32_t			 samplers) {
		auto code = load_spirv(path);
		return load_sdlshader_from_code(
		  device, code, stage, uniform_buffers, storage_buffers, samplers);
	}

	SDL_GPUShader* load_sdlshader_from_code(SDL_GPUDevice*		 device,
											std::vector<uint8_t> code,
											SDL_GPUShaderStage	 stage,
											uint32_t uniform_buffers,
											uint32_t storage_buffers,
											uint32_t samplers) {
		SDL_GPUShaderCreateInfo info{
			.code_size			 = code.size(),
			.code				 = code.data(),
			.entrypoint			 = "main",
			.format				 = SDL_GPU_SHADERFORMAT_SPIRV,
			.stage				 = stage,
			.num_samplers		 = samplers,
			.num_storage_buffers = storage_buffers,
			.num_uniform_buffers = uniform_buffers,
		};

		SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
		PERDU_ASSERT(shader, "failed to create shader");
		return shader;
	}

	GPUShader::GPUShader(GPUContext& ctx, const CPUShader& cpu) : _ctx(ctx) {
		_shader = load_sdlshader_from_code(_ctx.device,
										   cpu.spirv,
										   cpu.stage == ShaderStage::Vertex
											 ? SDL_GPU_SHADERSTAGE_VERTEX
											 : SDL_GPU_SHADERSTAGE_FRAGMENT,
										   cpu.uniform_buffers,
										   cpu.storage_buffers,
										   cpu.samplers);
	}

	GPUShader::~GPUShader() {
		if (_shader) SDL_ReleaseGPUShader(_ctx.device, _shader);
	}

	CPUShader::CPUShader(std::string __path, ShaderStage __stage) :
		stage(__stage) {
		spirv = load_spirv(__path.data());
		reflect(*this);
	}

	CPUShader::CPUShader(std::vector<uint8_t> __code, ShaderStage __stage) :
		spirv(std::move(__code)), stage(__stage) {
		reflect(*this);
	}

	CPUShader::CPUShader(std::vector<uint8_t>		  __code,
						 ShaderStage				  __stage,
						 std::vector<VertexAttribute> __attributes,
						 uint32_t					  __strides,
						 uint32_t					  __ubufs,
						 uint32_t					  __sbufs,
						 uint32_t					  __samplers) :
		spirv(std::move(__code)),
		stage(__stage),
		attributes(__attributes),
		vertex_strides(__strides),
		uniform_buffers(__ubufs),
		storage_buffers(__sbufs),
		samplers(__samplers) {}

}
