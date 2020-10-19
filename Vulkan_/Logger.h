#ifndef __VULKAN_LOGGER_H
#define __VULKAN_LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>

namespace Vulkan
{
  class Logger
  {
  private:
    static std::ofstream file;
    static std::string file_name;
    static bool cout_echo;
    static bool file_echo;
    static std::mutex echo_mutex;
    static void PrepOutput();
  public:
    Logger() = delete;
    Logger(const Logger &obj) = delete;
    Logger &operator=(const Logger &obj) = delete;
    static void UseCout(const bool enable);
    static void UseFile(const bool enable);
    static void SetFile(const std::string file_path);
    static void EchoInfo(std::string text, std::string func_name = "");
    static void EchoWarning(std::string text, std::string func_name = "");
    static void EchoError(std::string text, std::string func_name = "");
    static void EchoDebug(std::string text, std::string func_name = "");
    ~Logger();
  };
}

#endif