#include "Pipeline.h"
#include "Misc.h"

#include <filesystem>

namespace Vulkan
{
  Pipeline::~Pipeline()
  {
    Logger::EchoDebug("", __func__);
    for (auto &s : shaders)
    {
      if (s.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device->GetDevice(), s.shader, nullptr);
      }
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

  Pipeline::Pipeline(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  VkPipeline Pipeline::GetPipeline()
  {
    if (pipeline == VK_NULL_HANDLE)
      pipeline = BuildPipeline();

    return pipeline;
  }

  VkPipelineLayout Pipeline::GetPipelineLayout()
  {
    if (pipeline_layout == VK_NULL_HANDLE)
      pipeline_layout = CreatePipelineLayout();

    return pipeline_layout;
  }

  VkShaderModule Pipeline::CreateShader(const std::string shader_path)
  {
    return Misc::LoadPrecompiledShaderFromFile(device->GetDevice(), shader_path);;
  }

  VkResult Pipeline::AddShader(const ShaderInfo info)
  {
    VkResult result = VK_SUCCESS;
    result = std::filesystem::exists(info.file_path) ? VK_SUCCESS : VK_ERROR_UNKNOWN;

    if (result != VK_SUCCESS)
    {
      Logger::EchoError("File (" + info.file_path + ") does not exist", __func__);
      return result;
    }

    if (info.entry == "")
    {
      Logger::EchoError("Shaders entry point must not be empty", __func__);
      return result;
    }

    Shader sd = {};
    sd.info = info;
    sd.shader = CreateShader(sd.info.file_path);
    if (sd.shader != VK_NULL_HANDLE)
    {
      shaders.push_back(sd);
    }
    else
    {
      Logger::EchoError("Can't create shader module", __func__);
      result = VK_ERROR_UNKNOWN;
    }

    return result;
  }

  VkPipelineLayout Pipeline::CreatePipelineLayout()
  {
    VkPipelineLayout result = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = layouts.empty() ? 0 : (uint32_t) layouts.size();
    pipeline_layout_create_info.pSetLayouts = layouts.empty() ? nullptr : layouts.data();

    auto er = vkCreatePipelineLayout(device->GetDevice(), &pipeline_layout_create_info, nullptr, &result);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create pipeline layout", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return result;
  }

  VkResult Pipeline::AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts)
  {
    VkResult res = VK_SUCCESS;
    if (layouts.empty()) return res;

    for (const auto &l : layouts)
    {
      if (l != VK_NULL_HANDLE)
        this->layouts.push_back(l);
    }

    return res;
  }
}