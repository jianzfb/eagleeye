// Copyright 2019 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "eagleeye/port/linux_base/env.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeStr.h"

#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>
#include <errno.h>

namespace eagleeye {
namespace port {

int LinuxBaseEnv::GetCPUCount() {
  int cpu_count = 0;
  std::string cpu_sys_conf = "/proc/cpuinfo";
  std::ifstream f(cpu_sys_conf);
  if (!f.is_open()) {
    EAGLEEYE_LOGE("Failed to open %s.", cpu_sys_conf.c_str());
    return -1;
  }
  std::string line;
  const std::string processor_key = "processor";
  while (std::getline(f, line)) {
    if (line.size() >= processor_key.size()
        && line.compare(0, processor_key.size(), processor_key) == 0) {
      ++cpu_count;
    }
  }
  if (f.bad()) {
    EAGLEEYE_LOGE("Failed to read %s.", cpu_sys_conf.c_str());
  }
  if (!f.eof()) {
    EAGLEEYE_LOGE("Failed to read end of %s.", cpu_sys_conf.c_str());
  }
  f.close();
  return cpu_count;
}

LinuxBaseEnv::LinuxBaseEnv(){
  m_cpu_device = new EagleeyeCPU();
  m_gpu_device = new EagleeyeGPU();
}
LinuxBaseEnv::~LinuxBaseEnv(){
  delete m_cpu_device;
}

int64_t LinuxBaseEnv::NowMicros() {
  return EagleeyeTime::getCurrentTime();
}

// FileSystem *LinuxBaseEnv::GetFileSystem() {
//   return &posix_file_system_;
// }

EagleeyeCPU* LinuxBaseEnv::GetCPUDevice() {
  return m_cpu_device;
}

EagleeyeGPU* LinuxBaseEnv::GetGPUDevice() {
  return m_gpu_device;
}

EagleeyeError LinuxBaseEnv::GetCPUMaxFreq(std::vector<float> *max_freqs) {
  // 获得CPU频率
  EAGLEEYE_CHECK_NOTNULL(max_freqs);
  int cpu_count = this->GetCPUCount();
  if (cpu_count < 0) {
    return EagleeyeError::EAGLEEYE_RUNTIME_ERROR;
  }
  for (int cpu_id = 0; cpu_id < cpu_count; ++cpu_id) {
    std::string cpuinfo_max_freq_sys_conf = makeString(
        "/sys/devices/system/cpu/cpu",
        cpu_id,
        "/cpufreq/cpuinfo_max_freq");
    std::ifstream f(cpuinfo_max_freq_sys_conf);
    if (!f.is_open()) {
      EAGLEEYE_LOGE( "Failed to open %s.",cpuinfo_max_freq_sys_conf.c_str());
      return EagleeyeError::EAGLEEYE_RUNTIME_ERROR;
    }
    std::string line;
    if (std::getline(f, line)) {
      float freq = strtof(line.c_str(), nullptr);
      max_freqs->push_back(freq);
    }
    if (f.bad()) {
      EAGLEEYE_LOGE("Failed to read %s.",cpuinfo_max_freq_sys_conf.c_str());
    }
    f.close();
  }

  return EagleeyeError::EAGLEEYE_NO_ERROR;
}

EagleeyeError LinuxBaseEnv::SchedSetAffinity(const std::vector<size_t> &cpu_ids) {
  // 设置线程亲和性
  cpu_set_t mask;
  CPU_ZERO(&mask);
  for (auto cpu_id : cpu_ids) {
    CPU_SET(cpu_id, &mask);
  }

  pid_t pid = syscall(SYS_gettid);
  int err = sched_setaffinity(pid, sizeof(mask), &mask);
  if (err) {
    EAGLEEYE_LOGE("Fail to Set affinity.");
    return EagleeyeError::EAGLEEYE_ARG_ERROR;
  }

  return EagleeyeError::EAGLEEYE_NO_ERROR;
}

EagleeyeError LinuxBaseEnv::AdviseFree(void *addr, size_t length) {
  // 它就通过系统接口madvice和MADV_DONTNEED标志告诉内核，刚刚分配出去的内存在近期内不会使用，内核可以该内存对应的物理页回收。
  int page_size = sysconf(_SC_PAGESIZE);
  void *addr_aligned =
      reinterpret_cast<void *>(
          (reinterpret_cast<uintptr_t>(addr) + page_size - 1)
              & (~(page_size - 1)));
  uintptr_t delta =
      reinterpret_cast<uintptr_t>(addr_aligned)
          - reinterpret_cast<uintptr_t>(addr);
  if (length >= delta + page_size) {
    size_t len_aligned = (length - delta) & (~(page_size - 1));
    int error = madvise(addr_aligned, len_aligned, MADV_DONTNEED);
    if (error != 0) {
      EAGLEEYE_LOGE("Fail to Advise free.");
      return EagleeyeError::EAGLEEYE_RUNTIME_ERROR;
    }
  }
  return EagleeyeError::EAGLEEYE_NO_ERROR;
}

#if defined(__linux__)
Env *Env::Default() {
  static LinuxBaseEnv linux_env;
  return &linux_env;
}
#endif
}  // namespace port
}  // namespace eagleeye
