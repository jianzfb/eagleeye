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

#ifndef _EAGLEEYE_PORT_LINUX_BASE_ENV_H_
#define _EAGLEEYE_PORT_LINUX_BASE_ENV_H_

#include <vector>

#include "eagleeye/port/env.h"
#include "eagleeye/runtime/cpu/cpu_runtime.h"

namespace eagleeye {
namespace port {

typedef enum {
  kAPPLE = 0,
  kA35 = 35,
  kA53 = 53,
  kA55 = 55,
  kA57 = 57,
  kA72 = 72,
  kA73 = 73,
  kA75 = 75,
  kA76 = 76,
  kA77 = 77,
  kA78 = 78,
  kARMArch_UNKOWN = -1
} ARMArch;


class LinuxBaseEnv : public Env {
public:
  LinuxBaseEnv();
  virtual ~LinuxBaseEnv();


  int64_t NowMicros() override;
  EagleeyeError AdviseFree(void *addr, size_t length) override;
  EagleeyeError GetCPUMaxFreq(std::vector<float> *max_freqs) override;
  // FileSystem *GetFileSystem() override;
  EagleeyeError SchedSetAffinity(const std::vector<size_t> &cpu_ids) override;
  virtual EagleeyeCPU* GetCPUDevice() override;
  virtual EagleeyeGPU* GetGPUDevice() override;
  virtual int GetCPUCount() override;

protected:
  // PosixFileSystem posix_file_system_;
  EagleeyeCPU* m_cpu_device;
  EagleeyeGPU* m_gpu_device;
};

}  // namespace port
}  // namespace eagleeye

#endif  // _EAGLEEYE_PORT_LINUX_BASE_ENV_H_
