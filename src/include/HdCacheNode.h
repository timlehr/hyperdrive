/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_CACHENODE_H
#define HD_CACHENODE_H

#include <time.h>
#include <maya/MPxNode.h>

#include "HdPose.h"
#include "HdMeshCache.h"

class HdCacheNode : public MPxNode 
{
    public:
                                    HdCacheNode();
        virtual                     ~HdCacheNode();
        static void*                creator();
        static MStatus              initialize();

        virtual SchedulingType      schedulingType() const override;
        void                        postConstructor();
        MStatus                     setDependentsDirty(MPlug const & inPlug, MPlugArray  & affectedPlugs);

        virtual MStatus             compute(const MPlug& plug, MDataBlock& data);

        std::shared_ptr<HdMeshSet>  createCacheMeshData(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, MStatus& status);
        MStatus                     loadMeshDataFromCache(std::shared_ptr<HdMeshCache> meshCache, MObject* oMesh, HdMeshData* meshDataPtr);
        MStatus                     setOutMeshData(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, std::string poseId, int meshElementIndex, bool noEffect);
        MStatus                     setOutMeshes(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, std::string poseId, bool noEffect);
        MStatus                     skipCompute(const MPlug& plug, MDataBlock& data);
        MStatus                     preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode);
        
        std::shared_ptr<HdMeshCache> getMeshCache(std::string cacheId, MStatus& status);
        std::string                  getCacheId(MDataBlock& data, MStatus& status);
        
        void                         logExecutionTime(HdUtils::time_point startTime);

        static MTypeId id; // unique node id
    
        static MObject aInPoseId;
        static MObject aInMeshes;
        static MObject aOutMeshes;
        static MObject aInCacheId;

    private:
        std::string                     lastPoseId;
        bool                            currentPoseValid;
        bool                            needsEvaluation;
        bool                            hdDisabled = false;
        std::shared_ptr<spdlog::logger> log;
        bool                            instanceLog = false;
};

#endif