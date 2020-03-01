#ifndef _EAGLEEYE_ALGORITHM_H_
#define _EAGLEEYE_ALGORITHM_H_
#include "eagleeye/basic/Matrix.h"

namespace eagleeye{
class Algorithm{
public:
    Algorithm();
    virtual ~Algorithm();

    virtual void run() = 0;

protected:
};
}

#endif