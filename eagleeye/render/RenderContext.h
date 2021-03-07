#ifndef __EAGLEEYE_RENDER_CONTEXT_H_
#define __EAGLEEYE_RENDER_CONTEXT_H_
#include <GLES3/gl3.h>

namespace eagleeye
{
class RenderContext{
public:
    RenderContext();
    virtual ~RenderContext();

    /**
     * @brief get screen width
     */ 
    int getScreenW();

    /**
     * @brief get screen height
     */ 
    int getScreenH();

    /**
     * @brief opengl context create
     */ 
    void onCreated();

    /**
     * @brief opengl context change
     */ 
	void onChanged(int width, int height);

    /**
     * @brief opengl mouse event
     */ 
    void onMouse(int mouse_x, int mouse_y, int mouse_flag);

private:
	int m_ScreenW;
	int m_ScreenH;
};    
} // namespace eagleeye


#endif