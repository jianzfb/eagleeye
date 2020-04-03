#include "eagleeye/processnode/DrawNode.h"

namespace eagleeye
{
DrawNode::DrawNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char,3>>, 0);

    // port 0: IMAGE
    // port ~: any signal
    this->setNumberOfInputSignals(1);
} 

DrawNode::~DrawNode(){

}

void DrawNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char,3>>* image_sig = (ImageSignal<Array<unsigned char,3>>*)this->getInputPort(0);
    ImageSignal<Array<unsigned char,3>>* output_sig = (ImageSignal<Array<unsigned char,3>>*)this->getOutputPort(0);

    Matrix<Array<unsigned char,3>> image = image_sig->getData();
    int image_rows = image.rows(); int image_cols = image.cols();
    Matrix<Array<unsigned char,3>> output = image.clone();

    for(int port_index=0; port_index<this->getNumberOfInputSignals(); ++port_index){
        SignalType st = this->getInputPort(port_index)->getSignalType2();
        if(st == EAGLEEYE_ADVANCED_SIGNAL_DET){
            EAGLEEYE_LOGD("draw SIGNAL DET for port %d", port_index);
            ImageSignal<float>* det_sig = (ImageSignal<float>*)(this->getInputPort(port_index));
            Matrix<float> det_result = det_sig->getData();

            int det_num = det_result.rows();
            for(int i=0; i<det_num; ++i){
                int x1 = det_result.at(i,0);
                x1 = eagleeye_clip(x1, 0, image_cols-1);

                int y1 = det_result.at(i,1);
                y1 = eagleeye_clip(y1, 0, image_rows-1);

                int w = det_result.at(i,2);
                int x2 = x1 + w;
                x2 = eagleeye_clip(x2, 0, image_cols-1);

                int h = det_result.at(i,3);
                int y2 = y1 + h;
                y2 = eagleeye_clip(y2, 0, image_rows-1);

                // line 1
                for(int ii=y1; ii<y2; ++ii){
                    for(int jj=x1-2; jj<=x1+2; ++jj){
                        if(jj < 0 || jj >= image_cols){
                            continue;
                        }
                        output.at(ii,jj)[0] = 255;
                        output.at(ii,jj)[1] = 0;
                        output.at(ii,jj)[2] = 0;
                    }
                }

                // line 2
                for(int ii=y1; ii<y2; ++ii){
                    for(int jj=x2-2; jj<=x2+2; ++jj){
                        if(jj < 0 || jj >= image_cols){
                            continue;
                        }
                        output.at(ii,jj)[0] = 255;
                        output.at(ii,jj)[1] = 0;
                        output.at(ii,jj)[2] = 0;
                    }
                }

                // line 3
                for(int ii=y1-2; ii<y1+2; ++ii){
                    if(ii < 0 || ii >= image_rows){
                        continue;
                    }
                    for(int jj=x1; jj<x2; ++jj){
                        output.at(ii,jj)[0] = 255;
                        output.at(ii,jj)[1] = 0;
                        output.at(ii,jj)[2] = 0;
                    }
                }

                // line 4
                for(int ii=y2-2; ii<y2+2; ++ii){
                    if(ii < 0 || ii >= image_rows){
                        continue;
                    }
                    for(int jj=x1; jj<x2; ++jj){
                        output.at(ii,jj)[0] = 255;
                        output.at(ii,jj)[1] = 0;
                        output.at(ii,jj)[2] = 0;
                    }
                }
            } 
        }        
    }

    output_sig->setData(output);
}
} // namespace eagleeye
