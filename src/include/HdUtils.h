/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_UTILS_H
#define HD_UTILS_H

#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <chrono>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace HdUtils 
{
    typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
    typedef std::chrono::duration<double, std::milli> time_duration;

    static uint HASH_SEED = 0x9e3779b9; // GOLDEN RATIO

    template <typename T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + HASH_SEED + (seed<<6) + (seed>>2);
    }
    
    std::string                         getNodeName(const MObject& obj);
    std::shared_ptr<spdlog::logger>     getLoggerInstance(std::string loggerName);
    void                                setLogLevel(int level);
    
    std::string                         getTimeDiffString(HdUtils::time_point start, HdUtils::time_point end);
    time_point                          getCurrentTimePoint();
    double                              timePointToDouble(time_point timePoint);

    bool                                playbackActive();
    double                              getCurrentFrame();
}

template <typename T>
class HdVector3
{
    private:
        T m_x;
        T m_y;
        T m_z;

    public:
        HdVector3(T xVal, T yVal, T zVal) : m_x(xVal), m_y(yVal), m_z(zVal){}
        
        MStatus                               toMVector(MVector& vec);
        static std::shared_ptr<HdVector3<T>>  fromMVector(const MVector& vec);

        friend ostream& operator<<(ostream& os, HdVector3<T>& vec3) 
        {
            return os << "x: " << vec3.m_x << ", y: " << vec3.m_y << ", z: " << vec3.m_z << '\n';
        }

};

template <typename T>
class HdVector4
{
    private:
        T m_x;
        T m_y;
        T m_z;
        T m_w;

    public:
        HdVector4(T xVal, T yVal, T zVal, T wVal) : m_x(xVal), m_y(yVal), m_z(zVal), m_w(wVal){}
        
        MStatus                               toMPoint(MPoint& point);
        static std::shared_ptr<HdVector4<T>>  fromMPoint(const MPoint& point);

        friend ostream& operator<<(ostream& os, HdVector4<T>& vec4) 
        {
            return os << "x: " << vec4.m_x << ", y: " << vec4.m_y << ", z: " << vec4.m_z << ", w: " << vec4.m_w <<'\n';
        }
};

typedef class HdVector3<double> HdDouble3;
typedef class HdVector3<float> HdFloat3;

typedef class HdVector4<double> HdDouble4;
typedef class HdVector4<float> HdFloat4;

#endif