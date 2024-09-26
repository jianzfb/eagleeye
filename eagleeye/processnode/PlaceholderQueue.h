#ifndef _EAGLEEYE_PLACEHOLDER_QUEUE_H_
#define _EAGLEEYE_PLACEHOLDER_QUEUE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/AnyNode.h"


namespace eagleeye{
class PlaceholderQueue: public AnyNode{
public:
    typedef PlaceholderQueue                Self;
    typedef AnyNode                         Superclass;

    PlaceholderQueue(int queue_size=8);
    virtual ~PlaceholderQueue();

    EAGLEEYE_CLASSIDENTITY(PlaceholderQueue);

    void config(int placeholder_i, std::string data_type, std::string data_category);
    void push(int placeholder_i, void* data, const size_t* data_size, const int data_dims, const int data_rotation, const int data_type);

    virtual void postexit();

private:    
    PlaceholderQueue(const PlaceholderQueue&);
    void operator=(const PlaceholderQueue&);

    int m_queue_size;
}; 
}
#endif