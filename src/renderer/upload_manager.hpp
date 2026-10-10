#pragma once

#include "renderer/gpu_context.hpp"

#include <cstdint>
#include <vector>
namespace perdu {
	class UploadManager {
	  public:
		UploadManager(GPUContext* ctx, uint32_t size = 16 * 1 << 20) :
			_srb(ctx, size) {};

		void begin_frame(uint32_t frame) {
			_srb.begin_frame(frame);
			_pending.clear();
		}

		bool queue_write(const void* src,
						 uint32_t	 size,
						 Buffer&	 dst,
						 uint32_t	 offset);

		void flush(vk::CommandBuffer cmd);

	  private:
		struct PendingCopy
		{
			VkBuffer	 dst;
			VkBufferCopy region;
		};

		SRB						 _srb;
		std::vector<PendingCopy> _pending;
	};
}
