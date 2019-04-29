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

#include "HdMeshCache.h"

#include <maya/MGlobal.h>
#include "HdUtils.h"

/***********************************************
 * HDMESHUVSETDATA
 * ********************************************/

double HdMeshUVSetData::memSize()
{
    double result = 0.0;
    result += uData.capacity() * sizeof(float);
    result += vData.capacity() * sizeof(float);
    result += polyUVCounts.capacity() * sizeof(int);
    result += polyUVIds.capacity() * sizeof(int);
    result = result / 1024.0; // Kbytes
}

/***********************************************
 * HDMESHDATA
 * ********************************************/

double HdMeshData::memSize()
{
    double result = 0.0;
    result += polyVertCounts->length() * sizeof(int);
    result += polyVertConnections->length() * sizeof(int);
    result += points->length() * sizeof(float);
    
    if (normals != nullptr) 
    {
        result += normals->length() * sizeof(float);
    }

    for(std::vector<HdMeshUVSetData>::iterator it = uvSets.begin(); it != uvSets.end(); ++it) {
        result += it->memSize();
    }

    //result += uvSets->length() * sizeof(int); TODO: UvSet iterate and add size
    result = result * 2 / 1024.0; // + MObject estimation in Kbytes
}

/***********************************************
 * HDMESHSET
 * ********************************************/

double HdMeshSet::memSize()
{
    double result = 0.0;
    for(std::vector<HdMeshData>::iterator it = begin(); it != end(); ++it) {
        result += it->memSize();
    }
    return result;
}

/***********************************************
 * HDMESHCACHE
 * ********************************************/

HdMeshCache::HdMeshCache(std::string cacheId, size_t maxCacheSize)
{
    // set cache Id
    cacheId_ = cacheId;

    log = HdUtils::getLoggerInstance("HdMeshCache ('" + cacheId + "')");
    
    initCache(maxCacheSize);
}

HdMeshCache::~HdMeshCache()
{
    destroyCache();
}

MStatus HdMeshCache::initCache(size_t maxCacheSize) 
{
    std::string msgStr = "Hyperdrive :: Initialized cache. Size: " + std::to_string(maxCacheSize) + "ID: '" + cacheId() + "'";

    log->info("Initializing cache '{}' with size: {}", cacheId(), maxCacheSize);

    if(meshCache_ != nullptr) 
    {
        // If there is an exisiting cache, destroy it first.
        //destroyCache();
    }

    meshCache_ = new lru11::Cache<std::string, HdMeshSet, std::mutex>(maxCacheSize, 0);
    MGlobal::displayInfo(MString(msgStr.c_str()));
    return MS::kSuccess;
}

MStatus HdMeshCache::destroyCache() 
{
    clear();
    delete meshCache_;
    meshCache_ = NULL;
    std::string msgStr = "Hyperdrive :: De-allocated / destroyed cache. ID: '" + cacheId() + "'";
    MGlobal::displayInfo(MString(msgStr.c_str()));
    log->warn("Destroyed cache.");
    return MS::kSuccess;
}

MStatus HdMeshCache::put(std::string poseId, std::shared_ptr<HdMeshSet> meshSet)
{
    if (maxSize() == 0) 
    {
        log->info("Cache without max size detected. Set max size based on current pose size.");
        applyMaxMemSize(meshSet->memSize());
    }

    itemMemSize_ = meshSet->memSize();
    log->debug("Put cache for pose ID: '{}'. Mem size: {} kbytes", poseId, (int) itemMemSize_);
    meshCache_->insert(poseId, *meshSet);
    return MS::kSuccess;
}

std::shared_ptr<HdMeshSet> HdMeshCache::get(std::string poseId, MStatus &status, bool copyData = false)
{
    std::shared_ptr<HdMeshSet> data;
    log->debug("Get cache for pose: {}", poseId);
    if(meshCache_->contains(poseId)) 
    {
        status = MS::kSuccess;
        if (copyData)
        {
            return std::make_shared<HdMeshSet>(meshCache_->getCopy(poseId)); // get a copy so it's safe to delete
        } else 
        {
            return std::make_shared<HdMeshSet>(meshCache_->get(poseId));
        }
    }
}

bool HdMeshCache::exists(std::string poseId) 
{
    return meshCache_->contains(poseId); 
}

void HdMeshCache::setMaxSize(size_t maxSize)
{
    log->debug("Set maximum cache pose count to: {}", maxSize);
    meshCache_->setMaxSize(maxSize);
}

