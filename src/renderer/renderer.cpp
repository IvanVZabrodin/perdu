#include "perdu/renderer/renderer.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"
#include "renderer/gpu_context.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/shader.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

template <size_t A, size_t B>
void debug_read_matrix(const std::array<float, A * B>& arr) {
	for (size_t y = 0; y < B; ++y) {
		std::cout << "[" << y << "] ";
		for (size_t x = 0; x < A; ++x) { std::cout << arr[A * y + x] << " "; }
		std::cout << "\n";
	}
}

namespace perdu {
	Renderer::Renderer(GPUContext* ctx, Scene& scene, WinContext* wtx) :
		_ctx(ctx), _wtx(wtx), _scene(scene) {}

	Renderer::~Renderer() {};

	void Renderer::set_wtx(WinContext* wtx) {
		_wtx = wtx;
		if (!_cmdpool)
			_cmdpool = std::make_unique<CommandPool>(_ctx, max_flight_frames);

		_swp	 = std::make_unique<Swapchain>(_wtx, _cmdpool.get());
		_vertbuf = std::make_unique<Buffer>(
		  create_staging_buffer(_ctx,
								_verts.size() * sizeof(Vertex),
								false,
								vk::BufferUsageFlagBits::eVertexBuffer));
		_vertstage = std::make_unique<Buffer>(
		  create_staging_buffer(_ctx, _verts.size() * sizeof(Vertex), true));
		_vertstage->write(_verts.data(), _verts.size() * sizeof(Vertex));
		copy_buffer(*_cmdpool, *_vertstage, *_vertbuf, _vertstage->size);

		_indbuf = std::make_unique<Buffer>(
		  create_staging_buffer(_ctx,
								_inds.size() * sizeof(uint16_t),
								false,
								vk::BufferUsageFlagBits::eIndexBuffer));
		_indstage = std::make_unique<Buffer>(
		  create_staging_buffer(_ctx, _inds.size() * sizeof(uint16_t), true));
		_indstage->write(_inds.data(), _inds.size() * sizeof(uint16_t));
		copy_buffer(*_cmdpool, *_indstage, *_indbuf, _indstage->size);

		for (size_t i = 0; i < _swp->images.size(); ++i) {
			_rendersems.push_back(std::make_unique<Semaphore>(_ctx));
		}

		for (size_t i = 0; i < max_flight_frames; ++i) {
			_presentsems.push_back(std::make_unique<Semaphore>(_ctx));
			_fences.push_back(std::make_unique<Fence>(_ctx));
			_ubos.push_back(std::make_unique<UniformBuffer>(_ctx, sizeof(UBO)));
		}
	}

	void Renderer::make_pipeline() {
		_testpipe = std::make_unique<Pipeline>(
		  _ctx, PipelineType::Graphics, std::vector{ vert, frag }, _swp.get());

		_descpool = std::make_unique<DescriptorPool>(
		  _ctx,
		  std::vector<vk::DescriptorSetLayout>(
			max_flight_frames, *_testpipe->get_descriptors()[0]));

		for (size_t i = 0; i < max_flight_frames; ++i) {

			vk::DescriptorBufferInfo bufinfo{ .buffer = *_ubos[i]->buffer,
											  .offset = 0,
											  .range  = sizeof(UBO) };
			vk::WriteDescriptorSet	 descwrite{
				.dstSet			 = *_descpool->sets[i],
				.dstBinding		 = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType	 = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo	 = &bufinfo
			};

			_ctx->device.updateDescriptorSets(descwrite, {});
		}
	}

	void Renderer::recreate_swp() {
		_ctx->device.waitIdle();

		_swp.reset();
		_swp = std::make_unique<Swapchain>(_wtx, _cmdpool.get());
	}

