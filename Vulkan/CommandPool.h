#ifndef __CPU_NW_VULKAN_COMMANDPOOL_H
#define __CPU_NW_VULKAN_COMMANDPOOL_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include <map>

#include "Device.h"
#include "Supply.h"
#include "Buffer.h"
#include "Image.h"
#include "RenderPass.h"
#include "Sampler.h"

namespace Vulkan
{
  struct BufferLock
  {
  private:
    std::optional<uint32_t> index;
    std::shared_ptr<std::mutex> lock;
    friend class CommandPool;
  };

  class CommandPool
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<std::pair<VkCommandBufferLevel, VkCommandBuffer>> command_buffers;
    std::mutex block_mutex;
    std::map<uint32_t, std::shared_ptr<std::mutex>> blocks;
    uint32_t family_queue_index = 0;
    void Destroy();
    void BufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, 
                      const std::pair<VkAccessFlags, VkAccessFlags> access_flags, 
                      const std::pair<VkPipelineStageFlags, VkPipelineStageFlags> stage_flags, 
                      const std::pair<uint32_t, uint32_t> offset_size);
  public:
    CommandPool() = delete;
    CommandPool(const CommandPool &obj) = delete;
    CommandPool(std::shared_ptr<Vulkan::Device> dev, const uint32_t family_queue_index);
    CommandPool& operator= (const CommandPool &obj) = delete;
    const VkCommandBuffer& operator[] (const BufferLock buffer_lock) const 
    { 
      return command_buffers[buffer_lock.index.value()].second; 
    }
    VkCommandPool GetCommandPool() const { return command_pool; }
    size_t GetCommandBuffersCount() const { return command_buffers.size(); }

    void ExecuteBuffer(const BufferLock buffer_lock);
    void BeginCommandBuffer(const BufferLock buffer_lock, 
                      const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void EndCommandBuffer(const BufferLock buffer_lock);
    void BindPipeline(const BufferLock buffer_lock, const VkPipeline pipeline, 
                      const VkPipelineBindPoint bind_point);
    void BeginRenderPass(const BufferLock buffer_lock, 
                      const std::shared_ptr<Vulkan::RenderPass> render_pass, 
                      const uint32_t frame_buffer_index, const VkOffset2D offset = {0, 0});
    void EndRenderPass(const BufferLock buffer_lock);
    void BindVertexBuffers(const BufferLock buffer_lock, const std::vector<VkBuffer> buffers, 
                      const std::vector<VkDeviceSize> offsets, const uint32_t first_binding, 
                      const uint32_t binding_count);
    void BindIndexBuffer(const BufferLock buffer_lock, const VkBuffer buffer, 
                      const VkIndexType index_type, const VkDeviceSize offset = 0);
    void BindDescriptorSets(const BufferLock buffer_lock, const VkPipelineLayout pipeline_layout, 
                      const VkPipelineBindPoint bind_point, 
                      const std::vector<VkDescriptorSet> sets, 
                      const std::vector<uint32_t> dynamic_offeset, 
                      const uint32_t first_set = 0);
    void DrawIndexed(const BufferLock buffer_lock, const uint32_t index_count, 
                      const uint32_t first_index, const uint32_t vertex_offset = 0, 
                      const uint32_t instance_count = 1, const uint32_t first_instance = 0);
    void Draw(const BufferLock buffer_lock, const uint32_t vertex_count, 
                      const uint32_t first_vertex = 0, const uint32_t instance_count = 1, 
                      const uint32_t first_instance = 0);
    void Dispatch(const BufferLock buffer_lock, const uint32_t x, const uint32_t y, 
                      const uint32_t z);
    void CopyBuffer(const BufferLock buffer_lock, const std::shared_ptr<IBuffer> src, 
                      const std::shared_ptr<IBuffer> dst, std::vector<VkBufferCopy> regions);
    void CopyBuffer(const BufferLock buffer_lock, const VkBuffer src, const VkBuffer dst, 
                      std::vector<VkBufferCopy> regions);
    void CopyBufferToImage(const BufferLock buffer_lock, const std::shared_ptr<IBuffer> src, 
                      const std::shared_ptr<Image> dst, const std::vector<VkBufferImageCopy> regions);
    void TransitionImageLayout(const BufferLock buffer_lock, const std::shared_ptr<Image> image, 
                      const VkImageLayout old_layout, const VkImageLayout new_layout, const uint32_t mip_level = 0, const bool transit_all_mip_levels = true);
    void GenerateMipLevels(const BufferLock buffer_lock, const std::shared_ptr<Image> image, 
                      const std::shared_ptr<Vulkan::Sampler> sampler);
    void SetViewport(const BufferLock buffer_lock, const std::vector<VkViewport> &viewports);
    void SetDepthBias(const BufferLock buffer_lock, const float depth_bias_constant_factor, 
                      const float depth_bias_clamp, const float depth_bias_slope_factor);
    void CopyBufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, 
                      const uint32_t offset, const uint32_t size, const VkPipelineStageFlags dst_stage);
    void HostWriteBufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, 
                      const uint32_t offset, const uint32_t size, const VkPipelineStageFlags dst_stage);
    BufferLock OrderBufferLock();
    void ReleaseBufferLock(BufferLock &buffer_lock);
    ~CommandPool();
  };
}
#endif