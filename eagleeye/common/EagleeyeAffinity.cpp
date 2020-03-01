#include "eagleeye/common/EagleeyeAffinity.h"

#ifdef EAGLEEYE_ANDROID
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdio.h>
#include <vector>
#include <errno.h>
#include <algorithm>
#include <iostream>
namespace eagleeye{
static int GetCPUCount() {
  char path[32];
  int cpu_count = 0;
  int result = 0;

  while (true) {
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d", cpu_count);
    result = access(path, F_OK);
    if (result != 0) {
      if (errno != ENOENT) {
        // mylog("Access %s path failed: %s\n", path, strerror(errno));
      }
      return cpu_count;
    }
    cpu_count++;
  }
}

static int GetCPUMaxFreq(int cpu_id) {
  char path[64];
  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq",
           cpu_id);

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    // mylog("File: %s not exists.\n", path);
    return 0;
  }

  int freq = 0;
  int items_read = fscanf(fp, "%d", &freq);
  if (items_read != 1) {
    // mylog("Read file: %s failed.", path);
  }
  fclose(fp);
  return freq;
}

static int getAffinityMask() {
  int cpu_count = GetCPUCount();
  std::vector<int> cpu_max_freq(cpu_count);

  // get cpu max frequency
  for (int i = 0; i < cpu_count; ++i) {
    cpu_max_freq[i] = GetCPUMaxFreq(i);
    if (cpu_max_freq[i] <= 0) {
//      VLOG(1) << "Cannot get CPU" << i << "'s max frequency info, maybe it is offline.";
      return 0;
    }
  }

  int big_core_freq = *(std::max_element(cpu_max_freq.begin(), cpu_max_freq.end()));
  int little_core_freq = *(std::min_element(cpu_max_freq.begin(), cpu_max_freq.end()));

  if (big_core_freq == little_core_freq) {
//    VLOG(2) << "Frequency of every core is the same: " << big_core_freq;
    return 0;
  }

  int mask = 0;
  for (int i = 0; i < cpu_count; ++i) {
    if (cpu_max_freq[i] == big_core_freq) {
      mask |= (1 << i);
    }
  }

  return mask;
}

void setCurrentThreadAffinityMask() {
  thread_local bool init = false;
  thread_local int mask = 0;
  if (init)
    return;
  if (mask == 0) {
    mask = getAffinityMask();
    if (mask == 0) {
      init = true;
      // mylog("No need to set affinity\n");
      std::cout<<"No need to set affinity"<<std::endl;
      return;
    }
  }
  int err, syscallres;
  pid_t pid = gettid();
  syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
  if (syscallres) {
    err = errno;
    // mylog("Error in the syscall setaffinity with mask: %d err: %d\n", mask, err);
    std::cout<<"error"<<std::endl;
  } else {
    // mylog("Success in the syscall setaffinity with mask: %d\n", mask);
    std::cout<<"success"<<std::endl;
    init = true;
  }
}
}
#else
namespace eagleeye{
void setCurrentThreadAffinityMask() {
    // empty
}
}
#endif
