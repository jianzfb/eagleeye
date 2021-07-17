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
     * @brief get xy
     * 
     * @param x 
     * @param y 
     */
    void getXY(float& x, float& y);

    /**
     * @brief Get the Rotate object
     * 
     * @return int 
     */
    int getRotate();

    /**
     * @brief Get the Mirror object
     * 
     * @return true 
     * @return false 
     */
    bool getMirror();

    /**
     * @brief opengl context create
     */ 
    void onCreated();

    /**
     * @brief opengl context change
     */ 
	void onChanged(int width, int height, int rotate, bool mirror);

    /**
     * @brief opengl mouse event
     */ 
    void onMouse(int mouse_x, int mouse_y, int mouse_action);

    void getMouse(int& mouse_x, int& mouse_y, int& mouse_action);

    /**
     * @brief 
     * 
     * @param data 
     */
    void onTransformMatrix(float* data);


private:
	int m_ScreenW;
	int m_ScreenH;

    int m_rotate;
    bool m_mirror;

    int m_mouse_action;
    int m_mouse_x;
    int m_mouse_y;
};    
} // namespace eagleeye


#endif