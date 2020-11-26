#include "CommandPool.h"
#include "Logger.h"

namespace Vulkan
{
  CommandPool_impl::~CommandPool_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    command_buffers.clear();
    
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }
    }
  }

  CommandPool_impl::CommandPool_impl(std::shared_ptr<Device> dev, const uint32_t family_queue_index)
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

  CommandBuffer &CommandPool_impl::GetCommandBuffer(const uint32_t buffer_index, const VkCommandBufferLevel new_buffer_level)
  {
    if (buffer_index < command_buffers.size())
    {
      return command_buffers[buffer_index];
    }
    else
    {
      while (command_buffers.size() <= buffer_index)
      {
        command_buffers.push_back(CommandBuffer(device, command_pool, new_buffer_level));
      }
      return command_buffers[command_buffers.size() - 1];

      return dummy_buffer;
    }
  }

  void CommandPool_impl::ResetCommandBuffer(const uint32_t buffer_index)
  {
    if (buffer_index < command_buffers.size())
    {
      command_buffers[buffer_index].ResetCommandBuffer();
    }
  }

  void CommandPool_impl::PopLastCommandBuffer() noexcept
  {
    command_buffers.pop_back();
  }

  bool CommandPool_impl::IsError(const uint32_t buffer_index) const noexcept
  {
    if (buffer_index < command_buffers.size())
    {
      return command_buffers[buffer_index].IsError();
    }

    return true;
  }

  bool CommandPool_impl::IsReady(const uint32_t buffer_index) const noexcept
  {
    if (buffer_index < command_buffers.size())
    {
      return command_buffers[buffer_index].IsReady();
    }

    return false;
  }

  bool CommandPool_impl::IsReset(const uint32_t buffer_index) const noexcept
  {
    if (buffer_index < command_buffers.size())
    {
      return command_buffers[buffer_index].IsReset();
    }

    return true;
  }

  VkResult CommandPool_impl::ExecuteBuffer(const uint32_t buffer_index, const std::vector<VkSemaphore> signal_semaphores, const std::vector<VkPipelineStageFlags> wait_dst_stages, const std::vector<VkSemaphore> wait_semaphores)
  {
    if (command_buffers.size() <= buffer_index || !command_buffers[buffer_index].IsReady())
    {
      Logger::EchoError("Buffer is not ready", __func__);
      return VK_ERROR_UNKNOWN;
    }
    
    return command_buffers[buffer_index].ExecuteBuffer(family_queue_index, signal_semaphores, wait_dst_stages, wait_semaphores);
  }

  VkResult CommandPool_impl::WaitForExecute(const uint32_t buffer_index, const uint64_t timeout)
  {
    if (command_buffers.size() <= buffer_index || !command_buffers[buffer_index].IsReady())
    {
      Logger::EchoError("Buffer is not ready", __func__);
      return VK_ERROR_UNKNOWN;
    }

    return command_buffers[buffer_index].WaitForExecute(timeout);
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