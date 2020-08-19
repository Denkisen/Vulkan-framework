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

  void CommandPool::ExecuteBuffer(const uint32_t index)
  {
    VkFence move_sync = VK_NULL_HANDLE;
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[index].second;

    vkCreateFence(device->GetDevice(), &fence_info, nullptr, &move_sync);
    vkQueueSubmit(Vulkan::Supply::GetQueueFormFamilyIndex(device->GetDevice(), family_queue_index), 1, &submit_info, move_sync);
    vkWaitForFences(device->GetDevice(), 1, &move_sync, VK_TRUE, UINT64_MAX);

    if (device != VK_NULL_HANDLE && move_sync != VK_NULL_HANDLE)
      vkDestroyFence(device->GetDevice(), move_sync, nullptr);
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

  void CommandPool::BeginRenderPass(const uint32_t index, const std::shared_ptr<Vulkan::RenderPass> render_pass, const uint32_t frame_buffer_index, const VkOffset2D offset)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass->GetRenderPass();    
    render_pass_info.renderArea.offset = offset;
    render_pass_info.renderArea.extent = render_pass->GetSwapChainExtent();

    std::vector<VkClearValue> clear_colors = render_pass->GetClearColors();

    render_pass_info.clearValueCount = (uint32_t) clear_colors.size();
    render_pass_info.pClearValues = clear_colors.data();
    render_pass_info.framebuffer = render_pass->GetFrameBuffers()[frame_buffer_index];

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

  void CommandPool::CopyBuffer(const uint32_t index, const std::shared_ptr<IBuffer> src, const std::shared_ptr<IBuffer> dst, std::vector<VkBufferCopy> regions)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    if (regions.empty())
      throw std::runtime_error("It must be atleast one VkBufferCopy region.");

    if (src.get() == nullptr || dst.get() == nullptr || src->GetBuffer() == VK_NULL_HANDLE || dst->GetBuffer() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid buffer pointer");
    
    vkCmdCopyBuffer(command_buffers[index].second, src->GetBuffer(), dst->GetBuffer(), (uint32_t) regions.size(), regions.data());
  }

  void CommandPool::CopyBufferToImage(const uint32_t index, const std::shared_ptr<IBuffer> src, const std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if (dst->GetLayout() == VK_IMAGE_LAYOUT_UNDEFINED || dst->GetLayout() == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
      layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    else
      layout = dst->GetLayout();
    
    TransitionImageLayout(index, dst, layout);
    vkCmdCopyBufferToImage(command_buffers[index].second, src->GetBuffer(), dst->GetImage(), layout, (uint32_t) regions.size(), regions.data());
    TransitionImageLayout(index, dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  void CommandPool::TransitionImageLayout(const uint32_t index, const std::shared_ptr<Image> image, const VkImageLayout new_layout)
  {
    if (index >= command_buffers.size())
      throw std::runtime_error("Command buffers count is less then " + std::to_string(index));

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = image->GetLayout();
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image->GetImage();
    barrier.subresourceRange.aspectMask = image->GetImageAspectFlags();
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } 
    else if (barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (barrier.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    else if (barrier.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else 
    {
      throw std::invalid_argument("Unsupported layout transition!");
    }

    image->SetLayout(new_layout);
    vkCmdPipelineBarrier(command_buffers[index].second, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }

  void CommandPool::TransitionPipelineBarrier(const uint32_t index, const std::shared_ptr<IBuffer> buffer)
  {
    // VkBufferMemoryBarrier barrier = {};
    // barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    // barrier.pNext = nullptr;
    // barrier.buffer = buffer->GetBuffer();
    // barrier.size = buffer->BufferLength();
    // barrier.offset = 0;
    // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  }
}