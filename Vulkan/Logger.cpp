#include "Logger.h"

namespace Vulkan
{
  std::string Logger::file_name = "log.txt";
  bool Logger::cout_echo = true;
  bool Logger::file_echo = true;
  std::mutex Logger::echo_mutex;
  std::ofstream Logger::file;

  void Logger::UseCout(const bool enable) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    cout_echo = true;
  }

  void Logger::UseFile(const bool enable) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    file_echo = true;
  }

  void Logger::SetFile(const std::string file_path) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    if (file.is_open()) file.close();

    file_name = file_path;
  }

  void Logger::PrepOutput()
  {
    if (file_echo && !file.is_open())
    {
      file.open(file_name, std::ios::app);
    }
  }

  void Logger::EchoInfo(std::string text, std::string func_name) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    std::string str = "Info";
    if (func_name != "") str += ": " + func_name;
    if (text != "") str += ": " + text;
    str += ";";

    try
    {
      PrepOutput();

      if (file_echo && file.is_open())
      {
        file << str << std::endl;
        file.flush();
      }

      if (cout_echo)
        std::cout << str << std::endl;      
    }
    catch(...) { }
  }

  void Logger::EchoWarning(std::string text, std::string func_name) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    std::string str = "Warning";
    if (func_name != "") str += ": " + func_name;
    if (text != "") str += ": " + text;
    str += ";";

    try
    {
      PrepOutput();

      if (file_echo && file.is_open())
        file << str << std::endl;

      if (cout_echo)
        std::cout << str << std::endl;      
    }
    catch(...) { }
  }

  void Logger::EchoError(std::string text, std::string func_name) noexcept
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    std::string str = "Error";
    if (func_name != "") str += ": " + func_name;
    if (text != "") str += ": " + text;
    str += ";";

    try
    {
      PrepOutput();

      if (file_echo && file.is_open())
        file << str << std::endl;

      if (cout_echo)
        std::cout << str << std::endl;      
    }
    catch(...) { }
  }

  void Logger::EchoDebug(std::string text, std::string func_name) noexcept
  {
#ifdef DEBUG
    std::lock_guard<std::mutex> lock(echo_mutex);
    std::string str = "Debug";
    if (func_name != "") str += ": " + func_name;
    if (text != "") str += ": " + text;
    str += ";";

    try
    {
      PrepOutput();

      if (file_echo && file.is_open())
        file << str << std::endl;

      if (cout_echo)
        std::cout << str << std::endl;      
    }
    catch(...) { }
#endif
  }

  Logger::~Logger()
  {
    std::lock_guard<std::mutex> lock(echo_mutex);
    try
    {
      if (file.is_open()) file.close();
    }
    catch(...) { }
  }
}