/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_POSE_NODE
#define HD_POSE_NODE

#include <maya/MPxNode.h>
#include "spdlog/spdlog.h"
#include "HdPose.h"

class HdPoseNode : public MPxNode 
{
    public:
                                    HdPoseNode();
        virtual                     ~HdPoseNode();
        static void*                creator();
        static MStatus              initialize();

        void                        postConstructor();
        virtual SchedulingType      schedulingType() const override;

        virtual MStatus             compute(const MPlug& plug, MDataBlock& data);
        
        MStatus                     setCacheIds(MDataBlock& data);
        bool                        cachesContainPoseId(MDataBlock& data, std::string poseIdHash, MStatus& status);

        HdPose                      createPose(MDataBlock& data, MStatus& status);
        MStatus                     setPoseId(MDataBlock& data, HdPose* pose);
        MStatus                     setRigFrozen(MDataBlock& data, bool frozen);

        MStatus                     preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);

        void                        logExecutionTime(HdUtils::time_point startTime);

        static MTypeId id; // unique node id
    
        static MObject aInCtrlVals;
        static MObject aInRigTag;
        static MObject aOutPoseId;
        static MObject aOutFreezeRig;
        static MObject aOutCacheIds;

        static MObject aInWhitelist;

    private:
        std::shared_ptr<spdlog::logger> log;
        bool                            instanceLog = false;
        bool                            currentPoseValid = false;
        bool                            needsEvaluation = false;
};

#endif