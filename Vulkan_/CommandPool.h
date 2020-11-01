#ifndef __VULKAN_COMMAND_POOL_H
#define __VULKAN_COMMAND_POOL_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>
#include <optional>

#include "Device.h"

namespace Vulkan
{
  struct BufferLock
  {
  private:
    std::optional<uint32_t> index;
    std::shared_ptr<std::mutex> lock;
    friend class CommandPool_impl;
  };

  class CommandPool_impl
  {
  public:
    CommandPool_impl() = delete;
    CommandPool_impl(const CommandPool_impl &obj) = delete;
    CommandPool_impl(CommandPool_impl &&obj) = delete;
    CommandPool_impl &operator=(const CommandPool_impl &obj) = delete;
    CommandPool_impl &operator=(CommandPool_impl &&obj) = delete;
    ~CommandPool_impl();
  private:
    friend class CommandPool;
    CommandPool_impl(std::shared_ptr<Device> dev, const uint32_t family_queue_index);

    VkCommandPool GetCommandPool() { return command_pool; }
    size_t GetCommandBuffersCount() { return command_buffers.size(); }
    BufferLock BeginCommandBuffer(const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VkResult EndCommandBuffer(const BufferLock buffer_lock);
    VkResult ResetCommandBuffer(const BufferLock buffer_lock);
    VkResult ExecuteBuffer(const BufferLock buffer_lock);
    VkResult WaitForExecute(const BufferLock buffer_lock);
    VkResult Dispatch(const BufferLock buffer_lock, const uint32_t x, const uint32_t y, const uint32_t z);
    VkResult BindPipeline(const BufferLock buffer_lock, const VkPipeline pipeline, const VkPipelineBindPoint bind_point);
    VkResult BindDescriptorSets(const BufferLock buffer_lock, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const uint32_t first_set, const std::vector<uint32_t> dynamic_offeset);

    std::shared_ptr<Device> device;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    uint32_t family_queue_index = 0;
    std::mutex locks_control_mutex;
    struct Commandbuffer
    {
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      VkCommandBuffer buffer = VK_NULL_HANDLE;
      BufferLock lock;
      VkFence exec_fence = VK_NULL_HANDLE;
      bool started = false;
      bool reseted = true;
      bool finished = false;
    };
    std::vector<Commandbuffer> command_buffers;
  };

  class CommandPool
  {
  private:
    std::unique_ptr<CommandPool_impl> impl;
  public:
    CommandPool() = delete;
    CommandPool(const CommandPool &obj) = delete;
    CommandPool(CommandPool &&obj) noexcept : impl(std::move(obj.impl)) {};
    CommandPool(std::shared_ptr<Device> dev, const uint32_t family_queue_index) : 
      impl(std::unique_ptr<CommandPool_impl>(new CommandPool_impl(dev, family_queue_index))) {};
    CommandPool &operator=(const CommandPool &obj) = delete;
    CommandPool &operator=(CommandPool &&obj) noexcept;
    ~CommandPool() = default;
    void swap(CommandPool &obj) noexcept;
    bool IsValid() { return impl != nullptr; }

    VkCommandPool GetCommandPool() { return impl->GetCommandPool(); }
    size_t GetCommandBuffersCount() { return impl->GetCommandBuffersCount(); }
    BufferLock BeginCommandBuffer(const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) { return impl->BeginCommandBuffer(level); }
    VkResult EndCommandBuffer(const BufferLock &buffer_lock) {return impl->EndCommandBuffer(buffer_lock); }
    VkResult ResetCommandBuffer(const BufferLock &buffer_lock) {return impl->ResetCommandBuffer(buffer_lock); }
    VkResult ExecuteBuffer(const BufferLock &buffer_lock) {return impl->ExecuteBuffer(buffer_lock); }
    VkResult WaitForExecute(const BufferLock &buffer_lock) {return impl->WaitForExecute(buffer_lock); }
    VkResult Dispatch(const BufferLock &buffer_lock, const uint32_t x, const uint32_t y, const uint32_t z) {return impl->Dispatch(buffer_lock, x, y, z); }

    VkResult BindPipeline(const BufferLock buffer_lock, const VkPipeline pipeline, const VkPipelineBindPoint bind_point) { return impl->BindPipeline(buffer_lock, pipeline, bind_point); }
    VkResult BindDescriptorSets(const BufferLock buffer_lock, const VkPipelineLayout pipeline_layout, const VkPipelineBindPoint bind_point, const std::vector<VkDescriptorSet> sets, const uint32_t first_set, const std::vector<uint32_t> dynamic_offeset)
    {
      return impl->BindDescriptorSets(buffer_lock, pipeline_layout, bind_point, sets, first_set, dynamic_offeset);
    }
  };

  void swap(CommandPool &lhs, CommandPool &rhs) noexcept;
}

#endif