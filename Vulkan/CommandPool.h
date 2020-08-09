#ifndef __CPU_NW_VULKAN_COMMANDPOOL_H
#define __CPU_NW_VULKAN_COMMANDPOOL_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>
#include <memory>

#include "Device.h"
#include "Supply.h"

namespace Vulkan
{
  class CommandPool
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<std::pair<VkCommandBufferLevel, VkCommandBuffer>> command_buffers;
    uint32_t family_queue_index = 0;
    void Destroy();
  public:
    CommandPool() = delete;
    CommandPool(const CommandPool &obj) = delete;
    CommandPool(std::shared_ptr<Vulkan::Device> dev, const uint32_t family_queue_index);
    CommandPool& operator= (const CommandPool &obj) = delete;
    const VkCommandBuffer& operator[] (const uint32_t index) const { return command_buffers[index].second; }
    VkCommandPool GetCommandPool() const { return command_pool; }
    size_t GetCommandBuffersCount() const { return command_buffers.size(); }
    void BeginCommandBuffer(const uint32_t index, const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void EndCommandBuffer(const uint32_t index);
    void BindPipeline(const uint32_t index, const VkPipeline pipeline, const VkPipelineBindPoint bind_point);
    void BeginRenderPass(const uint32_t index, const VkRenderPass render_pass, const VkFramebuffer frame_buffer, const VkExtent2D extent, const VkOffset2D offset = {0, 0});
    void EndRenderPass(const uint32_t index);
    void BindVertexBuffers(const uint32_t index, const std::vector<VkBuffer> buffers, const std::vector<VkDeviceSize> offsets, const uint32_t first_binding, const uint32_t binding_count);
    void BindIndexBuffer(const uint32_t index, const VkBuffer buffer, const VkIndexType index_type, const VkDeviceSize offset = 0);
    void BindDescriptorSets(const uint32_t index, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const std::vector<uint32_t> dynamic_offeset, const uint32_t first_set = 0);
    void DrawIndexed(const uint32_t index, const uint32_t index_count, const uint32_t first_index, const uint32_t vertex_offset = 0, const uint32_t instance_count = 1, const uint32_t first_instance = 0);
    void Draw(const uint32_t index, const uint32_t vertex_count, const uint32_t first_vertex = 0, const uint32_t instance_count = 1, const uint32_t first_instance = 0);
    void Dispatch(const uint32_t index, const uint32_t x, const uint32_t y, const uint32_t z);
    ~CommandPool();
  };
}
#endif