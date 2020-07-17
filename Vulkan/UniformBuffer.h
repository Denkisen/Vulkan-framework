#ifndef __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H
#define __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  class UniformBuffer : public IStorage
  {
  private:
    void Create(std::shared_ptr<Vulkan::Device> dev, void *data, std::size_t len, uint32_t f_queue);
  public:
    UniformBuffer() = delete;
    UniformBuffer(std::shared_ptr<Vulkan::Device> dev, void *data, std::size_t len, uint32_t family_q);
    UniformBuffer(const UniformBuffer &obj);
    UniformBuffer& operator= (const UniformBuffer &obj);
    ~UniformBuffer()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };
}

#endif