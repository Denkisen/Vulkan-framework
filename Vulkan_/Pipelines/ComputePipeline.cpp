#include "ComputePipeline.h"
#include "../Misc.h"

namespace Vulkan
{
  ComputePipeline_impl::~ComputePipeline_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (shader.shader != VK_NULL_HANDLE)
    {
      vkDestroyShaderModule(device->GetDevice(), shader.shader, nullptr);
    }

    if (pipeline_layout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
      pipeline_layout = VK_NULL_HANDLE;
    }

    if (pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
  }

  ComputePipeline_impl::ComputePipeline_impl(const std::shared_ptr<Device> dev, const ComputePipelineConfig &params) noexcept
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    if (!std::filesystem::exists(params.shader_info.file_path))
    {
      Logger::EchoError("Shader file path is not valid", __func__);
      return;
    }

    if (params.shader_info.type != ShaderType::Compute)
    {
      Logger::EchoError("Invalid shader type", __func__);
      return;
    }

    device = dev;
    shader.entry = params.shader_info.entry;
    desc_layouts = params.desc_layouts;
    shader.shader = Misc::LoadPrecompiledShaderFromFile(device->GetDevice(), params.shader_info.file_path);
    pipeline_layout = Misc::CreatePipelineLayout(device->GetDevice(), desc_layouts);

    VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_create_info.module = shader.shader;
    shader_stage_create_info.pName = shader.entry.c_str();

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = shader_stage_create_info;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.basePipelineHandle = params.base_pipeline;
    pipeline_create_info.flags = params.base_pipeline != VK_NULL_HANDLE ? VK_PIPELINE_CREATE_DERIVATIVE_BIT : VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    is_base = params.base_pipeline == VK_NULL_HANDLE;

    auto er = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);
      
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create compute pipelines", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
  }

  ComputePipeline &ComputePipeline::operator=(ComputePipeline &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void ComputePipeline::swap(ComputePipeline &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(ComputePipeline &lhs, ComputePipeline &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}