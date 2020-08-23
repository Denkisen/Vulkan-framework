#include "ImageBuffer.h"

#include <omp.h>

ImageBuffer::ImageBuffer(const ImageBuffer &img)
{
  image = img.image;
}

ImageBuffer::ImageBuffer(std::string file_path, const int channals)
{
  Load(file_path, channals);
}

ImageBuffer::ImageBuffer(const size_t w, const size_t h, const int channals)
{
  if (channals <= 0 || channals > 4 || channals == 2)
    std::runtime_error("channels has invalid value");
  image = cv::Mat::zeros(cv::Size(w, h), CV_8UC(channals));
}

ImageBuffer& ImageBuffer::operator=(const ImageBuffer &img)
{
  image = img.image;
  return *this;
}

uint8_t& ImageBuffer::operator[](const size_t index)
{
  if (image.empty())
    throw std::runtime_error("An image is empty.");
  if (index >= (size_t) (image.dataend - image.datastart))
    throw std::runtime_error("An index is out of range.");
  
  return image.data[index];
}

void ImageBuffer::Load(std::string file_path, const int channals)
{
  image = cv::imread(file_path);
  SetChannels(channals);
  if (image.empty() || image.rows <= 0 || image.cols <= 0)
    throw std::runtime_error("An image is empty.");
}

void ImageBuffer::Save(std::string file_path, const int channals)
{
  if (image.empty())
    throw std::runtime_error("An image is empty.");
  SetChannels(channals);
  cv::imwrite(file_path, image);
}

void ImageBuffer::Resize(int w, int h)
{
  if (image.empty())
    throw std::runtime_error("An image is empty.");
  cv::Mat res;
  cv::Size s(w, h);
  cv::resize(image, res, s, 0.0, 0.0, cv::INTER_LINEAR);
  image = res;
}

std::vector<uint8_t> ImageBuffer::Canvas() const
{
  if (image.empty())
    throw std::runtime_error("An image is empty.");
  std::vector<uint8_t> result(Width() * Height() * Channels());
  std::copy(image.datastart, image.dataend, result.begin());
  return result;
}

std::vector<uint8_t> ImageBuffer::GetMipLevelsBuffer() const
{
  size_t tex_w = (size_t) (Width() * 2) / 3;
  size_t tex_h = (size_t) Height();
  size_t mip_levels = (size_t) std::floor(std::log2(std::max(tex_w, tex_h))) + 1;

  size_t mip_h = tex_h;
  for (size_t i = 1; i < mip_levels; ++i)
  {
    if (tex_h > 1) tex_h /= 2;
    mip_h += tex_h;
  }

  std::vector<uint8_t> result(tex_w * mip_h * Channels());
  auto it = result.begin();
  tex_h = (size_t) Height();
  mip_h = 0;
  size_t mip_w = tex_w;

  cv::Mat tmp = image(cv::Rect(0, 0, tex_w, tex_h));
  
  if (tmp.isContinuous())
    it = std::copy(tmp.data, (tmp.data + (tmp.total() * tmp.channels())), it);
  else
    for (size_t i = 0; i < tmp.rows; ++i)
      it = std::copy(tmp.ptr<uint8_t>(i), (tmp.ptr<uint8_t>(i) + (tmp.cols * tmp.channels())), it);
  
  tmp.release();

  for (size_t i = 1; i < mip_levels; ++i)
  {
    if (tex_h > 1) tex_h /= 2;
    if (tex_w > 1) tex_w /= 2;
    tmp = image(cv::Rect(mip_w, mip_h, tex_w, tex_h));

    if (tmp.isContinuous())
      it = std::copy(tmp.data, (tmp.data + (tmp.total() * tmp.channels())), it);
    else
      for (size_t i = 0; i < tmp.rows; ++i)
        it = std::copy(tmp.ptr<uint8_t>(i), (tmp.ptr<uint8_t>(i) + (tmp.cols * tmp.channels())), it);

    tmp.release();
    mip_h += tex_h;
  }

  return result;
}

ImageBuffer ImageBuffer::SubImage(int x, int y, int w, int h)
{  
  if (image.empty())
    throw std::runtime_error("An image is empty.");

  ImageBuffer res;
  res.image = image(cv::Rect(x, y, w, h));
  return res;
}

void ImageBuffer::SetChannels(const int channals)
{
  if (image.empty())
    throw std::runtime_error("An image is empty.");
  if (channals <= 0 || channals > 4 || channals == 2)
    throw std::runtime_error("channels has invalid value");
  if (image.channels() != channals)
  {
    cv::Mat dest;
    if (channals == 1)
      cv::cvtColor(image, dest, cv::COLOR_RGB2GRAY, channals);
    else if (channals == 3 && image.channels() == 4)
      cv::cvtColor(image, dest, cv::COLOR_RGBA2RGB, channals);
    else if (channals == 4 && image.channels() == 3)
      cv::cvtColor(image, dest, cv::COLOR_RGB2RGBA, channals);
    else if (channals == 4 && image.channels() == 1)
      cv::cvtColor(image, dest, cv::COLOR_GRAY2RGBA, channals);
    else if (channals == 3 && image.channels() == 1)
      cv::cvtColor(image, dest, cv::COLOR_GRAY2RGB, channals);
    image = dest;
  }
}
