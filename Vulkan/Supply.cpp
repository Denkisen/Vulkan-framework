#include "Supply.h"
#include <fstream>
#include <string.h>
#include <algorithm>

std::vector<const char *> Vulkan::Supply::ValidationLayers = 
{
  "VK_LAYER_KHRONOS_validation"
};

std::vector<const char *> Vulkan::Supply::RequiredGraphicDeviceExtensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) 
{
  std::ofstream f;
  f.open("log.txt", std::ios::app);
  std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
  f << pCallbackData->pMessage << std::endl;
  f.close();

  return VK_FALSE;
}

VkResult Vulkan::Supply::CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
{
  VkDebugUtilsMessengerCreateInfoEXT create_d_info = {};
  create_d_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_d_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_d_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_d_info.pfnUserCallback = DebugCallback;
  create_d_info.pUserData = nullptr;
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  return func(instance, &create_d_info, nullptr, &debug_messenger);
}

void Vulkan::Supply::DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  func(instance, debug_messenger, nullptr);
}

VkPhysicalDeviceFeatures Vulkan::Supply::GetPhysicalDeviceFeatures(VkPhysicalDevice &dev)
{
  VkPhysicalDeviceFeatures result = {};
  if (dev != VK_NULL_HANDLE)
    vkGetPhysicalDeviceFeatures(dev, &result);
  return result;
}

std::vector<VkPhysicalDevice> Vulkan::Supply::GetAllPhysicalDevices(VkInstance &instance)
{
  std::vector<VkPhysicalDevice> ret(GetPhisicalDevicesCount(instance), VK_NULL_HANDLE);
  uint32_t count = (uint32_t) ret.size();
  if (ret.empty() || vkEnumeratePhysicalDevices(instance, &count, ret.data()) != VK_SUCCESS) 
    ret.clear();

  return ret;
}

std::vector<VkPhysicalDevice> Vulkan::Supply::GetPhysicalDevicesByType(VkInstance &instance, VkPhysicalDeviceType type)
{
  std::vector<VkPhysicalDevice> ret;
  std::vector<VkPhysicalDevice> devices = GetAllPhysicalDevices(instance);

  for (auto &dev : devices)
  {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(dev, &device_properties);
    if (device_properties.deviceType == type)
    {
      ret.push_back(dev);
    }
  }

  return ret;
}

uint32_t Vulkan::Supply::GetPhisicalDevicesCount(VkInstance &instance)
{
  if (instance == VK_NULL_HANDLE) return 0;

  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  return device_count;
}

uint32_t Vulkan::Supply::GetFamilyQueuesCount(VkPhysicalDevice &dev)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);
  return queue_family_count;
}

bool Vulkan::Supply::CheckQueueBit(VkPhysicalDevice &dev, uint32_t index, VkQueueFlagBits bit)
{
  if (dev == VK_NULL_HANDLE) return false;

  uint32_t queue_family_count = GetFamilyQueuesCount(dev);
  if (index >= queue_family_count) return false;

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families.data());

  return queue_families[index].queueFlags & bit;
}

std::vector<char> Vulkan::Supply::LoadShaderFromFile(std::string file_name)
{
  std::ifstream f(file_name, std::ios::ate | std::ios::binary);
  std::vector<char> res;
  if (!f.is_open())
  {
    std::cout << "Error: No shader file (" << file_name << ")" << std::endl;
    return res;
  }
  

  res.resize((uint32_t) f.tellg());
  f.seekg(0);
  f.read(res.data(), res.size());
  f.close();
  return res;
}

VkShaderModule Vulkan::Supply::CreateShaderModule(VkDevice &dev, std::vector<char> &code)
{
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shader_module = VK_NULL_HANDLE;
  VkResult res = vkCreateShaderModule(dev, &create_info, nullptr, &shader_module);
  if (res != VK_SUCCESS) return VK_NULL_HANDLE;
  
  return shader_module;
}

VkResult Vulkan::Supply::LoadPrecompiledShaderFromFile(VkDevice dev, std::string file_name, VkShaderModule &shader)
{
  if (shader != VK_NULL_HANDLE)
    vkDestroyShaderModule(dev, shader, nullptr);
  shader = VK_NULL_HANDLE;
  
  auto shader_c = LoadShaderFromFile(file_name);
  shader = CreateShaderModule(dev, shader_c);
  if (shader != VK_NULL_HANDLE) return VK_SUCCESS;

  return VK_ERROR_INVALID_SHADER_NV;
}

