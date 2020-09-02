#ifndef __CPU_NW_VULKAN_PIPELINE_H
#define __CPU_NW_VULKAN_PIPELINE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>

#include "Supply.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"

namespace Vulkan
{
  enum class PipelineType
  {
    Compute,
    Graphic
  };

  class IPipeline
  {
  protected:
    Vulkan::PipelineType type = PipelineType::Graphic;

    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::SwapChain> swapchain;
    std::shared_ptr<Vulkan::RenderPass> render_pass;

    std::vector<VkPipeline> pipelines;
    std::vector<VkPipelineLayout> pipeline_layouts;
    IPipeline() = default;
    void Destroy();
  public:
    IPipeline(const IPipeline &obj) = delete;
    IPipeline& operator= (const IPipeline &obj) = delete;
    Vulkan::PipelineType Type() const { return type; }
    ~IPipeline()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      Destroy();
      device.reset();
      swapchain.reset();
      render_pass.reset();
    }
  };
}

#endif