void HdMeshCache::applyMaxMemSize(double poseDataMemSize)
{
    if (poseDataMemSize == 0.0) 
    {
        log->warn("Cannot set maximum cache size. Pose Data Memory Size is at an invalid value: {}kB", poseDataMemSize);
        return;
    }
    int poseCount = (int) (maxMemSize() / poseDataMemSize);
    log->info("Set maximum cache size to: {}kB. Estimated pose count: {} ({}kB each)", maxMemSize(), poseCount, poseDataMemSize);
    meshCache_->setMaxSize(poseCount);
}

MStatus HdMeshCache::clear()
{
    std::string msgStr = "Hyperdrive :: Cleared cache. ID: '" + cacheId() + "'";
    meshCache_->clear();
    log->info("Cleared cache.");
    MGlobal::displayInfo(MString(msgStr.c_str()));
    return MS::kSuccess;
}


/***********************************************
 * HDCACHEMAP
 * ********************************************/

// INIT LOG
std::shared_ptr<spdlog::logger> HdCacheMap::log = HdUtils::getLoggerInstance("HdCacheMap");
std::map<std::string, std::shared_ptr<HdMeshCache>> HdCacheMap::cacheMap = std::map<std::string, std::shared_ptr<HdMeshCache>>();


bool HdCacheMap::exists(std::string cacheId)
{
    if (cacheMap.empty()) 
    {
        return false;
    }

    if (cacheMap.count(cacheId) > 0) 
    {
        return true;
    } else
    {
        return false;
    }
}

std::shared_ptr<HdMeshCache> HdCacheMap::get(std::string cacheId, MStatus &status)
{
    if (cacheId == "")
    {
        status = MS::kInvalidParameter;
        return nullptr;
    }

    std::map<std::string, std::shared_ptr<HdMeshCache>>::iterator it = cacheMap.find(cacheId);
    if (it != cacheMap.end()) 
    {
        status = MS::kSuccess;
        return it->second;
    } else 
    {
        status = MS::kSuccess;
        log->warn("Could not find cache ID in map. Create new cache for ID: '{}'", cacheId);
        return HdCacheMap::createCache(cacheId, status, 0);
    }
        
}

MStatus HdCacheMap::removeCache(std::string cacheId)
{
    MStatus  status = MS::kSuccess;

    if (exists(cacheId))
    {
        std::shared_ptr<HdMeshCache> meshCache = get(cacheId, status);
        meshCache->destroyCache();
        cacheMap.erase(cacheId);
    }
}

std::shared_ptr<HdMeshCache> HdCacheMap::createCache(std::string cacheId,  MStatus& status, size_t maxSize)
{
    std::shared_ptr<HdMeshCache> meshCache = std::make_shared<HdMeshCache>(cacheId, maxSize);
    cacheMap[cacheId] = meshCache;
    status = MS::kSuccess;
    log->info("Created new cache for cache ID: '{}'", cacheId);
    return meshCache;
}

MStatus HdCacheMap::clearMap()
{
    cacheMap.clear();
    log->info("Cleared Cache Mapping.");
}

MStatus HdCacheMap::clearCaches()
{
    std::map<std::string, std::shared_ptr<HdMeshCache>>::iterator it;
    for ( it = cacheMap.begin(); it != cacheMap.end(); it++)
    {
        std::shared_ptr<HdMeshCache> meshCache = it->second;
        meshCache->clear();
    }
    log->info("Cleared all caches.");
}

std::string HdCacheMap::getStatsJson()
{
    std::string result = "[";
    std::map<std::string, std::shared_ptr<HdMeshCache>>::iterator it;
    for ( it = cacheMap.begin(); it != cacheMap.end(); it++)
    {   
        std::shared_ptr<HdMeshCache> meshCache = it->second;
        std::string substring = "";

        if (it != cacheMap.begin())
        {
            substring += ", ";
        }

        substring += "{\"id\": \"" + meshCache->cacheId() + "\", ";
        substring += "\"size\": " + std::to_string(meshCache->size()) + ", ";
        substring += "\"max_size\": " + std::to_string(meshCache->maxSize()) + ", ";
        substring += "\"item_mem_size\": " + std::to_string(meshCache->itemMemSize()) + ", ";
        substring += "\"current_mem_size\": " + std::to_string(meshCache->memSize()) + ", ";
        substring += "\"max_mem_size\": " + std::to_string(meshCache->maxMemSize()) + "}";
        result += substring;
    }
    result += "]";
    return result;
}