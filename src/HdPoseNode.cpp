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

#include "HdPoseNode.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnData.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MPlugArray.h>
#include <maya/MUuid.h>
#include <maya/MEvaluationNode.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnDependencyNode.h>

#include "HdUtils.h"
#include "HdMeshCache.h"

MTypeId HdPoseNode::id(0x00171215);
MObject HdPoseNode::aInCtrlVals;
MObject HdPoseNode::aInRigTag;
MObject HdPoseNode::aOutPoseId;
MObject HdPoseNode::aOutCacheIds;
MObject HdPoseNode::aOutFreezeRig;
MObject HdPoseNode::aInWhitelist;

HdPoseNode::HdPoseNode(){}
HdPoseNode::~HdPoseNode(){}

void HdPoseNode::postConstructor()
{   
    MStatus status = MStatus::kSuccess;

    // setup logger
    std::string loggerName(typeName().asChar());
    log = HdUtils::getLoggerInstance(loggerName);

    // set icon
    MFnDependencyNode nodeDepFn(thisMObject());
    nodeDepFn.setIcon("hyperdrivePose.png");
}

HdPoseNode::SchedulingType HdPoseNode::schedulingType() const
{
    // Globally serialize this node because the compute method is not thread safe
    return HdPoseNode::kParallel;
}

MStatus HdPoseNode::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
    needsEvaluation = !HdUtils::playbackActive();
    log->debug("Needs evaluation: {}", needsEvaluation);

    return MS::kSuccess;
}


