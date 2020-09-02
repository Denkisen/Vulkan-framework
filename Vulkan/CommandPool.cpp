#include "CommandPool.h"
#include "Instance.h"

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

  void CommandPool::ExecuteBuffer(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());
    
    VkFence move_sync = VK_NULL_HANDLE;
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[buffer_lock.index.value()].second;

    vkCreateFence(device->GetDevice(), &fence_info, nullptr, &move_sync);
    vkQueueSubmit(Vulkan::Supply::GetQueueFormFamilyIndex(device->GetDevice(), family_queue_index), 1, &submit_info, move_sync);
    vkWaitForFences(device->GetDevice(), 1, &move_sync, VK_TRUE, UINT64_MAX);

    if (device != VK_NULL_HANDLE && move_sync != VK_NULL_HANDLE)
      vkDestroyFence(device->GetDevice(), move_sync, nullptr);
  }

  void CommandPool::BeginCommandBuffer(const BufferLock buffer_lock, const VkCommandBufferLevel level)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    if (command_buffers.size() <= buffer_lock.index.value())
    {
      command_buffers.resize(buffer_lock.index.value() + 1);
    }
    else
    {
      vkQueueWaitIdle(Vulkan::Supply::GetQueueFormFamilyIndex(device->GetDevice(), family_queue_index));
      if (device->GetDevice() != VK_NULL_HANDLE && command_pool != VK_NULL_HANDLE)
      {
        if (command_buffers[buffer_lock.index.value()].second != VK_NULL_HANDLE)
          vkFreeCommandBuffers(device->GetDevice(), command_pool, 1, &command_buffers[buffer_lock.index.value()].second);
        command_buffers[buffer_lock.index.value()].second = VK_NULL_HANDLE;
      }
    }
    
    command_buffers[buffer_lock.index.value()].second = Vulkan::Supply::CreateCommandBuffers(device->GetDevice(), command_pool, 1, level)[0];
    command_buffers[buffer_lock.index.value()].first = level;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffers[buffer_lock.index.value()].second, &begin_info) != VK_SUCCESS) 
    {
      throw std::runtime_error("failed to begin recording command buffer!");
    }
  }

  void CommandPool::EndCommandBuffer(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    if (vkEndCommandBuffer(command_buffers[buffer_lock.index.value()].second) != VK_SUCCESS) 
    {
      throw std::runtime_error("Failed to record command buffer!");
    }
  }

  void CommandPool::BindPipeline(const BufferLock buffer_lock, const VkPipeline pipeline, const VkPipelineBindPoint bind_point)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdBindPipeline(command_buffers[buffer_lock.index.value()].second, bind_point, pipeline);
  }

  void CommandPool::BeginRenderPass(const BufferLock buffer_lock, const std::shared_ptr<Vulkan::RenderPass> render_pass, const uint32_t frame_buffer_index, const VkOffset2D offset)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

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
    if (command_buffers[buffer_lock.index.value()].first == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
      contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdBeginRenderPass(command_buffers[buffer_lock.index.value()].second, &render_pass_info, contents);
  }

  void CommandPool::EndRenderPass(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdEndRenderPass(command_buffers[buffer_lock.index.value()].second);
  }

  void CommandPool::BindVertexBuffers(const BufferLock buffer_lock, const std::vector<VkBuffer> buffers, const std::vector<VkDeviceSize> offsets, const uint32_t first_binding, const uint32_t binding_count)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdBindVertexBuffers(command_buffers[buffer_lock.index.value()].second, first_binding, binding_count, buffers.data(), offsets.data());
  }

  void CommandPool::BindIndexBuffer(const BufferLock buffer_lock, const VkBuffer buffer, const VkIndexType index_type, const VkDeviceSize offset)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdBindIndexBuffer(command_buffers[buffer_lock.index.value()].second, buffer, offset, index_type);
  }

  void CommandPool::BindDescriptorSets(const BufferLock buffer_lock, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const std::vector<uint32_t> dynamic_offeset, const uint32_t first_set)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdBindDescriptorSets(command_buffers[buffer_lock.index.value()].second, 
                            bind_point, pipeline_layout, 
                            first_set, (uint32_t) sets.size(), 
                            sets.data(), (uint32_t) dynamic_offeset.size(), 
                            dynamic_offeset.empty() ? nullptr : dynamic_offeset.data());
  }

  void CommandPool::DrawIndexed(const BufferLock buffer_lock, const uint32_t index_count, const uint32_t first_index, const uint32_t vertex_offset, const uint32_t instance_count, const uint32_t first_instance)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdDrawIndexed(command_buffers[buffer_lock.index.value()].second, index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void CommandPool::Draw(const BufferLock buffer_lock, const uint32_t vertex_count, const uint32_t first_vertex, const uint32_t instance_count, const uint32_t first_instance)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdDraw(command_buffers[buffer_lock.index.value()].second, vertex_count, instance_count, first_vertex, first_instance);
  }

  void CommandPool::Dispatch(const BufferLock buffer_lock, const uint32_t x, const uint32_t y, const uint32_t z)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdDispatch(command_buffers[buffer_lock.index.value()].second, x, y, z);
  }

  void CommandPool::CopyBuffer(const BufferLock buffer_lock, const std::shared_ptr<IBuffer> src, const std::shared_ptr<IBuffer> dst, std::vector<VkBufferCopy> regions)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    if (regions.empty())
      throw std::runtime_error("It must be atleast one VkBufferCopy region.");

    if (src.get() == nullptr || dst.get() == nullptr || src->GetBuffer() == VK_NULL_HANDLE || dst->GetBuffer() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid buffer pointer");

    CopyBuffer(buffer_lock, src->GetBuffer(), dst->GetBuffer(), regions);
  }

  void CommandPool::CopyBuffer(const BufferLock buffer_lock, const VkBuffer src, const VkBuffer dst, std::vector<VkBufferCopy> regions)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");
    
    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    if (regions.empty())
      throw std::runtime_error("It must be atleast one VkBufferCopy region.");
    
    vkCmdCopyBuffer(command_buffers[buffer_lock.index.value()].second, src, dst, (uint32_t) regions.size(), regions.data());
  }

  void CommandPool::CopyBufferToImage(const BufferLock buffer_lock, const std::shared_ptr<IBuffer> src, const std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdCopyBufferToImage(command_buffers[buffer_lock.index.value()].second, src->GetBuffer(), dst->GetImage(), dst->GetLayout(), (uint32_t) regions.size(), regions.data());
  }

  void CommandPool::TransitionImageLayout(const BufferLock buffer_lock, const std::shared_ptr<Image> image, const VkImageLayout old_layout, const VkImageLayout new_layout, const uint32_t mip_level, const bool transit_all_mip_levels)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image->GetImage();
    barrier.subresourceRange.aspectMask = image->GetImageAspectFlags();
    barrier.subresourceRange.baseMipLevel = mip_level;
    barrier.subresourceRange.levelCount = transit_all_mip_levels ? image->GetMipLevels() : 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    switch (barrier.oldLayout)
    {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        break;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
      default:
        throw std::invalid_argument("Unsupported layout transition!");
    }
    switch (barrier.newLayout)
    {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.dstAccessMask = 0;
        dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        break;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        break;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
      case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
      default:
        throw std::invalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(command_buffers[buffer_lock.index.value()].second, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }

  void CommandPool::GenerateMipLevels(const BufferLock buffer_lock, const std::shared_ptr<Image> image, const std::shared_ptr<Vulkan::Sampler> sampler)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::unique_lock<std::mutex> lock(*buffer_lock.lock.get());

    int32_t m_width = image->Width();
    int32_t m_height = image->Height();
    for (uint32_t i = 1; i < image->GetMipLevels(); i++) 
    {
      lock.unlock();
      TransitionImageLayout(buffer_lock, image, image->GetLayout(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i - 1, false);
      lock.lock();
      VkImageBlit blit = {};
      blit.srcOffsets[0] = { 0, 0, 0 };
      blit.srcOffsets[1] = { m_width, m_height, 1 };
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = i - 1;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = 1;

      blit.dstOffsets[0] = { 0, 0, 0 };
      blit.dstOffsets[1] = { m_width > 1 ? m_width / 2 : 1, m_height > 1 ? m_height / 2 : 1, 1 };
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = i;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = 1;

      vkCmdBlitImage(command_buffers[buffer_lock.index.value()].second,
                    image->GetImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image->GetImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    sampler->GetMinificationFilter());

      lock.unlock();
      TransitionImageLayout(buffer_lock, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->GetLayout(), i - 1, false);
      lock.lock();
      if (m_width > 1) m_width /= 2;
      if (m_height > 1) m_height /= 2;
    }
  }

  void CommandPool::BufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, const std::pair<VkAccessFlags, VkAccessFlags> access_flags, const std::pair<VkPipelineStageFlags, VkPipelineStageFlags> stage_flags, const std::pair<uint32_t, uint32_t> offset_size)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.buffer = buffer;
    barrier.size = offset_size.second;
    barrier.offset = offset_size.first;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = access_flags.first;
    barrier.dstAccessMask = access_flags.second;

    vkCmdPipelineBarrier(command_buffers[buffer_lock.index.value()].second, stage_flags.first, stage_flags.second, 0, 0, nullptr, 1, &barrier, 0, nullptr);
  }

  void CommandPool::CopyBufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, const uint32_t offset, const uint32_t size, const VkPipelineStageFlags dst_stage)
  {
    BufferBarrier(buffer_lock, buffer, 
                  {VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT},
                  {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, dst_stage},
                  {offset, size});
  }

  void CommandPool::HostWriteBufferBarrier(const BufferLock buffer_lock, const VkBuffer buffer, const uint32_t offset, const uint32_t size, const VkPipelineStageFlags dst_stage)
  {
    BufferBarrier(buffer_lock, buffer, 
                  {VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT},
                  {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, dst_stage},
                  {offset, size});
  }

  void CommandPool::SetViewport(const BufferLock buffer_lock, const std::vector<VkViewport> &viewports)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());

    vkCmdSetViewport(command_buffers[buffer_lock.index.value()].second, 0, (uint32_t) viewports.size(), viewports.data());
  }

  void CommandPool::SetDepthBias(const BufferLock buffer_lock, const float depth_bias_constant_factor, const float depth_bias_clamp, const float depth_bias_slope_factor)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*buffer_lock.lock.get());
    vkCmdSetDepthBias(command_buffers[buffer_lock.index.value()].second, depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
  }

  BufferLock CommandPool::OrderBufferLock()
  {
    std::lock_guard<std::mutex> lock(block_mutex);
    BufferLock result;
    result.index = blocks.size();
    blocks[blocks.size()] = std::make_shared<std::mutex>();
    result.lock = blocks[result.index.value()];
    return result;
  }

  void CommandPool::ReleaseBufferLock(BufferLock &buffer_lock)
  {
    std::lock_guard<std::mutex> lock(block_mutex);
    if (!buffer_lock.index.has_value())
      return;
    if (buffer_lock.lock.get() == nullptr)
      return;

    buffer_lock.lock->lock();
    buffer_lock.lock.reset();
    blocks.erase(buffer_lock.index.value());
    buffer_lock.index.reset();
  }
}