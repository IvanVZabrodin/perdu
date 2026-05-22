#include "perdu/renderer/renderer.hpp"

#include "perdu/core/log.hpp"
#include "renderer/gpu_context.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/shader.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <memory>

namespace perdu {
	Renderer::Renderer(GPUContext* ctx, Scene& scene, WinContext* wtx) :
		_ctx(ctx), _wtx(wtx), _scene(scene) {
		// _cmdpool = std::make_unique<CommandPool>(_ctx, 1);
		// if (wtx) _swp = std::make_unique<Swapchain>(_wtx, _cmdpool.get());
	}

	Renderer::~Renderer() {};

	void Renderer::set_wtx(WinContext* wtx) {
		_wtx = wtx;
		if (!_cmdpool) _cmdpool = std::make_unique<CommandPool>(_ctx, 1);
		_drawfence	= std::make_unique<Fence>(_ctx);
		_rendersem	= std::make_unique<Semaphore>(_ctx);
		_presentsem = std::make_unique<Semaphore>(_ctx);

		_swp = std::make_unique<Swapchain>(_wtx, _cmdpool.get());
	}

	void Renderer::make_pipeline() {
		_testpipe = std::make_unique<Pipeline>(
		  _ctx, PipelineType::Graphics, std::vector{ vert, frag }, _swp.get());
	}

	void Renderer::rendertest(uint32_t image) {
		auto& cmd = _cmdpool->buffers[0];
		cmd.begin({});

		_swp->transition_layout(
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

		cmd.draw(3, 1, 0, 0);

		cmd.endRendering();

		_swp->transition_layout(
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
		auto fenceres
		  = _ctx->device.waitForFences(*_drawfence->fence, true, UINT64_MAX);
		_ctx->device.resetFences(*_drawfence->fence);
		auto [result, image] = _swp->swp.acquireNextImage(
		  UINT64_MAX, *_presentsem->semaphore, nullptr);

		rendertest(image);

		vk::PipelineStageFlags waitstagemask(
		  vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo submitinfo{
			.waitSemaphoreCount	  = 1,
			.pWaitSemaphores	  = &*(_presentsem->semaphore),
			.pWaitDstStageMask	  = &waitstagemask,
			.commandBufferCount	  = 1,
			.pCommandBuffers	  = &*(_cmdpool->buffers[0]),
			.signalSemaphoreCount = 1,
			.pSignalSemaphores	  = &*(_rendersem->semaphore)
		};

		_ctx->gcq.submit(submitinfo, *(_drawfence->fence));

		const vk::PresentInfoKHR presentinfo{ .waitSemaphoreCount = 1,
											  .pWaitSemaphores
											  = &*(_rendersem->semaphore),
											  .swapchainCount = 1,
											  .pSwapchains	  = &*(_swp->swp),
											  .pImageIndices  = &image };

		_ctx->gcq.presentKHR(presentinfo);
	}
}
