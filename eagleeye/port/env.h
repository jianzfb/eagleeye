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

#ifndef _EAGLEEYE_PORT_ENV_H_
#define _EAGLEEYE_PORT_ENV_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <malloc.h>
#endif

#include <sys/stat.h>

namespace eagleeye {
namespace port {

// class MallocLogger {
//  public:
//   MallocLogger() = default;
//   virtual ~MallocLogger() = default;
// };

// class FileSystem;
// class LogWriter;

class Env {
 public:
  virtual int64_t NowMicros(){};
  virtual EagleeyeError AdviseFree(void *addr, size_t length){};
  virtual EagleeyeError GetCPUMaxFreq(std::vector<float> *max_freqs){};
  virtual EagleeyeError SchedSetAffinity(const std::vector<size_t> &cpu_ids){};
  virtual int GetCPUCount(){return 0;};

  virtual std::vector<std::string> GetBackTraceUnsafe(int max_steps){};


  virtual EagleeyeCPU* GetCPUDevice(){return NULL;};
  virtual EagleeyeGPU* GetGPUDevice(){return NULL;};
  static Env *Default();
};
}  // namespace port

inline int64_t NowMicros() {
  return port::Env::Default()->NowMicros();
}

inline EagleeyeError AdviseFree(void *addr, size_t length) {
  return port::Env::Default()->AdviseFree(addr, length);
}

inline EagleeyeError GetCPUMaxFreq(std::vector<float> *max_freqs) {
  return port::Env::Default()->GetCPUMaxFreq(max_freqs);
}

inline EagleeyeError SchedSetAffinity(const std::vector<size_t> &cpu_ids) {
  return port::Env::Default()->SchedSetAffinity(cpu_ids);
}

inline int GetCPUCount() {
  return port::Env::Default()->GetCPUCount();
}

inline EagleeyeError Memalign(void **memptr, size_t alignment, size_t size) {
#ifdef _WIN32
  *memptr = _aligned_malloc(size, alignment);
  if (*memptr == nullptr) {
    return EagleeyeError::EAGLEEYE_OUT_OF_RESOURCES;
  } else {
    return EagleeyeError::EAGLEEYE_NO_ERROR;
  }
#else
#if defined(__ANDROID__) || defined(__hexagon__)
  *memptr = memalign(alignment, size);
  if (*memptr == nullptr) {
    return EagleeyeError::EAGLEEYE_OUT_OF_RESOURCES;
  } else {
    return EagleeyeError::EAGLEEYE_NO_ERROR;
  }
#else
  int error = posix_memalign(memptr, alignment, size);
  if (error != 0) {
    if (*memptr != nullptr) {
      free(*memptr);
      *memptr = nullptr;
    }
    return EagleeyeError::EAGLEEYE_OUT_OF_RESOURCES;
  } else {
    return EagleeyeError::EAGLEEYE_NO_ERROR;
  }
#endif
#endif
}

inline EagleeyeError GetEnv(const char *name, std::string *value) {
#ifdef _WIN32
  char *val;
  size_t len;
  errno_t error = _dupenv_s(&val, &len, name);
  if (error != 0) {
    return EagleeyeError::MACE_RUNTIME_ERROR;
  } else {
    if (val != nullptr) {
      *value = std::string(val);
      free(val);
    }
    return EagleeyeError::EAGLEEYE_NO_ERROR;
  }
#else
  char *val = getenv(name);
  if (val != nullptr) {
    *value = std::string(val);
  }
  return EagleeyeError::EAGLEEYE_NO_ERROR;
#endif
}

#if defined(_WIN32) && !defined(S_ISREG)
#define S_ISREG(m) (((m) & 0170000) == (0100000))
#endif
}  // namespace eagleeye

#endif  // EAGLEEYE_PORT_ENV_H_
