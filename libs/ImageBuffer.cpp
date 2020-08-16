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
  if (image.empty())
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
  std::vector<uint8_t> result(image.cols * image.rows * image.channels());
  std::copy(image.datastart, image.dataend, result.begin());
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
