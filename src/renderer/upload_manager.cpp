#include "renderer/upload_manager.hpp"

#include "perdu/core/log.hpp"
#include "renderer/gpu_context.hpp"

#include <cstdint>

namespace perdu {
	bool UploadManager::queue_write(const void* src,
									uint32_t	size,
									Buffer&		dst,
									uint32_t	offset) {
		SRB::Slice s = _srb.allocate(size);
		if (!s.ptr) {
			PERDU_LOG_WARN("SRB out of space.");
			return false;
		}

		_srb.write(src, size, s.offset);
		_pending.push_back({
			*dst, { s.offset, offset, size }
		 });
		return true;
	}

	void UploadManager::flush(vk::CommandBuffer cmd) {
		if (_pending.empty()) return;
		_srb.flush_if_needed(_srb.head, _srb.frame * _srb.region_size);

		vk::MemoryBarrier2 barrier{
			.sType		   = vk::StructureType::eMemoryBarrier2,
			.srcStageMask  = vk::PipelineStageFlagBits2::eCopy,
			.srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
			.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
			.dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead
		};
		vk::DependencyInfo dep{ .sType = vk::StructureType::eDependencyInfo,
								.memoryBarrierCount = 1,
								.pMemoryBarriers	= &barrier };
		cmd.pipelineBarrier2(&dep);
	}
}
