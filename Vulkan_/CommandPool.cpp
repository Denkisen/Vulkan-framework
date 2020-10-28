#include "CommandPool.h"
#include "Logger.h"

namespace Vulkan
{
  CommandPool_impl::~CommandPool_impl()
  {
    Logger::EchoDebug("", __func__);
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }
    }

    for (auto &f : command_buffers)
    {
      if (f.exec_fence != VK_NULL_HANDLE)
        vkDestroyFence(device->GetDevice(), f.exec_fence, nullptr);
    }
  }

  CommandPool_impl::CommandPool_impl(std::shared_ptr<Vulkan::Device> dev, const uint32_t family_queue_index)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = family_queue_index;

    auto er = vkCreateCommandPool(dev->GetDevice(), &command_pool_create_info, nullptr, &command_pool);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create command pool", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return;
    }

    device = dev;
    this->family_queue_index = family_queue_index;
  }

  BufferLock CommandPool_impl::BeginCommandBuffer(const VkCommandBufferLevel level)
  {
    std::lock_guard lock(locks_control_mutex);
    std::optional<uint32_t> index;

    for (uint32_t i = 0; i < command_buffers.size(); ++i)
    {
      if (command_buffers[i].reseted)
      {
        index = i;
        break;
      }
    }

    if (!index.has_value())
    {
      BufferLock buff_lock;
      buff_lock.lock = std::make_shared<std::mutex>();
      buff_lock.index = (uint32_t) command_buffers.size();

      VkFenceCreateInfo fence_info = {};
      fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_info.flags = 0;

      Commandbuffer tmp = {};
      tmp.lock = buff_lock;

      auto er = vkCreateFence(device->GetDevice(), &fence_info, nullptr, &tmp.exec_fence);
      if (er != VK_SUCCESS) 
      {
        Logger::EchoError("Failed to create fence", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        return BufferLock();
      }

      VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
      command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      command_buffer_allocate_info.commandPool = command_pool; 
      command_buffer_allocate_info.level = level;
      command_buffer_allocate_info.commandBufferCount = 1;

      er = vkAllocateCommandBuffers(device->GetDevice(), &command_buffer_allocate_info, &tmp.buffer);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't allocate command buffers", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        return BufferLock();
      }

      index = (uint32_t) command_buffers.size();
      command_buffers.push_back(tmp);
    }

    std::lock_guard lock1(*command_buffers[index.value()].lock.lock.get());

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    auto er = vkBeginCommandBuffer(command_buffers[index.value()].buffer, &begin_info);
    if (er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to begin recording command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return BufferLock();
    }

    command_buffers[index.value()].reseted = false;
    command_buffers[index.value()].started = true;
    command_buffers[index.value()].finished = false;
    command_buffers[index.value()].level = level;

    return command_buffers[index.value()].lock;
  }

  VkResult CommandPool_impl::EndCommandBuffer(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    if (!command_buffers[buffer_lock.index.value()].started || command_buffers[buffer_lock.index.value()].finished)
    {
      Logger::EchoWarning("Buffer already is finished", __func__);
      return VK_SUCCESS;
    }

    auto er = vkEndCommandBuffer(command_buffers[buffer_lock.index.value()].buffer);
    if (er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to record command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    command_buffers[buffer_lock.index.value()].finished = true;
    command_buffers[buffer_lock.index.value()].started = false;

    return VK_SUCCESS;
  }

  VkResult CommandPool_impl::ResetCommandBuffer(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    auto er = vkQueueWaitIdle(device->GetQueueFormFamilyIndex(family_queue_index));

    if (er != VK_SUCCESS) 
    {
      Logger::EchoError("Queue wait idle error", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    er = vkResetCommandBuffer(command_buffers[buffer_lock.index.value()].buffer, 0);

    if (er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to reset command buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    command_buffers[buffer_lock.index.value()].finished = false;
    command_buffers[buffer_lock.index.value()].started = false;
    command_buffers[buffer_lock.index.value()].reseted = true;

    return VK_SUCCESS;
  }

  VkResult CommandPool_impl::ExecuteBuffer(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[buffer_lock.index.value()].buffer;

    auto er = vkResetFences(device->GetDevice(), 1, &command_buffers[buffer_lock.index.value()].exec_fence);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to reset fence", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    er = vkQueueSubmit(device->GetQueueFormFamilyIndex(family_queue_index), 1, &submit_info, command_buffers[buffer_lock.index.value()].exec_fence);

    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to submit buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return er;
  }

  VkResult CommandPool_impl::WaitForExecute(const BufferLock buffer_lock)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    auto er = vkWaitForFences(device->GetDevice(), 1, &command_buffers[buffer_lock.index.value()].exec_fence, VK_TRUE, UINT64_MAX);

    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to submit buffer", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return er;
  }

  VkResult CommandPool_impl::Dispatch(const BufferLock buffer_lock, const uint32_t x, const uint32_t y, const uint32_t z)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    vkCmdDispatch(command_buffers[buffer_lock.index.value()].buffer, x, y, z);

    return VK_SUCCESS;
  }

  VkResult CommandPool_impl::BindPipeline(const BufferLock buffer_lock, const VkPipeline pipeline, const VkPipelineBindPoint bind_point)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    vkCmdBindPipeline(command_buffers[buffer_lock.index.value()].buffer, bind_point, pipeline);

    return VK_SUCCESS;
  }

  VkResult CommandPool_impl::BindDescriptorSets(const BufferLock buffer_lock, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const uint32_t first_set, const std::vector<uint32_t> dynamic_offeset)
  {
    if (!buffer_lock.index.has_value() || buffer_lock.lock.get() == nullptr)
    {
      Logger::EchoError("Buffer lock is empty", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::lock_guard lock(*buffer_lock.lock.get());

    vkCmdBindDescriptorSets(command_buffers[buffer_lock.index.value()].buffer, 
                            bind_point, pipeline_layout, 
                            first_set, (uint32_t) sets.size(), 
                            sets.data(), (uint32_t) dynamic_offeset.size(), 
                            dynamic_offeset.empty() ? nullptr : dynamic_offeset.data());
    
    return VK_SUCCESS;
  }

  CommandPool &CommandPool::operator=(CommandPool &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void CommandPool::swap(CommandPool &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(CommandPool &lhs, CommandPool &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}