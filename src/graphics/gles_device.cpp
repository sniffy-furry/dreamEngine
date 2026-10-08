#include "dream/graphics/gles_device.hpp"
#if defined(__ANDROID__)
#include <GLES3/gl3.h>
#endif
namespace dream::gfx { bool GLESDevice::initialize(void*,int w,int h){w_=w;h_=h;ready_=true;return true;} void GLESDevice::resize(int w,int h){w_=w;h_=h;} void GLESDevice::begin_frame(FrameContext){if(!ready_)return;
#if defined(__ANDROID__)
 glViewport(0,0,w_,h_); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
} void GLESDevice::end_frame(){} void GLESDevice::shutdown()noexcept{ready_=false;} }
