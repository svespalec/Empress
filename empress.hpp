/*
 * MIT License
 *
 * Copyright (c) 2024 Samuel Vespalec
 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#pragma once

#include <Windows.h>
#include <iostream>
#include <format>

namespace empress {
  namespace logging {
    enum class log_level {
      info,
      warning,
      error
    };

    /**
     * @brief Logs a formatted message with specified severity level
     * 
     * @tparam Args: Variadic template for format arguments
     * @param level: Severity level of the message
     * @param fmt: Format string
     * @param args: Format arguments
     */
    template<typename... Args>
    void log(log_level level, std::format_string<Args...> fmt, Args&&... args) {
      const char* prefix = "";

      switch (level) {
        case log_level::info: prefix = "[INFO] "; break;
        case log_level::warning: prefix = "[WARN] "; break;
        case log_level::error: prefix = "[ERROR] "; break;
      }

      std::cout << prefix << std::format(fmt, std::forward<Args>(args)...) << std::endl;
    }
  }

  class protection {
  private:
    // NT API function types
    typedef LONG NTSTATUS;
       
    typedef NTSTATUS(NTAPI* NtCreateJobObject_t)(PHANDLE, ACCESS_MASK, PVOID);
    typedef NTSTATUS(NTAPI* NtAssignProcessToJobObject_t)(HANDLE, HANDLE);
    typedef NTSTATUS(NTAPI* NtSetInformationJobObject_t)(HANDLE, JOBOBJECTINFOCLASS, PVOID, ULONG);

    #define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
    #define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)

    /**
     * @brief Gets a function pointer to an NT API
     * 
     * @tparam T: Function pointer type
     * @param name: Name of the NT API function
     * @return Function: pointer or nullptr if not found
     */
    template<typename T>
    static T get_nt_api(const char* name) {
      static const auto ntdll = GetModuleHandleA("ntdll.dll");
      return reinterpret_cast<T>(GetProcAddress(ntdll, name));
    }

    enum class job_status {
      success = 0,
      job_create_failed,
      process_assign_failed,
      limit_setting_failed
    };

  public:
    /**
     * @brief Enables memory protection using job object restrictions
     * 
     * Sets a minimal memory limit (0x1000) that allows the current process
     * to continue running but prevents debuggers from:
     * - Allocating memory for their operations
     * - Creating threads for debug event handling
     * - Modifying memory for breakpoints
     * 
     * @return true if protection was successfully enabled
     * @return false if any step failed
     */
    static bool enable() {
      const auto NtCreateJobObject = get_nt_api<NtCreateJobObject_t>("NtCreateJobObject");
      const auto NtAssignProcessToJobObject = get_nt_api<NtAssignProcessToJobObject_t>("NtAssignProcessToJobObject");
      const auto NtSetInformationJobObject = get_nt_api<NtSetInformationJobObject_t>("NtSetInformationJobObject");

      if (!NtCreateJobObject || !NtAssignProcessToJobObject || !NtSetInformationJobObject) {
        logging::log(logging::log_level::error, "Failed to get NT API functions");
        return false;
      }

      // Create job object
      HANDLE job = nullptr;
      NTSTATUS status = NtCreateJobObject(&job, MAXIMUM_ALLOWED, nullptr);
      
      if (!NT_SUCCESS(status)) {
        logging::log(logging::log_level::error, "Failed to create job object");
        return false;
      }

      // Assign current process to job
      status = NtAssignProcessToJobObject(job, NtCurrentProcess());

      if (!NT_SUCCESS(status)) {
        CloseHandle(job);
        logging::log(logging::log_level::error, "Failed to assign process to job");
        return false;
      }

      // Set minimal memory limit to prevent memory modifications
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
      limits.ProcessMemoryLimit = 0x1000;  // Minimal memory limit
      limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;

      status = NtSetInformationJobObject(
        job,
        JobObjectExtendedLimitInformation,
        &limits,
        sizeof(limits)
      );

      if (!NT_SUCCESS(status)) {
        CloseHandle(job);
        logging::log(logging::log_level::error, "Failed to set job limits");
        return false;
      }

      logging::log(logging::log_level::info, "Memory protection active");
      return true;
    }
  };
}
