#ifndef __VULKAN_PIPELINES_H
#define __VULKAN_PIPELINES_H

#include <vulkan/vulkan.h>
#include <memory>
#include <mutex>
#include <variant>
#include <filesystem>

#include "Device.h"
#include "Logger.h"
#include "Pipelines/ComputePipeline.h"
#include "Pipelines/GraphicPipeline.h"

namespace Vulkan
{
  using Pipeline = std::variant<ComputePipeline, GraphicPipeline>;

  class Pipelines
  {
  private:
    std::vector<Pipeline> pipelines;
    std::mutex pipelines_mutex;
  public:
    Pipelines() = default;
    Pipelines(const Pipelines &obj) = delete;
    Pipelines(Pipelines &&obj) noexcept;
    Pipelines &operator=(const Pipelines &obj) = delete;
    Pipelines &operator=(Pipelines &&obj) noexcept;
    void swap(Pipelines &obj) noexcept;
    VkResult AddPipeline(const std::shared_ptr<Device> dev, const ComputePipelineConfig &params);
    VkResult AddPipeline(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const GraphicPipelineConfig &params);
    VkResult AddPipeline(ComputePipeline &&obj);
    VkResult AddPipeline(GraphicPipeline &&obj);
    VkPipelineLayout GetLayout(const size_t index);
    VkPipeline GetPipeline(const size_t index);
    bool IsValid() { std::lock_guard lock(pipelines_mutex); return !pipelines.empty(); }
  };

  void swap(Pipelines &lhs, Pipelines &rhs) noexcept;
}

#endif