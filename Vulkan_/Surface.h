#ifndef __VULKAN_SURFACE_H
#define __VULKAN_SURFACE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Vulkan
{
  class SurfaceConfig
  {
  private:
    friend class Surface_impl;
    std::string app_title = "Application";
    int32_t widght = 1024;
    int32_t height = 768;
  public:
    SurfaceConfig() noexcept = default;
    ~SurfaceConfig() noexcept = default;
    auto &SetAppTitle(const std::string text) noexcept { try { app_title = text; } catch(...) {} return *this; }
    auto &SetWidght(const int32_t w) noexcept { widght = w; return *this; }
    auto &SetHeight(const int32_t h) noexcept { height = h; return *this; }
  };

  class Surface_impl
  {
  public:
    ~Surface_impl() noexcept;
    Surface_impl() = delete;
    Surface_impl(const Surface_impl &obj) = delete;
    Surface_impl(Surface_impl &&obj) = delete;
    Surface_impl &operator=(const Surface_impl &obj) = delete;
    Surface_impl &operator=(Surface_impl &&obj) = delete;
  private:
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GLFWwindow *window = nullptr;

    friend class Surface;

    Surface_impl(const SurfaceConfig &params) noexcept;
    VkSurfaceKHR GetSurface() const noexcept { return surface; }
    GLFWwindow *GetWindow() const noexcept { return window; }
    void WaitEvents() noexcept { glfwWaitEvents(); }
    void PollEvents() noexcept { glfwPollEvents(); }
    void SetKeyCallback(GLFWkeyfun callback) noexcept { glfwSetKeyCallback(window, callback); }
    void SetFramebufferSizeCallback(GLFWframebuffersizefun callback) noexcept { glfwSetFramebufferSizeCallback(window, callback); }
    void SetWindowUserPointer(void *p) noexcept { glfwSetWindowUserPointer(window, p); }
    void *GetWindowUserPointer() noexcept { return glfwGetWindowUserPointer(window); }
    std::pair<int32_t, int32_t> GetFramebufferSize() const noexcept;
    VkBool32 IsWindowShouldClose() noexcept { return glfwWindowShouldClose(window); }
  };

  class Surface
  {
  private:
    std::unique_ptr<Surface_impl> impl;
  public:
    Surface() = delete;
    Surface(const Surface &obj) = delete;
    Surface &operator=(const Surface &obj) = delete;
    ~Surface() noexcept = default;
    Surface(Surface &&obj) noexcept : impl(std::move(obj.impl)) {};
    Surface(const SurfaceConfig &params) noexcept : impl(std::unique_ptr<Surface_impl>(new Surface_impl(params))) {};
    Surface &operator=(Surface &&obj) noexcept;
    void swap(Surface &obj) noexcept;

    VkSurfaceKHR GetSurface() const noexcept { if (impl.get()) return impl->GetSurface(); return VK_NULL_HANDLE; }
    GLFWwindow *GetWindow() const noexcept { if (impl.get()) return impl->GetWindow(); return nullptr; }
    void WaitEvents() noexcept { if (impl.get()) impl->WaitEvents(); }
    void PollEvents() noexcept { if (impl.get()) impl->PollEvents(); }
    void SetKeyCallback(GLFWkeyfun callback) noexcept { if (impl.get()) impl->SetKeyCallback(callback); }
    void SetFramebufferSizeCallback(GLFWframebuffersizefun callback) noexcept { if (impl.get()) impl->SetFramebufferSizeCallback(callback); }
    void SetWindowUserPointer(void *p) noexcept { if (impl.get()) impl->SetWindowUserPointer(p); }
    void *GetWindowUserPointer() noexcept { if (impl.get()) return impl->GetWindowUserPointer(); return nullptr; }
    std::pair<int32_t, int32_t> GetFramebufferSize() const noexcept { if (impl.get()) return impl->GetFramebufferSize(); return {0, 0};}
    VkBool32 IsWindowShouldClose() noexcept { if (impl.get()) return impl->IsWindowShouldClose(); return VK_TRUE; }
    bool IsValid() const noexcept { return impl.get() && impl->surface != VK_NULL_HANDLE; }
  };

  void swap(Surface &lhs, Surface &rhs) noexcept;
}


#endif