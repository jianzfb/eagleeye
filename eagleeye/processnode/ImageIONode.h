#ifndef _EAGLEEYE_IMAGEIONODE_H_
#define _EAGLEEYE_IMAGEIONODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"

namespace eagleeye
{
enum ChanelsOrder
{
	CHANELS_SWITCH_R_AND_B,
	CHANELS_NO_CHANGE
};

template<class ImgSigT>
class ImageIONode:public AnyNode
{
public:
	typedef ImageIONode							Self;
	typedef AnyNode								Superclass;

	typedef typename ImgSigT::MetaType			PixelType;

	ImageIONode(const char* unit_name="");
	virtual ~ImageIONode();

	/**
	 *	@brief get class identity	
	 */	 
	 EAGLEEYE_CLASSIDENTITY(ImageIONode);

	/**
	 *	@brief set/get the file path
	 */
	virtual void setFilePath(std::string file_path);
	virtual void getFilePath(std::string& file_path);

	/**
	 * @brief set channel order
	 * 
	 * @param c_order 
	 */
	void switchChanelsOrder(ChanelsOrder c_order);

protected:
	std::string m_file_path;
	ChanelsOrder m_channels_order;

	/**
	 *	@brief make one output image signal
	 */
	virtual AnySignal* makeOutputSignal();

	/**
	 *	@brief switch pixel channels
	 */
	template<class T>
	void switchPixelChannels(Matrix<T>& img){};
	template<>
	void switchPixelChannels(Matrix<ERGB>& img)
	{
		switch(m_channels_order)
		{
		case CHANELS_SWITCH_R_AND_B:
			{
				img = img.transform(SwitchOperations<ERGB,ERGB,0,2>());
				return;
			}
		}
	}
	template<>
	void switchPixelChannels(Matrix<ERGBA>& img)
	{
		switch(m_channels_order)
		{
		case CHANELS_SWITCH_R_AND_B:
			{
				img = img.transform(SwitchOperations<ERGBA,ERGBA,0,2>());
				return;
			}
		}
	}

private:
	ImageIONode(const ImageIONode&);
	void operator=(const ImageIONode&);
};
}

#include "eagleeye/processnode/ImageIONode.hpp"
#endif