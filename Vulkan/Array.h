#ifndef __CPU_NW_LIBS_VULKAN_ARRAY_H
#define __CPU_NW_LIBS_VULKAN_ARRAY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "IStorage.h"
#include "Device.h"


namespace Vulkan
{
  template <typename T> class Array : public IStorage
  {
  private:
    void Create(Device &dev, T *data, size_t len);
    void Create(VkDevice dev, VkPhysicalDevice p_dev, T *data, size_t len, uint32_t f_queue);
  public:
    Array() = delete;
    Array(Device &dev, std::vector<T> &data);
    Array(Device &dev, T *data, size_t len);
    Array(const Array<T> &array);
    Array<T>& operator= (const Array<T> &obj);
    std::vector<T> Extract() const;
    ~Array()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };
}

#endif