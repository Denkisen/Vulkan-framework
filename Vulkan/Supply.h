
#ifndef __CPU_NW_VULKAN_SUPPLY_H
#define __CPU_NW_VULKAN_SUPPLY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>

namespace Vulkan
{
  struct VertexDescription
  {
    uint32_t offset = 0; // offset in bytes of struct member
    VkFormat format = VK_FORMAT_R32G32_SFLOAT; // struct member format
  };

  struct SwapChainDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  enum class ShaderType
  {
    Vertex,
    Fragment,
    Compute,
  };

  struct ShaderInfo
  {
    std::string entry = "main";
    std::string file_path;
    Vulkan::ShaderType type;
  };

  enum class StorageType
  {
    None,
    Storage,
    Uniform,
    Vertex,
    Index
  };

  class Supply
  {
  private:
    Supply() {}
    ~Supply() {}
    static std::vector<char> LoadShaderFromFile(std::string file_name);
    static VkShaderModule CreateShaderModule(VkDevice &dev, std::vector<char>& code);
  public:
    static std::vector<const char *> ValidationLayers;
    static std::vector<const char *> RequiredGraphicDeviceExtensions;
    static std::vector<std::string> GetInstanceExtensions();
    static std::vector<std::string> GetPhysicalDeviceExtensions(VkPhysicalDevice &device);
    static std::optional<size_t> GetMemoryTypeIndex(VkDevice dev, VkPhysicalDevice p_dev, VkBuffer buffer, uint32_t &buffer_size, VkMemoryPropertyFlags flags);
    static std::optional<size_t> GetMemoryTypeIndex(VkDevice dev, VkPhysicalDevice p_dev, VkImage image, uint32_t &buffer_size, VkMemoryPropertyFlags flags);
    static std::vector<VkPhysicalDevice> GetPhysicalDevicesByType(VkInstance &instance, VkPhysicalDeviceType type);
    static std::vector<VkPhysicalDevice> GetAllPhysicalDevices(VkInstance &instance);
    static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice &dev);
    static uint32_t GetPhisicalDevicesCount(VkInstance &instance);
    static VkResult LoadPrecompiledShaderFromFile(VkDevice dev, std::string file_name, VkShaderModule &shader);
    static VkResult CreateShaderStageInfo(VkDevice device, Vulkan::ShaderInfo &info, VkShaderModule &module, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo &out);
    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static VkQueue GetQueueFormFamilyIndex(const VkDevice &device, const uint32_t index);
    static uint32_t GetFamilyQueuesCount(VkPhysicalDevice &dev);
    static bool CheckQueueBit(VkPhysicalDevice &dev, uint32_t index, VkQueueFlagBits bit); 
    static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice &device, VkSurfaceKHR &surface);
    static VkCommandBuffer CreateCommandBuffer(VkDevice device, const VkCommandPool pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    static std::vector<VkCommandBuffer> CreateCommandBuffers(VkDevice device, const VkCommandPool pool, const uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    static VkCommandPool CreateCommandPool(VkDevice device, const uint32_t family_queue);
    static VkPipelineLayout CreatePipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> layouts);
    template <class T>
    static void GetVertexInputBindingDescription(uint32_t binding, std::vector<VertexDescription> vertex_descriptions, VkVertexInputBindingDescription &out_binding_description, std::vector<VkVertexInputAttributeDescription> &out_attribute_descriptions);
    static VkDescriptorType StorageTypeToDescriptorType(Vulkan::StorageType t);
    static VkBufferUsageFlags StorageTypeToBufferUsageFlags(Vulkan::StorageType t);
  };
}
namespace Vulkan
{
  template <class T>
  void Supply::GetVertexInputBindingDescription(uint32_t binding, std::vector<VertexDescription> vertex_descriptions, VkVertexInputBindingDescription &out_binding_description, std::vector<VkVertexInputAttributeDescription> &out_attribute_descriptions)
  {
    if (vertex_descriptions.empty())
      throw std::runtime_error("Vertex description is empty.");
    
    out_binding_description.binding = binding;
    out_binding_description.stride = sizeof(T);
    out_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    out_attribute_descriptions.resize(vertex_descriptions.size());
    for (size_t i = 0; i < out_attribute_descriptions.size(); ++i)
    {
      out_attribute_descriptions[i].binding = binding;
      out_attribute_descriptions[i].location = i;
      out_attribute_descriptions[i].format = vertex_descriptions[i].format;
      out_attribute_descriptions[i].offset = vertex_descriptions[i].offset;
    }
  }
}


#endif