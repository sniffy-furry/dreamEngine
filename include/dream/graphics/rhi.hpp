#pragma once
#include <cstdint>
#include <string_view>
namespace dream::gfx {
enum class Backend : uint8_t { None, OpenGLES, Vulkan };
struct DeviceInfo { Backend backend=Backend::None; int major=0,minor=0; std::string_view vendor{}; std::string_view renderer{}; };
struct FrameContext { uint64_t frame=0; float dt=0; int width=0,height=0; };
class Device { public: virtual ~Device()=default; virtual Backend backend()const noexcept=0; virtual bool initialize(void*,int,int)=0; virtual void resize(int,int)=0; virtual void begin_frame(FrameContext)=0; virtual void end_frame()=0; virtual void shutdown()noexcept=0; virtual DeviceInfo info()const noexcept=0; };
}