std::vector<std::string> Vulkan::Supply::GetInstanceExtensions()
{
  std::vector<std::string> res;

  uint32_t ext_count = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr) == VK_SUCCESS)
  {
    std::vector<VkExtensionProperties> props(ext_count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, props.data()) == VK_SUCCESS)
    {
      for (auto &ext : props) 
      {
        res.push_back(ext.extensionName);
      }
    }
  }

  return res;
}

VkQueue Vulkan::Supply::GetQueueFormFamilyIndex(const VkDevice &device, const uint32_t index)
{
  VkQueue q;
  vkGetDeviceQueue(device, index, 0, &q);
  return q;
}

std::vector<std::string> Vulkan::Supply::GetPhysicalDeviceExtensions(VkPhysicalDevice &device)
{
  uint32_t count;
  if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Can't enumerate device extensions");
  std::vector<VkExtensionProperties> available_extensions(count);
  if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available_extensions.data()) != VK_SUCCESS)
    throw std::runtime_error("Can't enumerate device extensions");
  std::vector<std::string> ret;

  for (size_t i = 0; i < count; ++i)
  {
    ret.push_back(available_extensions[i].extensionName);
  }

  return ret;
}

Vulkan::SwapChainDetails Vulkan::Supply::GetSwapChainDetails(VkPhysicalDevice &device, VkSurfaceKHR &surface)
{
  Vulkan::SwapChainDetails ret;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &ret.capabilities) != VK_SUCCESS)
    throw std::runtime_error("Can't get surface capabilities.");

  uint32_t count;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Can't get surface formats.");

  if (count != 0) 
  {
    ret.formats.resize(count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, ret.formats.data()) != VK_SUCCESS)
      throw std::runtime_error("Can't get surface formats.");
  }

  count = 0;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Can't get surface present modes.");

  if (count != 0) 
  {
    ret.present_modes.resize(count);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, ret.present_modes.data()) != VK_SUCCESS)
      throw std::runtime_error("Can't get surface present modes.");
  }

  return ret;
}

VkResult Vulkan::Supply::CreateShaderStageInfo(VkDevice device, Vulkan::ShaderInfo &info, VkShaderModule &module, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo &out)
{
  out.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  out.stage = stage;
  out.pSpecializationInfo = nullptr;
  if (Vulkan::Supply::LoadPrecompiledShaderFromFile(device, info.file_path, module) == VK_SUCCESS)
  {
      out.module = module;
      out.pName = info.entry.c_str();
      return VK_SUCCESS;
  }

  return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

VkCommandBuffer Vulkan::Supply::CreateCommandBuffer(VkDevice device, const VkCommandPool pool, VkCommandBufferLevel level)
{
  VkCommandBuffer result = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.commandPool = pool; 
  command_buffer_allocate_info.level = level;
  command_buffer_allocate_info.commandBufferCount = 1; 
  if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &result) != VK_SUCCESS)
    throw std::runtime_error("Can't allocate command buffer.");
    
  return result;
}

std::vector<VkCommandBuffer> Vulkan::Supply::CreateCommandBuffers(VkDevice device, const VkCommandPool pool, const uint32_t count, VkCommandBufferLevel level)
{
  std::vector<VkCommandBuffer> result(count);
  VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.commandPool = pool; 
  command_buffer_allocate_info.level = level;
  command_buffer_allocate_info.commandBufferCount = count; 
  if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, result.data()) != VK_SUCCESS)
    throw std::runtime_error("Can't allocate command buffers.");
    
  return result;
}

VkCommandPool Vulkan::Supply::CreateCommandPool(VkDevice device, const uint32_t family_queue)
{
  VkCommandPool result = VK_NULL_HANDLE;
  VkCommandPoolCreateInfo command_pool_create_info = {};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = family_queue;

  if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("Can't create command pool."); 

  return result;
}

VkPipelineLayout Vulkan::Supply::CreatePipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> layouts)
{
  VkPipelineLayout result = VK_NULL_HANDLE;
  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = layouts.empty() ? 0 : (uint32_t) layouts.size();
  pipeline_layout_create_info.pSetLayouts = layouts.empty() ? nullptr : layouts.data();
  if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("Can't create pipeline layout.");

  return result;
}

