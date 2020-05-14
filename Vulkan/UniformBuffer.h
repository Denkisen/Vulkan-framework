#ifndef __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H
#define __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  class UniformBuffer : public IStorage
  {
  private:
    void Create(Device &dev, void *data, size_t len);
    void Create(VkDevice dev, VkPhysicalDevice p_dev, void *data, size_t len, uint32_t f_queue);
  public:
    UniformBuffer() = delete;
    UniformBuffer(Device &dev, void *data, size_t len);
    UniformBuffer(const UniformBuffer &obj);
    UniformBuffer& operator= (const UniformBuffer &obj);
    void Extract(void *out) const;
    ~UniformBuffer()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };

}

#endif