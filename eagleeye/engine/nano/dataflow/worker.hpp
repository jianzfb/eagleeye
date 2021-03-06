#ifndef _EAGLEEYE_WORKER_H_
#define _EAGLEEYE_WORKER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <thread>
#include <mutex>

namespace eagleeye{
namespace dataflow {

class Node;
class Worker {
public:  
  std::size_t id;
  std::thread thread;

  Worker (std::size_t id)
  : id(id) {
  }
};

}

}
#endif
