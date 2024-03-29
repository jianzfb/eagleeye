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

#ifndef _EAGLEEYE_PORT_ANDROID_ENV_H_
#define _EAGLEEYE_PORT_ANDROID_ENV_H_

#include <memory>
#include <string>
#include <vector>

#include "eagleeye/port/env.h"
#include "eagleeye/port/linux_base/env.h"

namespace eagleeye {
namespace port {

class AndroidEnv : public LinuxBaseEnv {
 public:
  // LogWriter *GetLogWriter() override;
  EagleeyeError GetCPUMaxFreq(std::vector<float> *max_freqs) override;
  std::vector<std::string> GetBackTraceUnsafe(int max_steps) override;

  // set/get architecture
  void SetArch(ARMArch arch) { arch_ = arch; }
  ARMArch arch(){return arch_;};

  // std::unique_ptr<MallocLogger> NewMallocLogger(
  //     std::ostringstream *oss,
  //     const std::string &name) override;

 private:
  // AndroidLogWriter log_writer_;
  ARMArch arch_;
};

}  // namespace port
}  // namespace mace

#endif  // MACE_PORT_ANDROID_ENV_H_
