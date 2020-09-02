#include "Pipeline.h"

namespace Vulkan
{
  void IPipeline::Destroy()
  {
    for (auto &pipeline : pipelines)
      if (pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
    pipelines.clear();

    for (auto &pipeline_layout : pipeline_layouts)
      if (pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
    pipeline_layouts.clear();
  }
}