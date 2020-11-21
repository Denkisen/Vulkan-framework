#ifndef __VULKAN_COMPUTE_PIPELINE_H
#define __VULKAN_COMPUTE_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <algorithm>

#include "../Device.h"
#include "../Logger.h"
#include "Types.h"

namespace Vulkan
{
  class ComputePipelineConfig
  {
  private:
    friend class Pipelines;
    friend class ComputePipeline_impl;
    std::vector<VkDescriptorSetLayout> desc_layouts;
    VkPipeline base_pipeline = VK_NULL_HANDLE;
    ShaderInfo shader_info;
  public:
    ComputePipelineConfig() = default;
    ~ComputePipelineConfig() noexcept = default;
    auto &AddDescriptorSetLayout(const VkDescriptorSetLayout layout) { if (layout != VK_NULL_HANDLE) desc_layouts.push_back(layout); return *this; }
    auto &AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts)
    { 
      std::copy_if(layouts.begin(), layouts.end(), std::back_inserter(desc_layouts), [] (const auto &obj) { return obj != VK_NULL_HANDLE; });
      return *this;
    }
    auto &SetShader(const std::filesystem::path file_path, const std::string entry = "main")
    { 
      if (std::filesystem::exists(file_path)) shader_info = {entry, file_path, ShaderType::Compute}; 
      return *this; 
    }
    auto &SetBasePipeline(const VkPipeline pipeline) noexcept { base_pipeline = pipeline; return *this; }
  };

  class ComputePipeline_impl
  {  
  public:
    ComputePipeline_impl() = delete;
    ComputePipeline_impl(const ComputePipeline_impl &obj) = delete;
    ComputePipeline_impl(ComputePipeline_impl &&obj) = delete;
    ComputePipeline_impl &operator=(const ComputePipeline_impl &obj) = delete;
    ComputePipeline_impl &operator=(ComputePipeline_impl &&obj) = delete;
    ~ComputePipeline_impl() noexcept;
  private:
    friend class ComputePipeline;
    std::shared_ptr<Device> device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> desc_layouts;
    Shader shader;

    ComputePipeline_impl(const std::shared_ptr<Device> dev, const ComputePipelineConfig &params);
    VkPipeline GetPipeline() const noexcept { return pipeline; }
    VkPipelineLayout GetLayout() const noexcept { return pipeline_layout; }
  };

  class ComputePipeline
  {
  private:
    std::unique_ptr<ComputePipeline_impl> impl;
  public:
    ComputePipeline() = delete;
    ComputePipeline(const ComputePipeline &obj) = delete;
    ComputePipeline(ComputePipeline &&obj) noexcept : impl(std::move(obj.impl)) {};
    ComputePipeline(const std::shared_ptr<Device> dev, const ComputePipelineConfig &params) :
      impl(std::unique_ptr<ComputePipeline_impl>(new ComputePipeline_impl(dev, params))) {};
    ComputePipeline &operator=(const ComputePipeline &obj) = delete;
    ComputePipeline &operator=(ComputePipeline &&obj) noexcept;
    void swap(ComputePipeline &obj) noexcept;

    VkPipeline GetPipeline() const noexcept { if (impl.get()) return impl->GetPipeline(); return VK_NULL_HANDLE; }
    VkPipelineLayout GetLayout() const noexcept { if (impl.get()) return impl->GetLayout(); return VK_NULL_HANDLE; }
    bool IsValid() const noexcept { return impl.get() && impl->pipeline != VK_NULL_HANDLE; }
    ~ComputePipeline() noexcept = default;
  };

  void swap(ComputePipeline &lhs, ComputePipeline &rhs) noexcept;
}

#endif