std::optional<size_t> Vulkan::Supply::GetMemoryTypeIndex(VkDevice dev, VkPhysicalDevice p_dev, VkBuffer buffer, std::pair<uint32_t, uint32_t> &buffer_size, VkMemoryPropertyFlags flags)
{
  std::optional<size_t> res;
  VkPhysicalDeviceMemoryProperties properties;
  vkGetPhysicalDeviceMemoryProperties(p_dev, &properties);
  VkMemoryRequirements mem_req = {};
  vkGetBufferMemoryRequirements(dev, buffer, &mem_req);
  buffer_size.first = mem_req.size;
  buffer_size.second = mem_req.alignment;

  for (size_t i = 0; i < properties.memoryTypeCount; i++) 
  {
    if (mem_req.memoryTypeBits & (1 << i) && 
        (properties.memoryTypes[i].propertyFlags & flags) &&
        (buffer_size.first < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
    {
      res = i;
      break;
    }
  }

  return res;
}

std::optional<size_t> Vulkan::Supply::GetMemoryTypeIndex(VkDevice dev, VkPhysicalDevice p_dev, std::vector<VkBuffer> buffers, std::pair<uint32_t, uint32_t> &buffer_size, VkMemoryPropertyFlags flags)
{
  std::optional<size_t> res;
  VkPhysicalDeviceMemoryProperties properties;
  vkGetPhysicalDeviceMemoryProperties(p_dev, &properties);
  VkMemoryRequirements mem_req = {};
  std::pair<uint32_t, uint32_t> mem_size = std::make_pair(0, 0);

  for (size_t i = 0; i < buffers.size(); ++i)
  {
    mem_req = {};
    vkGetBufferMemoryRequirements(dev, buffers[i], &mem_req);
    mem_size.first += mem_req.size;
  }
  mem_size.second = mem_req.alignment;

  for (size_t i = 0; i < properties.memoryTypeCount; i++) 
  {
    if (mem_req.memoryTypeBits & (1 << i) && 
        (properties.memoryTypes[i].propertyFlags & flags) &&
        (mem_size.first < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
    {
      res = i;
      break;
    }
  }
  buffer_size = mem_size;
  
  return res;
}

std::optional<size_t> Vulkan::Supply::GetMemoryTypeIndex(VkDevice dev, VkPhysicalDevice p_dev, VkImage image, std::pair<uint32_t, uint32_t> &buffer_size, VkMemoryPropertyFlags flags)
{
  std::optional<size_t> res;
  VkPhysicalDeviceMemoryProperties properties;
  vkGetPhysicalDeviceMemoryProperties(p_dev, &properties);
  VkMemoryRequirements mem_req = {};
  vkGetImageMemoryRequirements(dev, image, &mem_req);
  buffer_size.first = mem_req.size;
  buffer_size.second = mem_req.alignment;
  
  for (size_t i = 0; i < properties.memoryTypeCount; i++) 
  {
    if (mem_req.memoryTypeBits & (1 << i) && 
        (properties.memoryTypes[i].propertyFlags & flags) &&
        (buffer_size.first < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
    {
      res = i;
      break;
    }
  }

  return res;
}

std::string Vulkan::Supply::GetExecDirectory(const std::string argc_path)
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

std::string Vulkan::Supply::GetFileExtention(const std::string file)
{
  size_t pos = file.find_last_of(".");
  if (pos == file.size())
    return "";

  std::string ext = file.substr(pos);
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

  return ext;
}

size_t Vulkan::Supply::SizeOfFormat(const VkFormat format)
{
  const std::vector<VkFormat> bit8 =
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
  const std::vector<VkFormat> bit16 =
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

  const std::vector<VkFormat> bit24 =
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

  const std::vector<VkFormat> bit32 =
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

  const std::vector<VkFormat> bit48 =
  {
    VK_FORMAT_R16G16B16_UNORM,
    VK_FORMAT_R16G16B16_SNORM,
    VK_FORMAT_R16G16B16_USCALED,
    VK_FORMAT_R16G16B16_SSCALED,
    VK_FORMAT_R16G16B16_UINT,
    VK_FORMAT_R16G16B16_SINT,
    VK_FORMAT_R16G16B16_SFLOAT
  };

  const std::vector<VkFormat> bit64 =
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

  const std::vector<VkFormat> bit96 =
  {
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R32G32B32_SFLOAT
  };

  const std::vector<VkFormat> bit128 =
  {
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R64G64_UINT,
    VK_FORMAT_R64G64_SINT,
    VK_FORMAT_R64G64_SFLOAT
  };

  const std::vector<VkFormat> bit192 =
  {
    VK_FORMAT_R64G64B64_UINT,
    VK_FORMAT_R64G64B64_SINT,
    VK_FORMAT_R64G64B64_SFLOAT
  };

  const std::vector<VkFormat> bit256 =
  {
    VK_FORMAT_R64G64B64A64_UINT,
    VK_FORMAT_R64G64B64A64_SINT,
    VK_FORMAT_R64G64B64A64_SFLOAT
  };

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