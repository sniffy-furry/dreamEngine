#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>

namespace astra {

struct Vec2 { float x=0, y=0; };
struct Vec3 {
    float x=0, y=0, z=0;
    Vec3 operator+(const Vec3& o) const { return {x+o.x,y+o.y,z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x,y-o.y,z-o.z}; }
    Vec3 operator*(float s) const { return {x*s,y*s,z*s}; }
    Vec3 operator/(float s) const { return {x/s,y/s,z/s}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    float lengthSquared() const { return x*x+y*y+z*z; }
    float length() const { return std::sqrt(lengthSquared()); }
    Vec3 normalized() const { float l=length(); return l > 1e-6f ? *this/l : Vec3{}; }
    static float dot(const Vec3& a,const Vec3& b){return a.x*b.x+a.y*b.y+a.z*b.z;}
    static Vec3 cross(const Vec3& a,const Vec3& b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
};

struct Quat {
    float x=0,y=0,z=0,w=1;
    static Quat identity(){return {};}
    static Quat angleAxis(float radians, Vec3 axis){
        axis=axis.normalized(); float h=radians*0.5f,s=std::sin(h);
        return {axis.x*s,axis.y*s,axis.z*s,std::cos(h)};
    }
    static Quat fromEulerXYZ(Vec3 radians){
        const Quat qx=angleAxis(radians.x,{1,0,0});
        const Quat qy=angleAxis(radians.y,{0,1,0});
        const Quat qz=angleAxis(radians.z,{0,0,1});
        return qy*qx*qz;
    }
    Quat conjugate() const {return {-x,-y,-z,w};}
    Quat normalized() const {float l=std::sqrt(x*x+y*y+z*z+w*w); return l>1e-6f?Quat{x/l,y/l,z/l,w/l}:identity();}
    Quat operator*(const Quat& b) const {
        return {w*b.x+x*b.w+y*b.z-z*b.y,
                w*b.y-x*b.z+y*b.w+z*b.x,
                w*b.z+x*b.y-y*b.x+z*b.w,
                w*b.w-x*b.x-y*b.y-z*b.z};
    }
    Vec3 rotate(Vec3 v) const {
        Quat p{v.x,v.y,v.z,0}; Quat r=(*this)*p*conjugate(); return {r.x,r.y,r.z};
    }
    Vec3 toEulerXYZ() const {
        const Quat q=normalized();
        const float sinr_cosp=2*(q.w*q.x+q.y*q.z);
        const float cosr_cosp=1-2*(q.x*q.x+q.y*q.y);
        const float roll=std::atan2(sinr_cosp,cosr_cosp);
        const float sinp=2*(q.w*q.y-q.z*q.x);
        const float pitch=std::abs(sinp)>=1 ? std::copysign(3.14159265358979323846f/2,sinp) : std::asin(sinp);
        const float siny_cosp=2*(q.w*q.z+q.x*q.y);
        const float cosy_cosp=1-2*(q.y*q.y+q.z*q.z);
        const float yaw=std::atan2(siny_cosp,cosy_cosp);
        return {roll,pitch,yaw};
    }
};

struct Mat4 {
    std::array<float,16> m{};
    static Mat4 identity(){Mat4 r; r.m={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; return r;}
    static Mat4 TRS(Vec3 p,Quat q,Vec3 s){
        q=q.normalized(); float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
        float xy=q.x*q.y,xz=q.x*q.z,yz=q.y*q.z,wx=q.w*q.x,wy=q.w*q.y,wz=q.w*q.z;
        Mat4 r=identity();
        r.m[0]=(1-2*(yy+zz))*s.x; r.m[1]=(2*(xy+wz))*s.x; r.m[2]=(2*(xz-wy))*s.x;
        r.m[4]=(2*(xy-wz))*s.y; r.m[5]=(1-2*(xx+zz))*s.y; r.m[6]=(2*(yz+wx))*s.y;
        r.m[8]=(2*(xz+wy))*s.z; r.m[9]=(2*(yz-wx))*s.z; r.m[10]=(1-2*(xx+yy))*s.z;
        r.m[12]=p.x; r.m[13]=p.y; r.m[14]=p.z; return r;
    }
    Mat4 operator*(const Mat4& b) const {
        Mat4 r{};
        for(int c=0;c<4;c++) for(int row=0;row<4;row++) {
            float v=0; for(int k=0;k<4;k++) v += m[k*4+row]*b.m[c*4+k]; r.m[c*4+row]=v;
        }
        return r;
    }
};

struct Transform {
    Vec3 position{};
    Quat rotation=Quat::identity();
    Vec3 scale{1,1,1};
    Mat4 localMatrix() const { return Mat4::TRS(position,rotation,scale); }
};

} // namespace astra
