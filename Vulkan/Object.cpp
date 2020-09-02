#include "Object.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tiny_obj_loader.h"

#include <unordered_map>

namespace Vulkan
{
  Object::Object(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::CommandPool> pool)
  {
    device = dev;
    command_pool = pool;
    dst_buffer_array = std::make_shared<Vulkan::BufferArray>(device);
  }

  void Object::LoadModel(const std::string model_file_path, const std::string materials_directory)
  {
    auto model_ext = Supply::GetFileExtention(model_file_path);
    if (model_ext != ".obj")
      throw std::runtime_error("Unsupported format");

    std::vector<uint32_t> indices;
    std::unordered_map<Vulkan::Vertex, uint32_t> vertices;
    std::vector<Vulkan::Vertex> vertices_buff;
    Vulkan::BufferArray src_buffer_array(device);

    if (model_ext == ".obj")
    {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;
      std::string warn, err;

      if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_file_path.c_str(), materials_directory.c_str())) 
        throw std::runtime_error(warn + err);

      for (const auto& shape : shapes) 
      {
        for (const auto& index : shape.mesh.indices) 
        {
          Vulkan::Vertex vertex = {};
          vertex.pos = 
          {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2]
          };

          vertex.normal =
          {
            attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
          };

          vertex.texCoord = 
          {
            attrib.texcoords[2 * index.texcoord_index + 0],
            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
          };

          vertex.color = {1.0f, 1.0f, 1.0f};

          if (vertices.count(vertex) == 0)
          {
            vertices[vertex] = uint32_t (vertices_buff.size());
            vertices_buff.push_back(vertex);
          }

          indices.push_back(vertices[vertex]);
        }
      }
    }

    src_buffer_array.DeclareBuffer((vertices_buff.size() * sizeof(Vertex)) + (indices.size() * sizeof(uint32_t)), HostVisibleMemory::HostVisible, Vulkan::StorageType::Storage);
    src_buffer_array.DeclareVirtualBuffer(0, 0, vertices_buff.size() * sizeof(Vertex));
    src_buffer_array.DeclareVirtualBuffer(0, vertices_buff.size() * sizeof(Vertex), indices.size() * sizeof(uint32_t));

    dst_buffer_array->DeclareBuffer(vertices_buff.size() * sizeof(Vertex), HostVisibleMemory::HostInvisible, Vulkan::StorageType::Vertex);
    dst_buffer_array->DeclareBuffer(indices.size() * sizeof(uint32_t), HostVisibleMemory::HostInvisible, Vulkan::StorageType::Index);

    src_buffer_array.TrySetValue(0, 0, vertices_buff);
    src_buffer_array.TrySetValue(0, 1, indices);

    VkBufferCopy copy_region = {};
    
    auto buffer_lock = command_pool->OrderBufferLock();
    command_pool->BeginCommandBuffer(buffer_lock);

    auto bf = src_buffer_array.GetVirtualBuffer(0, 0);
    copy_region.srcOffset = bf.second.offset;
    copy_region.size = bf.second.size;
    copy_region.dstOffset = 0;
    command_pool->CopyBuffer(buffer_lock, bf.first, dst_buffer_array->GetWholeBuffer(0).first, {copy_region});

    bf = src_buffer_array.GetVirtualBuffer(0, 1);
    copy_region.srcOffset = bf.second.offset;
    copy_region.size = bf.second.size;
    copy_region.dstOffset = 0;
    command_pool->CopyBuffer(buffer_lock, bf.first, dst_buffer_array->GetWholeBuffer(1).first, {copy_region});

    command_pool->EndCommandBuffer(buffer_lock);
    command_pool->ExecuteBuffer(buffer_lock);
    command_pool->ReleaseBufferLock(buffer_lock);
  }

  void Object::LoadTexture(const std::string texture_file_path, const bool enable_mip_levels)
  {
    ImageBuffer texture_loader;
    texture_loader.Load(texture_file_path);
    size_t tex_w = enable_mip_levels ? ((size_t) texture_loader.Width() * 2) / 3 : (size_t) texture_loader.Width();
    size_t tex_h = (size_t) texture_loader.Height();
    texture_image = std::make_shared<Vulkan::Image>(device, tex_w, tex_h, enable_mip_levels, 
                                                    Vulkan::ImageTiling::Optimal, 
                                                    Vulkan::HostVisibleMemory::HostInvisible,
                                                    Vulkan::ImageType::Sampled,
                                                    Vulkan::ImageFormat::SRGB_8,
                                                    VK_SAMPLE_COUNT_1_BIT);
    std::shared_ptr<Vulkan::Buffer<uint8_t>> texture_src_buffer = std::make_shared<Vulkan::Buffer<uint8_t>>(device, Vulkan::StorageType::Storage, 
                                                                  Vulkan::HostVisibleMemory::HostVisible);

    *texture_src_buffer = enable_mip_levels ? texture_loader.GetMipLevelsBuffer() : texture_loader.Canvas();
    sampler = std::make_shared<Vulkan::Sampler>(device, texture_image->GetMipLevels());

    std::vector<VkBufferImageCopy> image_regions(texture_image->GetMipLevels());

    uint32_t buffer_offset = 0;
    for (size_t i = 0; i < image_regions.size(); ++i)
    {
      image_regions[i].bufferOffset = buffer_offset;
      image_regions[i].bufferRowLength = (uint32_t) tex_w;
      image_regions[i].bufferImageHeight = (uint32_t) tex_h;

      image_regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_regions[i].imageSubresource.mipLevel = i;
      image_regions[i].imageSubresource.baseArrayLayer = 0;
      image_regions[i].imageSubresource.layerCount = 1;

      image_regions[i].imageOffset = {0, 0, 0};
      image_regions[i].imageExtent = 
      {
        (uint32_t) tex_w,
        (uint32_t) tex_h,
        1
      };

      buffer_offset += tex_w * tex_h * texture_image->Channels();      
      if (tex_w > 1) tex_w /= 2;
      if (tex_h > 1) tex_h /= 2;
    }

    auto buffer_lock = command_pool->OrderBufferLock();
    command_pool->BeginCommandBuffer(buffer_lock);
    command_pool->TransitionImageLayout(buffer_lock, texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_pool->CopyBufferToImage(buffer_lock, texture_src_buffer, texture_image, image_regions);
    command_pool->TransitionImageLayout(buffer_lock, texture_image, texture_image->GetLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    command_pool->EndCommandBuffer(buffer_lock);
    command_pool->ExecuteBuffer(buffer_lock);
    command_pool->ReleaseBufferLock(buffer_lock);
  }

  std::pair<VkBuffer, VkBufferCopy> Object::CopyTransformBufferInfo(const size_t index)
  {
    std::lock_guard<std::mutex> lock(transformations_buffer_mutex);

    if (index >= transformations_buffer->VirtualBuffersCount(0))
      throw std::runtime_error("Index out of bounds.");
    
    auto sub = transformations_buffer->GetVirtualBuffer(0, index);
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = sub.second.offset;
    copy_region.size = sub.second.size;
    copy_region.dstOffset = 0;

    return { sub.first, copy_region };
  }

  void Object::SetPosition(const glm::vec3 pos)
  {
    std::lock_guard<std::mutex> lock(transformations_mutex);
    position = pos;
  }

  void Object::Move(const glm::vec3 pos_offset)
  {
    std::lock_guard<std::mutex> lock(transformations_mutex);
    position += pos_offset;
  }

  void Object::SetDirection(const glm::vec3 dir)
  {
    std::lock_guard<std::mutex> lock(transformations_mutex);
    direction = dir;
  }

  void Object::Turn(const float angle)
  {
    std::lock_guard<std::mutex> lock(transformations_mutex);
    direction = glm::rotateY(direction, glm::radians(angle));
  }

  glm::mat4 Object::ObjectTransforations()
  {
    std::lock_guard<std::mutex> lock(transformations_mutex);
    glm::mat4 result = glm::mat4(1.0f);
    result = glm::translate(glm::mat4(1.0f), position);
    result = glm::rotate(result, glm::orientedAngle(glm::normalize(direction), glm::vec3(0.0f, 0.0f, -1.0f), up_axis), up_axis);
    return result;
  }
}