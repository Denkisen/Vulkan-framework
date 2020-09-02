#ifndef __CPU_NW_VULKAN_OBJECT_H
#define __CPU_NW_VULKAN_OBJECT_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <vector>
#include <mutex>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Device.h"
#include "Buffer.h"
#include "BufferArray.h"
#include "Image.h"
#include "CommandPool.h"
#include "Sampler.h"
#include "../libs/ImageBuffer.h"
#include "Vertex.h"
#include "Supply.h"

namespace Vulkan
{
  class Object
  {
  protected:
    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::CommandPool> command_pool;
    std::shared_ptr<Vulkan::Image> texture_image;
    std::shared_ptr<Vulkan::Sampler> sampler;
    std::shared_ptr<Vulkan::BufferArray> dst_buffer_array;

    std::shared_ptr<Vulkan::BufferArray> transformations_buffer;
    std::mutex transformations_buffer_mutex;

    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 up_axis = { 0.0f, 1.0f, 0.0f };
    glm::vec3 direction = { 0.0f, 0.0f, -1.0f };
    std::mutex transformations_mutex;
  public:
    Object() = delete;
    Object(const Object &obj) = delete;
    Object &operator= (const Object &obj) = delete;
    Object(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::CommandPool> pool);
    std::pair<VkBuffer, uint32_t> GetIndexBuffer() const { return {dst_buffer_array->GetWholeBuffer(1).first, dst_buffer_array->ElementsInBuffer<uint32_t>(1)}; }
    std::pair<VkBuffer, uint32_t> GetVertexBuffer() const { return {dst_buffer_array->GetWholeBuffer(0).first, dst_buffer_array->ElementsInBuffer<uint32_t>(0)}; }
    std::shared_ptr<Vulkan::Image> GetTextureImage() const { return texture_image; }
    std::shared_ptr<Vulkan::Sampler> GetSampler() const { return sampler; }
    void LoadModel(const std::string model_file_path, const std::string materials_directory = "");
    void LoadTexture(const std::string texture_file_path, const bool enable_mip_levels);
    template <typename T> void BuildTransformBuffers(const size_t count);
    template <typename T> void UpdateTransformBuffer(const size_t index, const T data);
    std::pair<VkBuffer, VkBufferCopy> CopyTransformBufferInfo(const size_t index);
    glm::mat4 ObjectTransforations();
    void SetPosition(const glm::vec3 pos);
    void SetDirection(const glm::vec3 dir);
    void Move(const glm::vec3 pos_offset);
    void Turn(const float angle);
    ~Object()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };

  template <typename T> 
  void Object::BuildTransformBuffers(const size_t count)
  {
    if (count == 0)
      throw std::runtime_error("Invalid Transform buffers count");

    std::lock_guard<std::mutex> lock(transformations_buffer_mutex);
    transformations_buffer = std::make_shared<Vulkan::BufferArray>(device);

    auto sz = transformations_buffer->CalculateBufferSize(sizeof(T), count, Vulkan::StorageType::Uniform);
    transformations_buffer->DeclareBuffer(sz, Vulkan::HostVisibleMemory::HostVisible, Vulkan::StorageType::Uniform);
    transformations_buffer->SplitBufferOnEqualVirtualBuffers<T>(0);
  }

  template <typename T> 
  void Object::UpdateTransformBuffer(const size_t index, const T data)
  {
    std::lock_guard<std::mutex> lock(transformations_buffer_mutex);

    if (index >= transformations_buffer->VirtualBuffersCount(0))
      throw std::runtime_error("Index out of bounds.");

    transformations_buffer->TrySetValue(0, index, std::vector<T>{data});
  }
}

#endif