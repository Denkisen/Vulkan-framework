#include "Pipeline.h"
#include "Logger.h"

namespace Vulkan
{
  ComputePipeline::~ComputePipeline()
  {
    Logger::EchoDebug("", __func__);
  };

  VkPipeline ComputePipeline::BuildPipeline()
  {
    VkPipeline result = VK_NULL_HANDLE;

    if (shaders.empty())
    {
      Logger::EchoError("No shaders available", __func__);
      return result;
    }

    if (layouts.empty())
    {
      Logger::EchoWarning("Descriptors layout is empty", __func__);
    }

    pipeline_layout = GetPipelineLayout();
    VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_create_info.module = shaders[0].shader;
    shader_stage_create_info.pName = shaders[0].info.entry.c_str();

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = shader_stage_create_info;
    pipeline_create_info.layout = pipeline_layout;

    auto er = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &result);
      
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create compute pipelines", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return result;
  }

  VkResult ComputePipeline::AddShader(const ShaderInfo info)
  {
    if (info.type != ShaderType::Compute)
    {
      Logger::EchoError("invalid shader type for compute pipeline", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (pipeline != VK_NULL_HANDLE)
    {
      Logger::EchoWarning("Pipeline is already compiled", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (!shaders.empty())
    {
      Logger::EchoWarning("Shader already setted up. Old shader will be discarded", __func__);
      auto tmp = shaders[0];
      shaders.clear();
      auto res = Pipeline::AddShader(info);
      if (res == VK_SUCCESS)
      {
        vkDestroyShaderModule(device->GetDevice(), tmp.shader, nullptr);
      }
      else
      {
        shaders.push_back(tmp);
      }
      
      return res;
    }

    return Pipeline::AddShader(info);
  }
}