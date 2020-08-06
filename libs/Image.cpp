#include "Image.h" 

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "Stb/stb_image.h"
#include "Stb/stb_image_write.h"
#include "Stb/stb_image_resize.h"
// #include <opencv.hpp>
#include <omp.h>

Image::Image(const Image &img)
{
  if (width <= 0 || height <= 0 || bpp <= 0)
    return;
  width = img.width;
  height = img.height;
  bpp = img.bpp;
  canva = img.canva;
}

Image::Image(std::string file_path)
{
  Open(file_path);
}

Image::Image(const int w, const int h, const int bpp)
{
  width = w;
  height = h;
  this->bpp = bpp;
  canva.resize(width * height * bpp);
}

Image& Image::operator=(const Image &img)
{
  Close();
  if (width <= 0 || height <= 0 || bpp <= 0)
    return *this;
  
  width = img.width;
  height = img.height;
  bpp = img.bpp;
  canva = img.canva;
  return *this;
}

void Image::Open(std::string file_path)
{
  Close();
  uint8_t *tmp = stbi_load(file_path.c_str(), &width, &height, &bpp, 0);
  if (width <= 0 || height <= 0 || bpp <= 0)
  {
    if (tmp != nullptr)
      stbi_image_free(tmp);
    throw std::runtime_error("An image file has invalid dimensions.");
  }

  canva.resize(width * height * bpp);

  std::copy(tmp, &tmp[canva.size()], canva.begin());
  stbi_image_free(tmp);
}

void Image::Close()
{
  width = 0;
  height = 0;
  bpp = 0;
  canva.clear();
}

void Image::Save(std::string file_path)
{
  if (canva.empty() || width <= 0 || height <= 0 || bpp <= 0)
    throw std::runtime_error("An image is empty or has invalid dimensions.");
  
  if(file_path.empty())
    throw std::runtime_error("Invalid file path.");

  if (file_path.find(".png") != std::string::npos)
  {
    if (!stbi_write_png(file_path.c_str(), width, height, bpp, canva.data(), width * bpp))
      throw std::runtime_error("Can't save an image.");
  }
  else if (file_path.find(".jpg") != std::string::npos)
  {
    if (!stbi_write_jpg(file_path.c_str(), width, height, bpp, canva.data(), 100))
      throw std::runtime_error("Can't save an image.");
  }
  else if (file_path.find(".bmp") != std::string::npos)
  {
    if (!stbi_write_bmp(file_path.c_str(), width, height, bpp, canva.data()))
      throw std::runtime_error("Can't save an image.");
  }
  else
  {
    file_path += ".bmp";
    if (!stbi_write_bmp(file_path.c_str(), width, height, bpp, canva.data()))
      throw std::runtime_error("Can't save an image.");
  }
}

void Image::Resize(int w, int h)
{
  if (w <= 0 || h <= 0)
    throw std::runtime_error("Invalid new dimensions.");
  
  std::vector<uint8_t> result(canva.size(), 0);
  if (!stbir_resize_uint8(canva.data(), width, height, width * bpp, result.data(), w, h, w * bpp, bpp))
    throw std::runtime_error("Can't resize an image.");
  canva = result;
}

Image Image::CutRectangle(int x, int y, int w, int h)
{
  Image result(w, h, bpp);

  #pragma omp parallel for
  for (int i = 0; i < h; ++i)
  {
    size_t index = (width * bpp * (i + y)) + (x * bpp);
    std::copy(&canva[index], &canva[index + (w * bpp)], &result[i * (w * bpp)]);
  }

  return result;
}