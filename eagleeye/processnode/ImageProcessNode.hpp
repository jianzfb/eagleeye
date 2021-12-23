namespace eagleeye
{
template<class SrcT,class TargetT>
ImageProcessNode<SrcT,TargetT>::ImageProcessNode()
{
}

template<class SrcT,class TargetT>
ImageProcessNode<SrcT,TargetT>::~ImageProcessNode()
{
}

template<class SrcT, class TargetT>
bool ImageProcessNode<SrcT, TargetT>::isAllInputSignalOK(){
	for(int i=0; i<this->m_input_signals.size(); ++i){
		if(this->getInputImageSignal(i)->empty()){
			return false;
		}
	}
	return true;
}

template<class SrcT,class TargetT>
SrcT* ImageProcessNode<SrcT,TargetT>::getInputImageSignal(unsigned int index)
{
	SrcT* img_signal=dynamic_cast<SrcT*>(m_input_signals[index]);
	return img_signal;
}

template<class SrcT,class TargetT>
const SrcT* ImageProcessNode<SrcT,TargetT>::getInputImageSignal(unsigned int index) const 
{
	SrcT* img_signal=dynamic_cast<SrcT*>(m_input_signals[index]);
	return img_signal;
}

template<class SrcT,class TargetT>
TargetT* ImageProcessNode<SrcT,TargetT>::getOutputImageSignal(unsigned int index)
{
	TargetT* img_signal=dynamic_cast<TargetT*>(m_output_signals[index]);
	return img_signal;
}

template<class SrcT,class TargetT>
const TargetT* ImageProcessNode<SrcT,TargetT>::getOutputImageSignal(unsigned int index) const
{
	TargetT* img_signal=dynamic_cast<TargetT*>(m_output_signals[index]);
	return img_signal;
}

template<class SrcT,class TargetT>
Matrix<typename TargetT::MetaType> ImageProcessNode<SrcT,TargetT>::getOutputImage(unsigned int index/* =0 */)
{
	if (index<m_output_signals.size())
	{
		TargetT* img_signal=dynamic_cast<TargetT*>(m_output_signals[index]);
		if (img_signal)
		{
			return img_signal->img;
		}
		else
		{
			return Matrix<OutputPixelType>();
		}
	}
	else
	{
		return Matrix<OutputPixelType>();
	}
}

template<class SrcT,class TargetT>
AnySignal* ImageProcessNode<SrcT,TargetT>::makeOutputSignal()
{
	return new TargetT();
}

template<class SrcT,class TargetT>
void ImageProcessNode<SrcT,TargetT>::addInputPort(AnySignal* sig)
{
	Superclass::addInputPort(sig);
}

template<class SrcT,class TargetT>
void ImageProcessNode<SrcT,TargetT>::setInputPort(AnySignal* sig,int index){
	Superclass::setInputPort(sig, index);
}

template<class SrcT,class TargetT>
void ImageProcessNode<SrcT,TargetT>::setOutputPort(AnySignal* sig,int index){
	Superclass::setOutputPort(sig, index);
}
}
