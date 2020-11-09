#include "Surface.h"
#include "Logger.h"
#include "Instance.h"

namespace Vulkan
{
  Surface_impl::~Surface_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (surface != VK_NULL_HANDLE)
    {
      vkDestroySurfaceKHR(Instance::GetInstance(), surface, nullptr);
      surface = VK_NULL_HANDLE;
    }

    if (window != nullptr)
    {
      glfwDestroyWindow(window);
      window = nullptr;
    }
    glfwTerminate();
  }

  Surface_impl::Surface_impl(const SurfaceConfig &params) noexcept
  {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(params.widght, params.height, params.app_title.c_str(), nullptr, nullptr);

    if (window == nullptr)
    {
      Logger::EchoError("Can't create window", __func__);
      return;
    }

    glfwSetWindowUserPointer(window, this);

    auto er = glfwCreateWindowSurface(Instance::GetInstance(), window, nullptr, &surface);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create surface", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      glfwDestroyWindow(window);
      window = nullptr;
      surface = VK_NULL_HANDLE;
      return;
    }
  }

  std::pair<int32_t, int32_t> Surface_impl::GetFramebufferSize() const noexcept
  {
    std::pair<int32_t, int32_t> res;
    glfwGetFramebufferSize(window, &res.first, &res.second);
    return res;
  }

  Surface &Surface::operator=(Surface &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Surface::swap(Surface &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Surface &lhs, Surface &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}