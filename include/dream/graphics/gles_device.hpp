#pragma once
#include "rhi.hpp"
namespace dream::gfx { class GLESDevice final : public Device { public: Backend backend()const noexcept override{return Backend::OpenGLES;} bool initialize(void* native_window,int w,int h)override; void resize(int w,int h)override; void begin_frame(FrameContext)override; void end_frame()override; void shutdown()noexcept override; DeviceInfo info()const noexcept override{return info_;} private: DeviceInfo info_{Backend::OpenGLES,3,0,{},{}}; int w_=0,h_=0; bool ready_=false; }; }
