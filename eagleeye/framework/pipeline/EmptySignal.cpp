#include "eagleeye/framework/pipeline/EmptySignal.h"

namespace eagleeye
{
EmptySignal::EmptySignal(){

}   
EmptySignal::~EmptySignal(){

} 

void EmptySignal::copyInfo(AnySignal* sig){
    Superclass::copyInfo(sig);
}
void EmptySignal::copy(AnySignal* sig){
    // do nothing
}

} // namespace eagleeye
