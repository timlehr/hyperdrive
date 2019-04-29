//
// -----------------------------------------------------------------------------
// This source file has been developed within the scope of the
// Technical Director course at Filmakademie Baden-Wuerttemberg.
// http://technicaldirector.de
//
// Written by Tim Lehr
// Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
// -----------------------------------------------------------------------------
//

#include "HdUtils.h"

#include <map>
#include <maya/MFnDependencyNode.h>
#include <maya/MAnimControl.h>
#include <spdlog/spdlog.h>
#include <maya/MGlobal.h>


std::string HdUtils::getTimeDiffString(HdUtils::time_point start, HdUtils::time_point end)
{
    HdUtils::time_duration diff = end-start;
    return std::to_string(diff.count()) + "ms";
}

double HdUtils::timePointToDouble(HdUtils::time_point timePoint)
{
    return std::chrono::duration<double>(timePoint.time_since_epoch()).count();
}

HdUtils::time_point HdUtils::getCurrentTimePoint()
{
    return std::chrono::high_resolution_clock::now();
}

bool HdUtils::playbackActive()
{
    bool animPlay = MAnimControl::isPlaying();
    bool animScrub = MAnimControl::isScrubbing();
    return (animPlay && !animScrub); 
}

double HdUtils::getCurrentFrame()
{
    return MAnimControl::currentTime().value(); 
}

void HdUtils::setLogLevel(int level)
{
    std::shared_ptr<spdlog::logger> log = getLoggerInstance("HdUtils");

    std::map <int, spdlog::level::level_enum> mapping {
        {0, spdlog::level::off},
        {1, spdlog::level::warn},
        {2, spdlog::level::info},
        {3, spdlog::level::debug},
    };

    if (mapping.find(level) != mapping.end())
    {
        MGlobal::displayInfo(MString("Set Hyperdrive log verbosity level: ") + level);
        spdlog::set_level(mapping[level]);
    } else 
    {
        MGlobal::displayError(MString("Invalid log level. Valid range 0 - 3. Input: ") + level);
    }
}

std::string HdUtils::getNodeName(const MObject& obj)
{
    MStatus status;
    MFnDependencyNode dagFn(obj, &status);
    std::string name = dagFn.name(&status).asChar();
    CHECK_MSTATUS(status);
    return name;
}

std::shared_ptr<spdlog::logger> HdUtils::getLoggerInstance(std::string loggerName)
{
    auto log = spdlog::get(loggerName);
    if (log != nullptr) {
        return log;
    }

    try
    {
        return spdlog::stdout_color_mt(loggerName);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Logger '" << loggerName << "' initialization failed: " << 
        ex.what() << std::endl;
    }

    return nullptr;
}

template<typename T>
MStatus HdVector4<T>::toMPoint(MPoint& point)
{
    point.x = this->m_x;
    point.y = this->m_y;
    point.z = this->m_z;
    point.w = this->m_w;
    return MS::kSuccess;
}

template<typename T>
std::shared_ptr<HdVector4<T>> HdVector4<T>::fromMPoint(const MPoint& point)
{
    HdVector4<T> vec4(point.x, point.y, point.z, point.w);
    return std::make_shared<HdVector4<T>>(vec4);
}

template<typename T>
MStatus HdVector3<T>::toMVector(MVector& vec)
{
    vec.x = this->m_x;
    vec.y = this->m_y;
    vec.z = this->m_z;
    return MS::kSuccess;
}

template<typename T>
std::shared_ptr<HdVector3<T>>  HdVector3<T>::fromMVector(const MVector& vec)
{
    HdVector3<T> vec3(vec.x, vec.y, vec.z);
    return std::make_shared<HdVector3<T>>(vec3);
}

// valid classes for the template, needed by the compiler to prevent unknown symbol issues
template class HdVector3<double>;
template class HdVector3<float>;
template class HdVector4<double>;
template class HdVector4<float>;