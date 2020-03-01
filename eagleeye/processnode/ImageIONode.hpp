namespace eagleeye
{
template<class ImgSigT>
ImageIONode<ImgSigT>::ImageIONode(const char* unit_name)
	:AnyNode(unit_name)
{
	m_file_path = "";
	m_channels_order = CHANELS_NO_CHANGE;
}
template<class ImgSigT>
ImageIONode<ImgSigT>::~ImageIONode()
{

}

template<class ImgSigT>
void ImageIONode<ImgSigT>::setFilePath(std::string file_path)
{
	m_file_path = file_path;

	//force time to update
	modified();
}

template<class ImgSigT>
void ImageIONode<ImgSigT>::getFilePath(std::string& file_path)
{
	file_path = m_file_path;
}

template<class ImgSigT>
void ImageIONode<ImgSigT>::switchChanelsOrder(ChanelsOrder c_order)
{
	m_channels_order = c_order;
}

template<class ImgSigT>
AnySignal* ImageIONode<ImgSigT>::makeOutputSignal()
{
	return new ImgSigT();
}
}