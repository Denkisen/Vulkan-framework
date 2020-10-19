#include "Misc.h"
#include "Logger.h"

#include <iostream>
#include <fstream>

namespace Vulkan
{
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

  VkResult Misc::CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
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

  void Misc::DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
  {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    func(instance, debug_messenger, nullptr);
  }
}

