#include "Misc.h"
#include "Logger.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace Vulkan
{
  const std::vector<VkFormat> Misc::bit8 =
      {
          VK_FORMAT_R4G4_UNORM_PACK8,
          VK_FORMAT_R8_UNORM,
          VK_FORMAT_R8_SNORM,
          VK_FORMAT_R8_USCALED,
          VK_FORMAT_R8_SSCALED,
          VK_FORMAT_R8_UINT,
          VK_FORMAT_R8_SINT,
          VK_FORMAT_R8_SRGB
      };

  const std::vector<VkFormat> Misc::bit16 =
      {
          VK_FORMAT_R4G4B4A4_UNORM_PACK16,
          VK_FORMAT_B4G4R4A4_UNORM_PACK16,
          VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT,
          VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT,
          VK_FORMAT_R5G6B5_UNORM_PACK16,
          VK_FORMAT_B5G6R5_UNORM_PACK16,
          VK_FORMAT_R5G5B5A1_UNORM_PACK16,
          VK_FORMAT_B5G5R5A1_UNORM_PACK16,
          VK_FORMAT_A1R5G5B5_UNORM_PACK16,
          VK_FORMAT_R8G8_UNORM,
          VK_FORMAT_R8G8_SNORM,
          VK_FORMAT_R8G8_USCALED,
          VK_FORMAT_R8G8_SSCALED,
          VK_FORMAT_R8G8_UINT,
          VK_FORMAT_R8G8_SINT,
          VK_FORMAT_R8G8_SRGB,
          VK_FORMAT_R16_UNORM,
          VK_FORMAT_R16_SNORM,
          VK_FORMAT_R16_USCALED,
          VK_FORMAT_R16_SSCALED,
          VK_FORMAT_R16_UINT,
          VK_FORMAT_R16_SINT,
          VK_FORMAT_R16_SFLOAT,
          VK_FORMAT_R10X6_UNORM_PACK16,
          VK_FORMAT_R12X4_UNORM_PACK16
      };

  const std::vector<VkFormat> Misc::bit24 =
      {
          VK_FORMAT_R8G8B8_UNORM,
          VK_FORMAT_R8G8B8_SNORM,
          VK_FORMAT_R8G8B8_USCALED,
          VK_FORMAT_R8G8B8_SSCALED,
          VK_FORMAT_R8G8B8_UINT,
          VK_FORMAT_R8G8B8_SINT,
          VK_FORMAT_R8G8B8_SRGB,
          VK_FORMAT_B8G8R8_UNORM,
          VK_FORMAT_B8G8R8_SNORM,
          VK_FORMAT_B8G8R8_USCALED,
          VK_FORMAT_B8G8R8_SSCALED,
          VK_FORMAT_B8G8R8_UINT,
          VK_FORMAT_B8G8R8_SINT,
          VK_FORMAT_B8G8R8_SRGB
      };

  const std::vector<VkFormat> Misc::bit32 =
      {
          VK_FORMAT_R8G8B8A8_UNORM,
          VK_FORMAT_R8G8B8A8_SNORM,
          VK_FORMAT_R8G8B8A8_USCALED,
          VK_FORMAT_R8G8B8A8_SSCALED,
          VK_FORMAT_R8G8B8A8_UINT,
          VK_FORMAT_R8G8B8A8_SINT,
          VK_FORMAT_R8G8B8A8_SRGB,
          VK_FORMAT_B8G8R8A8_UNORM,
          VK_FORMAT_B8G8R8A8_SNORM,
          VK_FORMAT_B8G8R8A8_USCALED,
          VK_FORMAT_B8G8R8A8_SSCALED,
          VK_FORMAT_B8G8R8A8_UINT,
          VK_FORMAT_B8G8R8A8_SINT,
          VK_FORMAT_B8G8R8A8_SRGB,
          VK_FORMAT_A8B8G8R8_UNORM_PACK32,
          VK_FORMAT_A8B8G8R8_SNORM_PACK32,
          VK_FORMAT_A8B8G8R8_USCALED_PACK32,
          VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
          VK_FORMAT_A8B8G8R8_UINT_PACK32,
          VK_FORMAT_A8B8G8R8_SINT_PACK32,
          VK_FORMAT_A8B8G8R8_SRGB_PACK32,
          VK_FORMAT_A2R10G10B10_UNORM_PACK32,
          VK_FORMAT_A2R10G10B10_SNORM_PACK32,
          VK_FORMAT_A2R10G10B10_USCALED_PACK32,
          VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
          VK_FORMAT_A2R10G10B10_UINT_PACK32,
          VK_FORMAT_A2R10G10B10_SINT_PACK32,
          VK_FORMAT_A2B10G10R10_UNORM_PACK32,
          VK_FORMAT_A2B10G10R10_SNORM_PACK32,
          VK_FORMAT_A2B10G10R10_USCALED_PACK32,
          VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
          VK_FORMAT_A2B10G10R10_UINT_PACK32,
          VK_FORMAT_A2B10G10R10_SINT_PACK32,
          VK_FORMAT_R16G16_UNORM,
          VK_FORMAT_R16G16_SNORM,
          VK_FORMAT_R16G16_USCALED,
          VK_FORMAT_R16G16_SSCALED,
          VK_FORMAT_R16G16_UINT,
          VK_FORMAT_R16G16_SINT,
          VK_FORMAT_R16G16_SFLOAT,
          VK_FORMAT_R32_UINT,
          VK_FORMAT_R32_SINT,
          VK_FORMAT_R32_SFLOAT,
          VK_FORMAT_B10G11R11_UFLOAT_PACK32,
          VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
          VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
          VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
          VK_FORMAT_G8B8G8R8_422_UNORM,
          VK_FORMAT_B8G8R8G8_422_UNORM
      };

  const std::vector<VkFormat> Misc::bit48 =
      {
          VK_FORMAT_R16G16B16_UNORM,
          VK_FORMAT_R16G16B16_SNORM,
          VK_FORMAT_R16G16B16_USCALED,
          VK_FORMAT_R16G16B16_SSCALED,
          VK_FORMAT_R16G16B16_UINT,
          VK_FORMAT_R16G16B16_SINT,
          VK_FORMAT_R16G16B16_SFLOAT
      };

  const std::vector<VkFormat> Misc::bit64 =
      {
          VK_FORMAT_R16G16B16A16_UNORM,
          VK_FORMAT_R16G16B16A16_SNORM,
          VK_FORMAT_R16G16B16A16_USCALED,
          VK_FORMAT_R16G16B16A16_SSCALED,
          VK_FORMAT_R16G16B16A16_UINT,
          VK_FORMAT_R16G16B16A16_SINT,
          VK_FORMAT_R16G16B16A16_SFLOAT,
          VK_FORMAT_R32G32_UINT,
          VK_FORMAT_R32G32_SINT,
          VK_FORMAT_R32G32_SFLOAT,
          VK_FORMAT_R64_UINT,
          VK_FORMAT_R64_SINT,
          VK_FORMAT_R64_SFLOAT,
          VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
          VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
          VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
          VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
          VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
          VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
          VK_FORMAT_G16B16G16R16_422_UNORM,
          VK_FORMAT_B16G16R16G16_422_UNORM
      };

  const std::vector<VkFormat> Misc::bit96 =
      {
          VK_FORMAT_R32G32B32_UINT,
          VK_FORMAT_R32G32B32_SINT,
          VK_FORMAT_R32G32B32_SFLOAT
      };

  const std::vector<VkFormat> Misc::bit128 =
      {
          VK_FORMAT_R32G32B32A32_UINT,
          VK_FORMAT_R32G32B32A32_SINT,
          VK_FORMAT_R32G32B32A32_SFLOAT,
          VK_FORMAT_R64G64_UINT,
          VK_FORMAT_R64G64_SINT,
          VK_FORMAT_R64G64_SFLOAT
      };

  const std::vector<VkFormat> Misc::bit192 =
      {
          VK_FORMAT_R64G64B64_UINT,
          VK_FORMAT_R64G64B64_SINT,
          VK_FORMAT_R64G64B64_SFLOAT
      };

  const std::vector<VkFormat> Misc::bit256 =
      {
          VK_FORMAT_R64G64B64A64_UINT,
          VK_FORMAT_R64G64B64A64_SINT,
          VK_FORMAT_R64G64B64A64_SFLOAT
      };

  std::vector<const char *> Misc::RequiredLayers = 
  {
#ifdef DEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
  };

  std::vector<const char *> Misc::RequiredGraphicDeviceExtensions = 
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) 
  {
    Logger::EchoInfo(pCallbackData->pMessage, "DebugCallback");
    return VK_FALSE;
  }

  VkResult Misc::CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger) noexcept
  {
    VkDebugUtilsMessengerCreateInfoEXT create_d_info = {};
    create_d_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_d_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_d_info.messageType = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_d_info.pfnUserCallback = DebugCallback;
    create_d_info.pUserData = nullptr;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    return func(instance, &create_d_info, nullptr, &debug_messenger);
  }

  void Misc::DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger) noexcept
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    func(instance, debug_messenger, nullptr);
  }

  SwapChainDetails Misc::GetSwapChainDetails(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) noexcept
  {
    SwapChainDetails ret;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &ret.capabilities) != VK_SUCCESS)
    {
      Logger::EchoError("Can't get surface capabilities", __func__);
      return ret;
    }

    uint32_t count;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr) != VK_SUCCESS)
    {
      Logger::EchoError("Can't get surface formats", __func__);
      return ret;
    }

    if (count != 0) 
    {
      try
      {
        ret.formats.resize(count);
      }
      catch (...)
      {
        return ret;
      }
      
      if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, ret.formats.data()) != VK_SUCCESS)
      {
        Logger::EchoError("Can't get surface formats", __func__);
        return ret;
      }
    }

    count = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr) != VK_SUCCESS)
    {
      Logger::EchoError("Can't get surface present modes", __func__);
      return ret;
    }

    if (count != 0) 
    {
      try
      {
        ret.present_modes.resize(count);
      }
      catch (...)
      {
        return ret;
      }
      
      if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, ret.present_modes.data()) != VK_SUCCESS)
      {
        Logger::EchoError("Can't get surface present modes", __func__);
        return ret;
      }
    }

    return ret;
  }

  size_t Misc::SizeOfFormat(const VkFormat format) noexcept
  {
    if (std::find(bit8.begin(), bit8.end(), format) != bit8.end())
      return 1;
    if (std::find(bit16.begin(), bit16.end(), format) != bit16.end())
      return 2;
    if (std::find(bit24.begin(), bit24.end(), format) != bit24.end())
      return 3;
    if (std::find(bit32.begin(), bit32.end(), format) != bit32.end())
      return 4;
    if (std::find(bit48.begin(), bit48.end(), format) != bit48.end())
      return 6;
    if (std::find(bit64.begin(), bit64.end(), format) != bit64.end())
      return 8;
    if (std::find(bit96.begin(), bit96.end(), format) != bit96.end())
      return 12;
    if (std::find(bit128.begin(), bit128.end(), format) != bit128.end())
      return 16;
    if (std::find(bit192.begin(), bit192.end(), format) != bit192.end())
      return 24;
    if (std::find(bit256.begin(), bit256.end(), format) != bit256.end())
      return 32;

    return 0;
  }

  VkShaderModule Misc::LoadPrecompiledShaderFromFile(const VkDevice dev, const std::string file_name) noexcept
  {
    VkShaderModule shader = VK_NULL_HANDLE;
  
    auto shader_c = LoadShaderFromFile(file_name);
    shader = CreateShaderModule(dev, shader_c);
    if (shader == VK_NULL_HANDLE)
    {
      Logger::EchoError("Can't create shader", __func__);
    }

    return shader;
  }

  std::vector<char> Misc::LoadShaderFromFile(std::string file_name) noexcept
  {
    try
    {
      std::ifstream f(file_name, std::ios::ate | std::ios::binary);
      std::vector<char> res;
      if (!f.is_open())
      {
        Logger::EchoError("Error: No shader file (" + file_name + ")", __func__);
        return res;
      }

      res.resize((uint32_t)f.tellg());
      f.seekg(0);
      f.read(res.data(), res.size());
      f.close();

      return res;
    }
    catch (...)
    {
      return {};
    }
  }

  VkShaderModule Misc::CreateShaderModule(const VkDevice dev, const std::vector<char>& code) noexcept
  {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(dev, &create_info, nullptr, &shader_module);
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't create shader", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(res), __func__);
    } 
  
    return shader_module;
  }

  std::string Misc::GetExecDirectory(const std::string argc_path) noexcept
  {
    try
    {
      std::string path = argc_path;

      for (size_t i = path.size() - 1; i >= 0; --i)
      {
        if (path[i] != '/')
          path.pop_back();
        else
          break;
      }

      return path;
    }
    catch(...)
    {
      return std::string();
    }
  }

  std::string Misc::GetFileExtention(const std::string file) noexcept
  {
    size_t pos = file.find_last_of(".");
    if (pos == file.size())
      return "";

    try
    {
      std::string ext = file.substr(pos);
      std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

      return ext;
    }
    catch (...)
    {
      return std::string();
    }
  }

  VkPipelineLayout Misc::CreatePipelineLayout(const VkDevice dev, const std::vector<VkDescriptorSetLayout> desc_layouts) noexcept
  {
    VkPipelineLayout result = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = desc_layouts.empty() ? 0 : (uint32_t) desc_layouts.size();
    pipeline_layout_create_info.pSetLayouts = desc_layouts.empty() ? nullptr : desc_layouts.data();

    auto er = vkCreatePipelineLayout(dev, &pipeline_layout_create_info, nullptr, &result);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create pipeline layout", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return result;
  }

  VkDeviceSize Misc::Align(const VkDeviceSize value, const VkDeviceSize align) noexcept
  {
    return align > 0 ? (std::ceil(value / (float) align) * align) : 0;
  }
}

