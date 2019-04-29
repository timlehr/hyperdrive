/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_MESHCACHE_H
#define HD_MESHCACHE_H

#include <vector>
#include <string>
#include <map>
#include "LRUCache11.hpp"
#include "spdlog/spdlog.h"

#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>

#include "HdUtils.h"

struct HdMeshUVSetData 
{
    std::string                         name;
    std::vector<int>                    polyUVCounts;
    std::vector<int>                    polyUVIds;
    std::vector<float>                  uData;
    std::vector<float>                  vData;

    double                              memSize();

    HdMeshUVSetData(std::string setName, int uvCounts, int uvIds) :
            name(setName), polyUVCounts(uvCounts), polyUVIds(uvIds){}
};

struct HdMeshData
{
    int                                 totalVertCount;
    int                                 totalPolyCount;
    std::shared_ptr<MIntArray>          polyVertCounts;
    std::shared_ptr<MIntArray>          polyVertConnections;
    std::shared_ptr<MFloatPointArray>   points;
    std::shared_ptr<MFloatPointArray>   normals;
    std::vector<HdMeshUVSetData>        uvSets;

    std::shared_ptr<MObject>            mayaObject;

    double                              memSize();

    HdMeshData(int tVertCount, int tPolyCount) : totalVertCount(tVertCount), totalPolyCount(tPolyCount){}
};

class HdMeshSet : public std::vector<HdMeshData> 
{
    public:
        double                      memSize();
};

class HdMeshCache 
{
    private:
        lru11::Cache<std::string, HdMeshSet, std::mutex>*  meshCache_;
        std::shared_ptr<spdlog::logger> log;
        std::string cacheId_;
        
        double itemMemSize_ = 0.0;
        double maxMemSize_ = 500 * 1024.0; // 500MB default

    public:
                                     HdMeshCache(std::string cacheId, size_t maxCacheSize);
        virtual                      ~HdMeshCache();

        bool                         exists(std::string poseId);
        MStatus                      put(std::string poseId, std::shared_ptr<HdMeshSet> meshDataPtr);
        std::shared_ptr<HdMeshSet>   get(std::string poseId, MStatus &status, bool copyData);
        MStatus                      clear();
        
        std::string                  cacheId()      {return cacheId_;};
       
        size_t                       size()         {return meshCache_->size();};
        double                       itemMemSize()  {return itemMemSize_;};
        double                       memSize()      {return itemMemSize() * meshCache_->size();};
        
        double                       maxMemSize() {return maxMemSize_;};
        double                       setMaxMemSize(double maxMemSize) {maxMemSize_ = maxMemSize;};
        void                         applyMaxMemSize(double poseDataMemSize);
        
        size_t                       maxSize()      {return meshCache_->getMaxSize();};
        void                         setMaxSize(size_t maxSize);

        MStatus                      initCache(size_t maxSize);
        MStatus                      destroyCache();

        
};

class HdCacheMap 
{
    private:
        static std::map<std::string, std::shared_ptr<HdMeshCache>> cacheMap;
        static std::shared_ptr<spdlog::logger> log;

    public:
        static bool                         exists(std::string cacheId);
        static std::shared_ptr<HdMeshCache> get(std::string cacheId, MStatus &status);
        static MStatus                      removeCache(std::string cacheId);
        static std::shared_ptr<HdMeshCache> createCache(std::string cacheId, MStatus& status, size_t maxSize = 0);
        static MStatus                      clearMap();
        static MStatus                      clearCaches();
        static std::string                  getStatsJson();
};

#endif