	void Renderer::rendertest(uint32_t image) {
		auto& cmd = _cmdpool->buffers[_frameidx];
		cmd.reset();
		cmd.begin({});

		_swp->transition_layout(
		  _frameidx,
		  image,
		  vk::ImageLayout::eUndefined,
		  vk::ImageLayout::eColorAttachmentOptimal,
		  {},
		  vk::AccessFlagBits2::eColorAttachmentWrite,
		  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		  vk::PipelineStageFlagBits2::eColorAttachmentOutput);

		vk::ClearValue clearcol = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attinfo
		  = { .imageView   = *_swp->views[image],
			  .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			  .loadOp	   = vk::AttachmentLoadOp::eClear,
			  .storeOp	   = vk::AttachmentStoreOp::eStore,
			  .clearValue  = clearcol };

		vk::RenderingInfo rendinfo = {
			.renderArea = { .offset = { 0, 0 }, .extent = _swp->extent },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments	  = &attinfo
		};

		cmd.beginRendering(rendinfo);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_testpipe->get());
		cmd.setViewport(0,
						vk::Viewport(0.0f,
									 0.0f,
									 static_cast<float>(_swp->extent.width),
									 static_cast<float>(_swp->extent.height),
									 0.0f,
									 1.0f));
		cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swp->extent));

		cmd.bindVertexBuffers(0, ***_vertbuf, { 0 });
		cmd.bindIndexBuffer(***_indbuf, 0, vk::IndexType::eUint16);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
							   *_testpipe->get_layout(),
							   0,
							   *_descpool->sets[_frameidx],
							   nullptr);
		cmd.drawIndexed(static_cast<uint32_t>(_inds.size()), 1, 0, 0, 0);

		cmd.endRendering();

		_swp->transition_layout(
		  _frameidx,
		  image,
		  vk::ImageLayout::eColorAttachmentOptimal,
		  vk::ImageLayout::ePresentSrcKHR,
		  vk::AccessFlagBits2::eColorAttachmentWrite,
		  {},
		  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		  vk::PipelineStageFlagBits2::eBottomOfPipe);

		cmd.end();
	}

	void Renderer::draw() {
		if ((SDL_GetWindowFlags(_wtx->window) & SDL_WINDOW_MINIMIZED) != 0)
			return;

		auto fenceres = _ctx->device.waitForFences(
		  *_fences[_frameidx]->fence, true, UINT64_MAX);
		PERDU_ASSERT(fenceres == vk::Result::eSuccess,
					 "failed to wait for fence?");
		auto [result, image] = _swp->swp.acquireNextImage(
		  UINT64_MAX, *_presentsems[_frameidx]->semaphore, nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR) {
			PERDU_LOG_DEBUG("out of date swap khr");
			recreate_swp();
			return;
		}

		PERDU_ASSERT(result == vk::Result::eSuccess
					   || result == vk::Result::eSuboptimalKHR,
					 "failed to acquire image");

		update_ubo(_frameidx);
		_ctx->device.resetFences(*_fences[_frameidx]->fence);

		rendertest(image);

		vk::PipelineStageFlags waitstagemask(
		  vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitinfo{
			.waitSemaphoreCount	  = 1,
			.pWaitSemaphores	  = &*(_presentsems[_frameidx]->semaphore),
			.pWaitDstStageMask	  = &waitstagemask,
			.commandBufferCount	  = 1,
			.pCommandBuffers	  = &*(_cmdpool->buffers[_frameidx]),
			.signalSemaphoreCount = 1,
			.pSignalSemaphores	  = &*(_rendersems[image]->semaphore)
		};

		_ctx->gcq.submit(submitinfo, *(_fences[_frameidx]->fence));

		const vk::PresentInfoKHR presentinfo{ .waitSemaphoreCount = 1,
											  .pWaitSemaphores	  = &*(
												_rendersems[image]->semaphore),
											  .swapchainCount = 1,
											  .pSwapchains	  = &*(_swp->swp),
											  .pImageIndices  = &image };

		result = _ctx->gcq.presentKHR(presentinfo);

		if ((result == vk::Result::eSuboptimalKHR)
			|| (result == vk::Result::eErrorOutOfDateKHR))
		{
			PERDU_LOG_DEBUG("out of date or suboptimal swap khr");
			recreate_swp();
		} else
			PERDU_ASSERT(result == vk::Result::eSuccess,
						 "failed to present khr");

		_frameidx = (_frameidx + 1) % max_flight_frames;
	}

	void Renderer::update_ubo(uint32_t image) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto  currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(
					   currentTime - startTime)
					   .count();

		UBO	 ubo{};
		auto mod = build_rotation_matrix(
		  3,
		  Vectorf{ pmod(time * (std::numbers::pi_v<float> / 2),
						std::numbers::pi_v<float> * 2),
				   0.0f,
				   0.0f });
		std::copy(mod.begin(), mod.begin() + 3, ubo.model.begin());
		std::copy(mod.begin() + 3, mod.begin() + 6, ubo.model.begin() + 4);
		std::copy(mod.begin() + 6, mod.begin() + 9, ubo.model.begin() + 8);
		ubo.model.back() = 1;

		ubo.view = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
					 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

		ubo.proj = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
					 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

		_ubos[image]->write(&ubo, sizeof(ubo));
	}
}
