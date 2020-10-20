#ifndef __VULKAN_SURFACE_H
#define __VULKAN_SURFACE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <iostream>

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
    SurfaceConfig() = default;
    ~SurfaceConfig() = default;
    SurfaceConfig &SetAppTitle(const std::string text) { app_title = text; return *this; }
    SurfaceConfig &SetWidght(const int32_t w) { widght = w; return *this; }
    SurfaceConfig &SetHeight(const int32_t h) { height = h; return *this; }
  };

  class Surface_impl
  {
  public:
    ~Surface_impl();
    Surface_impl() = delete;
    Surface_impl(const Surface_impl &obj) = delete;
    Surface_impl(Surface_impl &&obj) = delete;
    Surface_impl &operator=(const Surface_impl &obj) = delete;
    Surface_impl &operator=(Surface_impl &&obj) = delete;
  private:
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GLFWwindow *window = nullptr;

    friend class Surface;

    Surface_impl(const SurfaceConfig &params);
    VkSurfaceKHR GetSurface() { return surface; }
    GLFWwindow *GetWindow() { return window; }
    void WaitEvents() { glfwWaitEvents(); }
    void PollEvents() { glfwPollEvents(); }
    void SetKeyCallback(GLFWkeyfun callback) { glfwSetKeyCallback(window, callback); }
    void SetFramebufferSizeCallback(GLFWframebuffersizefun callback) { glfwSetFramebufferSizeCallback(window, callback); }
    void SetWindowUserPointer(void *p) { glfwSetWindowUserPointer(window, p); }
    void *GetWindowUserPointer() { return glfwGetWindowUserPointer(window); }
    std::pair<int32_t, int32_t> GetFramebufferSize();
    VkBool32 IsWindowShouldClose() { return glfwWindowShouldClose(window); }
  };

  class Surface
  {
  private:
    std::unique_ptr<Surface_impl> impl;
  public:
    Surface() = delete;
    Surface(const Surface &obj) = delete;
    Surface &operator=(const Surface &obj) = delete;
    ~Surface() = default;
    Surface(Surface &&obj) noexcept : impl(std::move(obj.impl)) {};
    Surface(const SurfaceConfig &params) : impl(std::unique_ptr<Surface_impl>(new Surface_impl(params))) {};
    Surface &operator=(Surface &&obj);
    void swap(Surface &obj) noexcept;

    VkSurfaceKHR GetSurface() { return impl->GetSurface(); }
    GLFWwindow *GetWindow() { return impl->GetWindow(); }
    void WaitEvents() { impl->WaitEvents(); }
    void PollEvents() { impl->PollEvents(); }
    void SetKeyCallback(GLFWkeyfun callback) { impl->SetKeyCallback(callback); }
    void SetFramebufferSizeCallback(GLFWframebuffersizefun callback) { impl->SetFramebufferSizeCallback(callback); }
    void SetWindowUserPointer(void *p) { impl->SetWindowUserPointer(p); }
    void *GetWindowUserPointer() { return impl->GetWindowUserPointer(); }
    std::pair<int32_t, int32_t> GetFramebufferSize() { return impl->GetFramebufferSize(); }
    VkBool32 IsWindowShouldClose() { return impl->IsWindowShouldClose(); }
  };

  void swap(Surface &lhs, Surface &rhs) noexcept;
}


#endif