#ifndef __CPU_NW_VULKAN_OBJECT_H
#define __CPU_NW_VULKAN_OBJECT_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <vector>
#include <mutex>

#include "Device.h"
#include "Buffer.h"
#include "Image.h"
#include "CommandPool.h"
#include "Sampler.h"
#include "../libs/ImageBuffer.h"
#include "Vertex.h"

namespace Vulkan
{
  class Object
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::CommandPool> command_pool;
    std::shared_ptr<Vulkan::Image> texture_image;
    std::shared_ptr<Vulkan::Buffer<uint8_t>> texture_src_buffer;
    std::shared_ptr<Vulkan::Buffer<uint32_t>> index_buffer;
    std::shared_ptr<Vulkan::Buffer<uint32_t>> index_src_buffer;
    std::shared_ptr<Vulkan::Buffer<Vulkan::Vertex>> vertex_buffer;
    std::shared_ptr<Vulkan::Buffer<Vulkan::Vertex>> vertex_src_buffer;
    std::shared_ptr<Vulkan::Sampler> sampler;
    std::string GetFileExtention(const std::string file);
  public:
    Object() = delete;
    Object(const Object &obj) = delete;
    Object &operator= (const Object &obj) = delete;
    Object(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::CommandPool> pool);
    std::shared_ptr<Vulkan::IBuffer> GetIndexBuffer() const { return index_buffer; }
    std::shared_ptr<Vulkan::IBuffer> GetVertexBuffer() const { return vertex_buffer; }
    std::shared_ptr<Vulkan::Image> GetTextureImage() const { return texture_image; }
    std::shared_ptr<Vulkan::Sampler> GetSampler() const { return sampler; }
    void LoadModel(const std::string model_file_path, const std::string materials_directory = "");
    void LoadTexture(const std::string texture_file_path, const bool enable_mip_levels);
    ~Object()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };
}

#endif