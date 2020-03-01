#include "eagleeye/framework/EagleeyeTimeStamp.h"
#include <iostream>

namespace eagleeye
{
bool EagleeyeTimeStamp::m_cross_border = false;    
/**
 * Instance creation.
 */
void EagleeyeTimeStamp::modified()
{
//static LONG time_stamp_time = 0;
//m_modified_time = (unsigned long)InterlockedIncrement(&time_stamp_time);
    static unsigned long time_stamp_time = 1;
    this->m_modified_time = __sync_fetch_and_add(&time_stamp_time, 1);
    if(this->m_modified_time == 0){
        this->m_cross_border = true;
    }
}
}