MStatus HdPoseNode::setCacheIds(MDataBlock& data)
{
    MStatus status = MS::kFailure;

    MArrayDataHandle hOutCacheIds = data.outputArrayValue(aOutCacheIds, &status);
    CHECK_MSTATUS(status);

    unsigned int outCacheIdsCount = hOutCacheIds.elementCount(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (outCacheIdsCount < 1)
    {
        status = MS::kInvalidParameter;
        log->error("No output data for cacheId generated so far. Connect a cache node to the plug.");
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    for (int i=0; i<outCacheIdsCount; i++)
    {
        status = hOutCacheIds.jumpToElement(i);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        
        MDataHandle hOutCacheId = hOutCacheIds.outputValue(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        std::string cacheId = hOutCacheId.asString().asChar();

        if(cacheId.size() < 1)
        {
            // Generate UUID
            MUuid uuid = MUuid();
            uuid.generate();
            hOutCacheId.setString(uuid.asString());
            hOutCacheId.setClean();

            log->info("Missing cache ID for output plug index {}. Generated new cache ID: '{}'", i, uuid.asString().asChar());
        }

        hOutCacheIds.setClean();
        hOutCacheIds.setAllClean();

        status = MS::kSuccess;
    }

    return status;
}

bool HdPoseNode::cachesContainPoseId(MDataBlock& data, std::string poseIdHash, MStatus& status)
{
    MPlug cacheIdsPlug(thisMObject(), aOutCacheIds);
    CHECK_MSTATUS(status);

    unsigned int plugCount = cacheIdsPlug.numElements();

    MArrayDataHandle hOutCacheIds = data.outputArrayValue(aOutCacheIds, &status);
    CHECK_MSTATUS(status);

    unsigned int idsCount = hOutCacheIds.elementCount(&status);
    CHECK_MSTATUS(status);

    unsigned int connectedPlugs = 0;

    if (plugCount < 1)
    {
        // No caches generated, run eval in any case
        return false;
    }

    for (int i=0; i<plugCount; i++)
    {
        MPlug plug = cacheIdsPlug[i];

        if (plug.isConnected())
        {
            connectedPlugs += 1;

            if (i >= idsCount)
            {
                log->warn("Tried accessing out of range datablock for output plug with index {}. Total datablock size: {}", i, idsCount);
                status = MS::kInvalidParameter;
                return false;
            }
            status = hOutCacheIds.jumpToElement(i);
            CHECK_MSTATUS(status);
            
            MDataHandle hOutCacheId = hOutCacheIds.outputValue(&status);
            std::string cacheId = hOutCacheId.asString().asChar();
            if (!HdCacheMap::exists(cacheId)) 
            {
                log->warn("Cache ID on out plug index {} is not mapped to a cache yet. Cache ID: '{}'", i, cacheId);
                status = MS::kSuccess;
                return false;
            }

            std::shared_ptr<HdMeshCache> meshCache = HdCacheMap::get(cacheId, status);
            CHECK_MSTATUS(status);

            if (!meshCache->exists(poseIdHash)) 
            {
                log->warn("Missing pose cache for plug '{}'. Cache ID: '{}'. Pose ID: '{}'", i, cacheId, poseIdHash);
                status = MS::kSuccess;
                return false;
            }
        }
    }

    if(connectedPlugs > 0)
    {
        status = MS::kSuccess;
        return true; // TODO: Check if this is really always correct!
    } else 
    {
        log->warn("No cache IDs connected to Hyperdrive cache nodes.");
        status = MS::kInvalidParameter;
        return false;
    }
     
}

MStatus HdPoseNode::compute(const MPlug& plug, MDataBlock& data)
{
    // init vars
    MStatus status;

    if (!instanceLog) 
    {
        std::string nodeName = HdUtils::getNodeName(thisMObject());
        if (nodeName != "") 
        {
            log = HdUtils::getLoggerInstance(nodeName);
            log->debug("Node instance logger initialized.");
            instanceLog = true;
        }
    }

    // ***********************************
    // CHECK IF COMPUTE NEEDS TO RUN
    // ***********************************

    if (plug != aOutPoseId)
    {
        log->debug("Ignore plug: {}", plug.info().asChar());
        return MS::kUnknownParameter;
    }

    // **********************************************
    // CHECK IF POSE NEEDS CALCULATION / RIG FREEZE
    // **********************************************

    MDataHandle stateData = data.inputValue( state, &status );
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set cache ids, make sure they exist for each plug available
    status = setCacheIds(data);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (stateData.asShort() == 1) // NodeState: HasNoEffect!
    { 
        log->warn("User disabled Hyperdrive. Forced evaluation.");
        status = setRigFrozen(data, false);
        CHECK_MSTATUS(status);

        MDataHandle hOutPoseId = data.outputValue(aOutPoseId, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        hOutPoseId.setString(MString(""));
        hOutPoseId.setClean();

        CHECK_MSTATUS(status);
        return status;
    } else if (needsEvaluation) 
    {
        log->warn("Bypass pose node. Forced evaluation.");
        status = setRigFrozen(data, false);
        CHECK_MSTATUS(status);
        return status;
    }

    // STOP TIME
    HdUtils::time_point startTime = HdUtils::getCurrentTimePoint();
    double startTimeDouble = HdUtils::timePointToDouble(startTime);
    log->debug("Compute plug: {}", plug.info().asChar());

    // set rig unfrozen in case compute fails
    status = setRigFrozen(data, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // *************************************
    // CREATE POSE AND SET DEBUG HASH ATTR
    // *************************************

    HdPose pose = createPose(data, status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    log->debug("Pose ID computed: {}", pose.hash());

    bool poseCached = cachesContainPoseId(data, pose.hash(), status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if(!poseCached) // ... if there is no cache for the current Pose
    {
        // set node states to NORMAL
        log->debug("Missing cache for pose ID '{}'. Evaluate Rig.", pose.hash());
        status = setRigFrozen(data, false);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } 
    else 
    {
        // set node states to HASNOEFFECT
        log->debug("Found cache for pose ID '{}'. Freeze Rig.", pose.hash());
        status = setRigFrozen(data, true);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // SET POSE ID
    setPoseId(data, &pose);

    // remove dirty so it won't be recalculated
    data.setClean(plug); 

    // FREE MEMORY
    pose.clear();

    logExecutionTime(startTime);

    return MS::kSuccess;
}

HdPose HdPoseNode::createPose(MDataBlock& data, MStatus& status) 
{
    // get RigTag
    MDataHandle hInRigTag = data.inputValue(aInRigTag, &status);
    CHECK_MSTATUS(status);
    MString rigTag = hInRigTag.asString();

    // create pose
    HdPose pose = HdPose(rigTag.asChar());

    // get controller values
    MArrayDataHandle hInCtrlVals = data.inputArrayValue(aInCtrlVals, &status);
    CHECK_MSTATUS(status);
    
    int count = hInCtrlVals.elementCount(&status);
    CHECK_MSTATUS(status);

    if ( count > 0) 
    {
        for (int i=0; i < count; i++)
        {
            status = hInCtrlVals.jumpToElement(i);
            CHECK_MSTATUS(status);

            MDataHandle hInputCtrl = hInCtrlVals.inputValue(&status);
            CHECK_MSTATUS(status);

            pose.push_back(hInputCtrl.asDouble());
        }
    }

    status = MS::kSuccess;
    return pose;
}

MStatus HdPoseNode::setRigFrozen(MDataBlock& data, bool frozen)
{
    MStatus status = MS::kSuccess;

    // Get out freeze handle
    MDataHandle hOutFreezeRig = data.outputValue(aOutFreezeRig, &status);
    CHECK_MSTATUS(status);

    hOutFreezeRig.set(frozen);
    hOutFreezeRig.setClean();

    return status;
}

MStatus HdPoseNode::setPoseId(MDataBlock& data, HdPose* pose)
{
    MStatus status = MS::kSuccess;

    MDataHandle hOutPoseId = data.outputValue(aOutPoseId, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    hOutPoseId.setString(MString(pose->hash().c_str()));
    hOutPoseId.setClean();

    return status;
}

void HdPoseNode::logExecutionTime(HdUtils::time_point startTime)
{
    HdUtils::time_point endTime = HdUtils::getCurrentTimePoint();
    double endTimeDouble = HdUtils::timePointToDouble(endTime);
    log->debug("Exec End: {} | Diff: {}", std::to_string(endTimeDouble), HdUtils::getTimeDiffString(startTime, endTime));
}


void* HdPoseNode::creator()
{
    return new HdPoseNode();
}

MStatus HdPoseNode::initialize()
{
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // OUTPUT - POSE ID
    aOutPoseId = tAttr.create("outPoseId", "outPoseId", MFnData::kString);
    tAttr.setWritable(false); // disable input
    tAttr.setStorable(false);
    tAttr.setHidden(false);
    addAttribute(aOutPoseId);

    // OUTPUT - CACHE IDS
    aOutCacheIds = tAttr.create("outCacheIds", "outCacheIds", MFnData::kString);
    tAttr.setWritable(false); // disable input
    tAttr.setStorable(true);  // store in file
    tAttr.setArray(true);
    tAttr.setIndexMatters(false);
    tAttr.setHidden(false);
    tAttr.setUsesArrayDataBuilder(true);
    addAttribute(aOutCacheIds);

    // OUTPUT - FROZEN
    aOutFreezeRig = nAttr.create("outFreezeRig", "outFreezeRig", MFnNumericData::kBoolean);
    nAttr.setWritable(false); // disable input
    nAttr.setStorable(false);
    nAttr.setHidden(false);
    nAttr.setDefault(false);
    addAttribute(aOutFreezeRig);

    // INPUT - RIG TAG
    aInRigTag = tAttr.create("inRigTag", "inRigTag", MFnData::kString);
    tAttr.setKeyable(true); // has to be true to be visible in node editor
    tAttr.setConnectable(true);
    tAttr.setStorable(true);
    tAttr.setReadable(false); // disable output
    addAttribute(aInRigTag);
    attributeAffects(aInRigTag, aOutPoseId);
    attributeAffects(aInRigTag, aOutFreezeRig);
    attributeAffects(aInRigTag, aOutCacheIds);

    // INPUT - CONTROLLER VALUES
    aInCtrlVals = nAttr.create("inCtrlVals", "inCtrlVals",MFnNumericData::kDouble);
    nAttr.setKeyable(true); // has to be true to be visible in node editor
    nAttr.setConnectable(true);
    nAttr.setStorable(true);
    nAttr.setReadable(false); // disable output
    nAttr.setArray(true);
    nAttr.setIndexMatters(false); // TODO: REMOVE THIS !!! JUST FOR DEBUGGING SO THAT connectAttr WITH NextAvailable WORKS !!!
    addAttribute(aInCtrlVals);
    attributeAffects(aInCtrlVals, aOutPoseId);
    attributeAffects(aInCtrlVals, aOutFreezeRig);
    attributeAffects(aInCtrlVals, aOutCacheIds);

    // INPUT - WHITELIST NODES
    aInWhitelist = tAttr.create("inWhitelist", "inWhitelist", MFnData::kString);
    tAttr.setKeyable(true); // has to be true to be visible in node editor
    tAttr.setConnectable(true);
    tAttr.setStorable(true);
    tAttr.setReadable(false); // disable output
    tAttr.setArray(true);
    tAttr.setIndexMatters(false); // has to be true to be visible in node editor
    addAttribute(aInWhitelist);

    return MS::kSuccess;
}