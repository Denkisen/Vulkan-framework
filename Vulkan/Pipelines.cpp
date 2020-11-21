#include "Pipelines.h"
#include "Misc.h"

namespace Vulkan
{
  Pipelines::Pipelines(Pipelines &&obj) noexcept
  {
    std::lock_guard lock(obj.pipelines_mutex);

    pipelines = std::move(obj.pipelines);
  }

  Pipelines &Pipelines::operator=(Pipelines &&obj) noexcept
  {
    if (&obj == this) return *this;

    std::scoped_lock lock(pipelines_mutex, obj.pipelines_mutex);

    pipelines = std::move(obj.pipelines);
    return *this;
  }

  void Pipelines::swap(Pipelines &obj) noexcept
  {
    if (&obj == this) return;

    std::scoped_lock lock(pipelines_mutex, obj.pipelines_mutex);

    pipelines.swap(obj.pipelines);
  }

  void swap(Pipelines &lhs, Pipelines &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  VkResult Pipelines::AddPipeline(const std::shared_ptr<Device> dev, const ComputePipelineConfig &params)
  {
    std::lock_guard lock(pipelines_mutex);

    pipelines.emplace_back(ComputePipeline(dev, params));

    return std::visit([](auto&& obj) -> VkResult
      {
        return obj.GetPipeline() != VK_NULL_HANDLE ? VK_SUCCESS : VK_INCOMPLETE;
      }, pipelines[pipelines.size() - 1]);

  }

  VkResult Pipelines::AddPipeline(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const std::shared_ptr<RenderPass> render_pass, const GraphicPipelineConfig &params)
  {
    std::lock_guard lock(pipelines_mutex);

    pipelines.emplace_back(GraphicPipeline(dev, swapchain, render_pass, params));

    return std::visit([](auto&& obj) -> VkResult
      {
        return obj.GetPipeline() != VK_NULL_HANDLE ? VK_SUCCESS : VK_INCOMPLETE;
      }, pipelines[pipelines.size() - 1]);
  }

  VkResult Pipelines::AddPipeline(ComputePipeline &&obj)
  {
    std::lock_guard lock(pipelines_mutex);
    if (obj.GetPipeline() != VK_NULL_HANDLE)
    {
      pipelines.emplace_back(std::move(obj));
      return VK_SUCCESS;
    }

    return VK_INCOMPLETE;
  }

  VkResult Pipelines::AddPipeline(GraphicPipeline &&obj)
  {
    std::lock_guard lock(pipelines_mutex);
    if (obj.GetPipeline() != VK_NULL_HANDLE)
    {
      pipelines.emplace_back(std::move(obj));
      return VK_SUCCESS;
    }

    return VK_INCOMPLETE;
  }

  VkPipelineLayout Pipelines::GetLayout(const size_t index)
  {
    std::lock_guard lock(pipelines_mutex);
    if (index >= pipelines.size())
    {
      Logger::EchoError("Index is out off range", __func__);
      return VK_NULL_HANDLE;
    }
    
    return std::visit([] (auto &&obj) -> VkPipelineLayout { return obj.GetLayout(); }, pipelines[index]);
  }

  VkPipeline Pipelines::GetPipeline(const size_t index)
  {
    std::lock_guard lock(pipelines_mutex);
    if (index >= pipelines.size())
    {
      Logger::EchoError("Index is out off range", __func__);
      return VK_NULL_HANDLE;
    }

    return std::visit([] (auto &&obj) -> VkPipeline { return obj.GetPipeline(); }, pipelines[index]);
  }
}