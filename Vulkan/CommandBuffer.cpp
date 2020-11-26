#include "CommandBuffer.h"
#include "Logger.h"

namespace Vulkan
{
  CommandBuffer_impl::~CommandBuffer_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (exec_fence != VK_NULL_HANDLE)
      vkDestroyFence(device->GetDevice(), exec_fence, nullptr);
    
    if (buffer != VK_NULL_HANDLE)
      vkFreeCommandBuffers(device->GetDevice(), pool, 1, &buffer);
  }

  CommandBuffer_impl::CommandBuffer_impl(const std::shared_ptr<Device> dev, const VkCommandPool pool, const VkCommandBufferLevel level)
  {
    if (dev.get() == nullptr || !dev->IsValid() || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    if (pool == VK_NULL_HANDLE)
    {
      Logger::EchoError("Pool is empty", __func__);
      return;
    }

    device = dev;
    this->pool = pool;
    this->level = level;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;

    auto er = vkCreateFence(device->GetDevice(), &fence_info, nullptr, &exec_fence);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to create fence", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return;
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = pool;
    command_buffer_allocate_info.level = level;
    command_buffer_allocate_info.commandBufferCount = 1;

    er = vkAllocateCommandBuffers(device->GetDevice(), &command_buffer_allocate_info, &buffer);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't allocate command buffers", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return;
    }
  }

  void CommandBuffer_impl::SetMemoryBarrier(const std::vector<VkBufferMemoryBarrier> buffer_barriers,
                        const std::vector<VkMemoryBarrier> memory_barriers,
                        const std::vector<VkImageMemoryBarrier> image_bariers,
                        const VkPipelineStageFlags src_tage_flags, 
                        const VkPipelineStageFlags dst_tage_flags) noexcept
  {
    vkCmdPipelineBarrier(buffer, src_tage_flags, dst_tage_flags, 0,
                        (uint32_t) memory_barriers.size(), memory_barriers.size() > 0 ? memory_barriers.data() : nullptr, 
                        (uint32_t) buffer_barriers.size(), buffer_barriers.size() > 0 ? buffer_barriers.data() : nullptr, 
                        (uint32_t) image_bariers.size(), image_bariers.size() > 0 ? image_bariers.data() : nullptr);
  }

  void CommandBuffer_impl::BeginCommandBuffer()
  {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (state != BufferState::NotReady) return;

    if (auto er = vkBeginCommandBuffer(buffer, &begin_info); er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to begin recording command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      state = BufferState::Error;
    }
    else
    {
      state = BufferState::OnWrite;
    }
  }

  void CommandBuffer_impl::EndCommandBuffer()
  {
    if (state != BufferState::OnWrite) return;

    if (auto er = vkEndCommandBuffer(buffer); er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to record command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      state = BufferState::Error;
    }
    else
    {
      state = BufferState::Ready;
    }
  }

  void CommandBuffer_impl::ResetCommandBuffer()
  {
    if (on_execute && state != BufferState::Error) return;

    if (auto er = vkResetCommandBuffer(buffer, 0); er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to reset command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      state = BufferState::Error;
    }
    else
    {
      state = BufferState::NotReady;
      on_execute = false;
    }
  }

  VkResult CommandBuffer_impl::ExecuteBuffer(const uint32_t family_queue_index, const std::vector<VkSemaphore> signal_semaphores, const std::vector<VkPipelineStageFlags> wait_dst_stages, const std::vector<VkSemaphore> wait_semaphores)
  {
    if (state != BufferState::Ready) return VK_NOT_READY;

    if (wait_dst_stages.size() != wait_semaphores.size())
    {
      Logger::EchoError("wait_dst_stages.size() != wait_semaphores.size()", __func__);
      state = BufferState::Error;
      return VK_ERROR_UNKNOWN;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;

    submit_info.waitSemaphoreCount = (uint32_t) wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.size() > 0 ? wait_semaphores.data() : nullptr;
    submit_info.pWaitDstStageMask = wait_semaphores.size() > 0 ? wait_dst_stages.data() : nullptr;
    submit_info.signalSemaphoreCount = (uint32_t) signal_semaphores.size();
    submit_info.pSignalSemaphores = signal_semaphores.size() > 0 ? signal_semaphores.data() : nullptr; 

    if (auto er = vkResetFences(device->GetDevice(), 1, &exec_fence); er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to reset fence", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      state = BufferState::Error;
      return VK_ERROR_UNKNOWN;
    }

    if (auto er = vkQueueSubmit(device->GetQueueFormFamilyIndex(family_queue_index), 1, &submit_info, exec_fence); er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to submit buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      state = BufferState::Error;
      return VK_ERROR_UNKNOWN;
    }

    on_execute = true;

    return VK_SUCCESS;
  }

  VkResult CommandBuffer_impl::WaitForExecute(const uint64_t timeout)
  {
    if (state != BufferState::Ready) return VK_NOT_READY;

    if (!on_execute) return VK_SUCCESS;
    
    if (auto er = vkWaitForFences(device->GetDevice(), 1, &exec_fence, VK_TRUE, timeout); er != VK_SUCCESS)
    {
      if (er == VK_TIMEOUT)
      {
        Logger::EchoError("Fence timeout", __func__);
      }
      else
      {
        Logger::EchoError("Failed to submit buffer", __func__);
        state = BufferState::Error;
      }
      return er;
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    on_execute = false;
    return VK_SUCCESS;
  }

  void CommandBuffer_impl::BeginRenderPass(const std::shared_ptr<Vulkan::RenderPass> render_pass, const uint32_t frame_buffer_index, const VkOffset2D offset)
  {
    if (state != BufferState::OnWrite) return;

    if (render_pass.get() == nullptr || !render_pass->IsValid() || render_pass->GetRenderPass() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Invalid render pass", __func__);
      return;
    }

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass->GetRenderPass();    
    render_pass_info.renderArea.offset = offset;
    render_pass_info.renderArea.extent = render_pass->GetExtent();

    auto clear_colors = render_pass->GetClearColors();

    render_pass_info.clearValueCount = (uint32_t) clear_colors.size();
    render_pass_info.pClearValues = clear_colors.data();
    render_pass_info.framebuffer = render_pass->GetFrameBuffers()[frame_buffer_index];

    VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
      contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdBeginRenderPass(buffer, &render_pass_info, contents);
  }

  void CommandBuffer_impl::EndRenderPass() noexcept
  {
    if (state != BufferState::OnWrite) return;

    vkCmdEndRenderPass(buffer);
  }

  void CommandBuffer_impl::NextSubpass()
  {
    if (state != BufferState::OnWrite) return;

    VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) 
      contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdNextSubpass(buffer, contents);
  }

  void CommandBuffer_impl::DrawIndexed(const uint32_t index_count, const uint32_t first_index, const uint32_t vertex_offset, const uint32_t instance_count, const uint32_t first_instance) noexcept
  {
    if (state != BufferState::OnWrite) return;

    vkCmdDrawIndexed(buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void CommandBuffer_impl::Draw(const uint32_t vertex_count, const uint32_t first_vertex, const uint32_t instance_count, const uint32_t first_instance) noexcept
  {
    if (state != BufferState::OnWrite) return;

    vkCmdDraw(buffer, vertex_count, instance_count, first_vertex, first_instance);
  }

  void CommandBuffer_impl::Dispatch(const uint32_t x, const uint32_t y, const uint32_t z) noexcept
  {
    if (state != BufferState::OnWrite) return;

    vkCmdDispatch(buffer, x, y, z);
  }

  void CommandBuffer_impl::BindPipeline(const VkPipeline pipeline, const VkPipelineBindPoint bind_point) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (pipeline == VK_NULL_HANDLE)
    {
      Logger::EchoError("Invalid pipeline", __func__);
      return;
    }

    vkCmdBindPipeline(buffer, bind_point, pipeline);
  }

  void CommandBuffer_impl::BindDescriptorSets(const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const uint32_t first_set, const std::vector<uint32_t> dynamic_offeset) noexcept
  {
    if (state != BufferState::OnWrite) return;
    
    vkCmdBindDescriptorSets(buffer, bind_point, pipeline_layout, 
                            first_set, (uint32_t) sets.size(), 
                            sets.data(), (uint32_t) dynamic_offeset.size(), 
                            dynamic_offeset.empty() ? nullptr : dynamic_offeset.data());
  }

  void CommandBuffer_impl::BindVertexBuffers(const std::vector<VkBuffer> buffers, const std::vector<VkDeviceSize> offsets, const uint32_t first_binding, const uint32_t binding_count) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (buffers.empty() || offsets.empty())
    {
      Logger::EchoError("Vertex buffers are empty", __func__);
      return;
    }

    vkCmdBindVertexBuffers(buffer, first_binding, binding_count, buffers.data(), offsets.data());
  }

  void CommandBuffer_impl::BindIndexBuffer(const VkBuffer buffer, const VkIndexType index_type, const VkDeviceSize offset) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is empty", __func__);
      return;
    }

    vkCmdBindIndexBuffer(this->buffer, buffer, offset, index_type);
  }

  void CommandBuffer_impl::SetViewport(const std::vector<VkViewport> &viewports) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (viewports.empty())
    {
      Logger::EchoError("Viewports are empty", __func__);
      return;
    }

    vkCmdSetViewport(buffer, 0, (uint32_t) viewports.size(), viewports.data());
  }

  void CommandBuffer_impl::SetScissor(const std::vector<VkRect2D> &scissors) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (scissors.empty())
    {
      Logger::EchoError("Viewports are empty", __func__);
      return;
    }

    vkCmdSetScissor(buffer, 0, (uint32_t) scissors.size(), scissors.data());
  }

  void CommandBuffer_impl::SetDepthBias(const float depth_bias_constant_factor, const float depth_bias_clamp, const float depth_bias_slope_factor) noexcept
  {
    if (state != BufferState::OnWrite) return;

    vkCmdSetDepthBias(buffer, depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
  }

  void CommandBuffer_impl::ImageLayoutTransition(ImageArray &image, const size_t image_index, const VkImageLayout new_layout, const uint32_t mip_level, const bool transit_all_mip_levels)
  {
    if (state != BufferState::OnWrite) return;

    if (!image.IsValid() || image.Count() <= image_index)
    {
      Logger::EchoError("Image array is not valid or index is out off bounds", __func__);
      state = BufferState::Error;
      return;
    }

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = image.GetInfo(image_index).layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image.GetInfo(image_index).image;
    barrier.subresourceRange.aspectMask = image.GetInfo(image_index).aspect_flags;
    barrier.subresourceRange.baseMipLevel = transit_all_mip_levels ? 0 : mip_level;
    barrier.subresourceRange.levelCount = transit_all_mip_levels ? image.GetInfo(image_index).image_info.mipLevels : 1;
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
        Logger::EchoError("Unsupported layout transition", __func__);
        state = BufferState::Error;
        return;
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
        Logger::EchoError("Unsupported layout transition", __func__);
        state = BufferState::Error;
        return;
    }

    SetMemoryBarrier({}, {}, {barrier}, src_stage, dst_stage);
    if (image.ChangeLayout(image_index, new_layout) != VK_SUCCESS)
    {
      Logger::EchoError("Can't change layout of image", __func__);
      state = BufferState::Error;
    }
  }

  void CommandBuffer_impl::CopyBufferToImage(const VkBuffer src, ImageArray &image, const size_t image_index, const std::vector<VkBufferImageCopy> regions) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (src == VK_NULL_HANDLE)
    {
      Logger::EchoError("Invalid buffer", __func__);
      state = BufferState::Error;
      return;
    }

    if (!image.IsValid() || image.Count() <= image_index)
    {
      Logger::EchoError("Image array is not valid or index is out off bounds", __func__);
      state = BufferState::Error;
      return;
    }

    if (regions.empty())
    {
      Logger::EchoError("Image array regions is empty", __func__);
      state = BufferState::Error;
      return;
    }

    vkCmdCopyBufferToImage(buffer, src, image.GetInfo(image_index).image, image.GetInfo(image_index).layout, (uint32_t) regions.size(), regions.data());
  }

  void CommandBuffer_impl::CopyBufferToBuffer(const VkBuffer src, const VkBuffer dst, std::vector<VkBufferCopy> regions) noexcept
  {
    if (state != BufferState::OnWrite) return;

    if (src == VK_NULL_HANDLE || dst == VK_NULL_HANDLE)
    {
      Logger::EchoError("Invalid buffer", __func__);
      state = BufferState::Error;
      return;
    }

    if (regions.empty())
    {
      Logger::EchoError("Image array regions is empty", __func__);
      state = BufferState::Error;
      return;
    }

    vkCmdCopyBuffer(buffer, src, dst, (uint32_t) regions.size(), regions.data());
  }

  CommandBuffer &CommandBuffer::operator=(CommandBuffer &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void CommandBuffer::swap(CommandBuffer &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(CommandBuffer &lhs, CommandBuffer &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}