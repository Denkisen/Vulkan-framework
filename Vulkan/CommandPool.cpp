#include "CommandPool.h"

namespace Vulkan
{
  void CommandPool::Destroy()
  {
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
        command_buffers.clear();
      }
    }
  }

  CommandPool::~CommandPool()
  {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    Destroy();
  }

  CommandPool::CommandPool(std::shared_ptr<Vulkan::Device> dev, const uint32_t family_queue_index)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");

    device = dev;
    command_pool = Vulkan::Supply::CreateCommandPool(device->GetDevice(), family_queue_index);
    this->family_queue_index = family_queue_index;
  }

  void CommandPool::BeginCommandBuffer(const uint32_t index, const VkCommandBufferLevel level)
  {
    if (command_buffers.size() <= index)
    {
      command_buffers.resize(index + 1);
    }
    else
    {
      vkQueueWaitIdle(Vulkan::Supply::GetQueueFormFamilyIndex(device->GetDevice(), family_queue_index));
      if (device->GetDevice() != VK_NULL_HANDLE && command_pool != VK_NULL_HANDLE)
      {
        if (command_buffers[index].second != VK_NULL_HANDLE)
          vkFreeCommandBuffers(device->GetDevice(), command_pool, 1, &command_buffers[index].second);
        command_buffers[index].second = VK_NULL_HANDLE;
      }
    }
    
    command_buffers[index].second = Vulkan::Supply::CreateCommandBuffers(device->GetDevice(), command_pool, 1, level)[0];
    command_buffers[index].first = level;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffers[index].second, &begin_info) != VK_SUCCESS) 
    {
      throw std::runtime_error("failed to begin recording command buffer!");
    }
  }

  void CommandPool::EndCommandBuffer(const uint32_t index)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    if (vkEndCommandBuffer(command_buffers[index].second) != VK_SUCCESS) 
    {
      throw std::runtime_error("Failed to record command buffer!");
    }
  }

  void CommandPool::BindPipeline(const uint32_t index, const VkPipeline pipeline, const VkPipelineBindPoint bind_point)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdBindPipeline(command_buffers[index].second, bind_point, pipeline);
  }

  void CommandPool::BeginRenderPass(const uint32_t index, const VkRenderPass render_pass, const VkFramebuffer frame_buffer, const VkExtent2D extent, const VkOffset2D offset)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;    
    render_pass_info.renderArea.offset = offset;
    render_pass_info.renderArea.extent = extent;

    VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;
    render_pass_info.framebuffer = frame_buffer;

    VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
    if (command_buffers[index].first == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
      contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdBeginRenderPass(command_buffers[index].second, &render_pass_info, contents);
  }

  void CommandPool::EndRenderPass(const uint32_t index)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdEndRenderPass(command_buffers[index].second);
  }

  void CommandPool::BindVertexBuffers(const uint32_t index, const std::vector<VkBuffer> buffers, const std::vector<VkDeviceSize> offsets, const uint32_t first_binding, const uint32_t binding_count)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdBindVertexBuffers(command_buffers[index].second, first_binding, binding_count, buffers.data(), offsets.data());
  }

  void CommandPool::BindIndexBuffer(const uint32_t index, const VkBuffer buffer, const VkIndexType index_type, const VkDeviceSize offset)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));
    
    vkCmdBindIndexBuffer(command_buffers[index].second, buffer, offset, index_type);
  }

  void CommandPool::BindDescriptorSets(const uint32_t index, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const std::vector<uint32_t> dynamic_offeset, const uint32_t first_set)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdBindDescriptorSets(command_buffers[index].second, 
                            bind_point, pipeline_layout, 
                            first_set, (uint32_t) sets.size(), 
                            sets.data(), (uint32_t) dynamic_offeset.size(), 
                            dynamic_offeset.empty() ? nullptr : dynamic_offeset.data());
  }

  void CommandPool::DrawIndexed(const uint32_t index, const uint32_t index_count, const uint32_t first_index, const uint32_t vertex_offset, const uint32_t instance_count, const uint32_t first_instance)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdDrawIndexed(command_buffers[index].second, index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void CommandPool::Draw(const uint32_t index, const uint32_t vertex_count, const uint32_t first_vertex, const uint32_t instance_count, const uint32_t first_instance)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdDraw(command_buffers[index].second, vertex_count, instance_count, first_vertex, first_instance);
  }

  void CommandPool::Dispatch(const uint32_t index, const uint32_t x, const uint32_t y, const uint32_t z)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    vkCmdDispatch(command_buffers[index].second, x, y, z);
  }
}