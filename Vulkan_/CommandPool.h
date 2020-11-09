#ifndef __VULKAN_COMMAND_POOL_H
#define __VULKAN_COMMAND_POOL_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>
#include <optional>

#include "Device.h"
#include "CommandBuffer.h"

namespace Vulkan
{
  class CommandPool_impl
  {
  public:
    CommandPool_impl() = delete;
    CommandPool_impl(const CommandPool_impl &obj) = delete;
    CommandPool_impl(CommandPool_impl &&obj) = delete;
    CommandPool_impl &operator=(const CommandPool_impl &obj) = delete;
    CommandPool_impl &operator=(CommandPool_impl &&obj) = delete;
    ~CommandPool_impl() noexcept;
  private:
    friend class CommandPool;
    CommandPool_impl(std::shared_ptr<Device> dev, const uint32_t family_queue_index) noexcept;
    VkCommandPool GetCommandPool() const noexcept { return command_pool; }
    size_t GetCommandBuffersCount() const noexcept { return command_buffers.size(); }
    CommandBuffer &GetCommandBuffer(const uint32_t buffer_index, const VkCommandBufferLevel new_buffer_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) noexcept;
    void ResetCommandBuffer(const uint32_t buffer_index) noexcept;
    void PopLastCommandBuffer() noexcept;
    VkResult ExecuteBuffer(const uint32_t buffer_index) noexcept;
    VkResult WaitForExecute(const uint32_t buffer_index, const uint64_t timeout = UINT64_MAX) noexcept;
    bool IsError(const uint32_t buffer_index) noexcept;
    bool IsReady(const uint32_t buffer_index) noexcept;
    bool IsReset(const uint32_t buffer_index) noexcept;

    std::shared_ptr<Device> device;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    uint32_t family_queue_index = 0;
    std::vector<CommandBuffer> command_buffers;
    CommandBuffer dummy_buffer;
    std::mutex buffers_lock;
  };

  class CommandPool
  {
  private:
    std::unique_ptr<CommandPool_impl> impl;
    CommandBuffer dummy_buffer;
  public:
    CommandPool() = delete;
    CommandPool(const CommandPool &obj) = delete;
    CommandPool(CommandPool &&obj) noexcept : impl(std::move(obj.impl)) {};
    CommandPool(std::shared_ptr<Device> dev, const uint32_t family_queue_index) noexcept : 
      impl(std::unique_ptr<CommandPool_impl>(new CommandPool_impl(dev, family_queue_index))) {};
    CommandPool &operator=(const CommandPool &obj) = delete;
    CommandPool &operator=(CommandPool &&obj) noexcept;
    ~CommandPool() = default;
    void swap(CommandPool &obj) noexcept;
    bool IsValid() { return impl.get() && impl->command_pool != VK_NULL_HANDLE; }

    VkCommandPool GetCommandPool() const noexcept { if (impl.get()) return impl->GetCommandPool(); return VK_NULL_HANDLE; }
    size_t GetCommandBuffersCount() const noexcept { if (impl.get()) return impl->GetCommandBuffersCount(); return 0; }
    CommandBuffer& GetCommandBuffer(const uint32_t buffer_index, const VkCommandBufferLevel new_buffer_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) noexcept { if (impl.get()) return impl->GetCommandBuffer(buffer_index, new_buffer_level); return dummy_buffer; }
    void ResetCommandBuffer(const uint32_t buffer_index) noexcept { if (impl.get()) impl->ResetCommandBuffer(buffer_index); }
    void PopLastCommandBuffer() noexcept { if (impl.get()) impl->PopLastCommandBuffer(); }
    VkResult ExecuteBuffer(const uint32_t buffer_index) noexcept { if (impl.get()) return impl->ExecuteBuffer(buffer_index); return VK_ERROR_UNKNOWN; }
    VkResult WaitForExecute(const uint32_t buffer_index, const uint64_t timeout = UINT64_MAX) noexcept { if (impl.get()) return impl-> WaitForExecute(buffer_index, timeout); return VK_ERROR_UNKNOWN; }
    bool IsError(const uint32_t buffer_index) const noexcept { if (impl.get()) return impl->IsError(buffer_index); return true; }
    bool IsReady(const uint32_t buffer_index) const noexcept { if (impl.get()) return impl->IsReady(buffer_index); return false; }
    bool IsReset(const uint32_t buffer_index) const noexcept { if (impl.get()) return impl->IsReset(buffer_index); return true; }
  };

  void swap(CommandPool &lhs, CommandPool &rhs) noexcept;
}

#endif