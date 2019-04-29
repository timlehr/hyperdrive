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

#include "HdEvaluator.h"

#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MProfiler.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MGraphNodeIterator.h>

#include "HdUtils.h"
#include "HdPoseNode.h"
#include "HdCacheNode.h"

namespace 
{
    int _profilerCategory = MProfiler::addCategory("Hyperdrive Evaluator");
}

HdEvaluator::HdEvaluator() {
    log = HdUtils::getLoggerInstance("HdEvaluator");
}

HdEvaluator::~HdEvaluator() {}

void* HdEvaluator::creator()
{
    // reset internal state on creation (new scene etc.)
    HdEvaluator* newEval = new HdEvaluator();
    newEval->evaluatorReset();
    return ((void*) newEval);
}

const char* HdEvaluator::evaluatorName()
{
    return "Hyperdrive";
}

void HdEvaluator::evaluatorReset()
{
    log->info("Reset Hyperdrive Evaluator.");
    hdAvailable = false;
    evaluatorInitialized = false;

    // NOTE: Subgraph consolidation is currently NOT SUPPORTED and crashes Maya.
    //setConsolidation(HdEvaluator::ConsolidationType::kConsolidateSubgraph);

    if(consolidation() == ConsolidationType::kConsolidateSubgraph)
    {
        log->debug("Consolidation: Subgraph");
    } else {
        log->debug("Consolidation: Single Node");
    }

    poseNodes.clear();
    outputMeshes.clear();
    whitelistNodes.clear();
    cachedPoses.clear();
}

void HdEvaluator::evaluatorInit()
{
    MStatus status;

    // STOP TIME
    HdUtils::time_point startTime = HdUtils::getCurrentTimePoint();
    double startTimeDouble = HdUtils::timePointToDouble(startTime);

    log->info("*** Begin Hyperdrive evaluator initialization ***");
    collectHdPoseNodes();

    if (poseNodes.length() == 0) 
    {
        hdAvailable = false;
        log->info("No Pose nodes detected. Ignore HdEvaluator.");
        evaluatorInitialized = true;
        log->info("*** Completed Hyperdrive evaluator initialization ***");
        return;
    }
    
    hdAvailable = true;
    log->info("{} pose nodes detected. HdEvaluator active.", poseNodes.length());

    // collect output meshes
    collectOutputMeshes();
    collectWhitelistNodes();

    // CALC EXEC TIME
    HdUtils::time_point endTime = HdUtils::getCurrentTimePoint();
    double endTimeDouble = HdUtils::timePointToDouble(endTime);

    evaluatorInitialized = true;
    log->info("*** Completed Hyperdrive Evaluator initialization | Exec time: {} ***", HdUtils::getTimeDiffString(startTime, endTime));
}

MStatus HdEvaluator::getNodesFromArrayPlug(MPlug arrayPlug, MObjectArray* nodes, bool asDst, bool asSrc)
{
    MStatus status = MS::kSuccess;

    if(!arrayPlug.isArray()) return MS::kInvalidParameter;

    for (unsigned int i=0; i<arrayPlug.numElements(); i++)
    {
        MPlug elementPlug = arrayPlug.elementByLogicalIndex(i, &status);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        // get connected plugs
        MPlugArray connectedPlugs;
        elementPlug.connectedTo(connectedPlugs, asDst, asSrc, &status);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        for (unsigned int j=0; j<connectedPlugs.length(); j++)
        {
            MPlug plug = connectedPlugs[j];
            MObject oNode = plug.node(&status);
            CHECK_MSTATUS(status);
            if (status != MS::kSuccess) continue;
            nodes->append(oNode);
        }
    }

    return status;
}

bool HdEvaluator::isHyperdriveNode(MObject oNode, MStatus& status)
{
    status = MS::kSuccess;

    MFnDependencyNode depNodeFn(oNode, &status);
    CHECK_MSTATUS(status);
    if (status != MS::kSuccess) return false;

    MTypeId nodeType = depNodeFn.typeId(&status);
    CHECK_MSTATUS(status);

    if (nodeType == HdCacheNode::HdCacheNode::id || 
    nodeType == HdPoseNode::HdPoseNode::id)
    {
        return true;
    }

    return false;
}

bool HdEvaluator::markIfSupported(const MEvaluationNode* node)
{
    MStatus status; 

    if (!evaluatorInitialized) evaluatorInit();
    return true; // TEST:  SUPPORT ALL NODES

    if (!hdAvailable) return false;

    MObject oNode = node->dependencyNode(&status);
    if (status != MS::kSuccess) return false;

    // check if other nodes are in rig node map and claim if they are
    MFnDependencyNode depNodeFn(oNode, &status);
    if (status != MS::kSuccess) return false;
    unsigned int nodeHash = MObjectHandle::objectHashCode(oNode);

    // claim hyperdrive nodes
    if (isHyperdriveNode(oNode, status) && status == MS::kSuccess)
    {
        log->debug("Claim Hyperdrive node: '{}' ('{}')", depNodeFn.name().asChar(), nodeHash);
        return true;
    } 

    HashMap::iterator it = evalNodeMap.find(nodeHash);
    if (it != evalNodeMap.end()) 
    {
        log->debug("Claim node: '{}' ('{}')", depNodeFn.name().asChar(), nodeHash);
        return true;
    } 

    return false;
}



void HdEvaluator::preEvaluate(const MEvaluationGraph* graph)
{
    // *** RESET EVALUATION CHECK VARIABLES
    cachedPoses.clear();
    fullyCached = false;
    // ***

    if (!hdAvailable) return;
    double frame = HdUtils::getCurrentFrame();

    if (!HdUtils::playbackActive()) 
    {
        log->info("Frame '{}': Playback not active. Evaluate frame.", frame);
        return;
    }

    MStatus status;

    // STOP TIME
    HdUtils::time_point startTime = HdUtils::getCurrentTimePoint();
    double startTimeDouble = HdUtils::timePointToDouble(startTime);

    for (unsigned int i=0; i<poseNodes.length(); i++)
    {
        MObject oNode = poseNodes[i];

        MFnDependencyNode poseNodedepFn(oNode, &status);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        MPlug poseIdPlug = poseNodedepFn.findPlug(HdPoseNode::HdPoseNode::aOutPoseId, &status);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        MPlug rigFreezePlug = poseNodedepFn.findPlug(HdPoseNode::HdPoseNode::aOutFreezeRig, &status);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        int freezeRig;
        MString poseId;

        status = rigFreezePlug.getValue(freezeRig);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        status = poseIdPlug.getValue(poseId);
        CHECK_MSTATUS(status);
        if (status != MS::kSuccess) continue;

        unsigned int poseNodeHash = MObjectHandle::objectHashCode(oNode);

        if (freezeRig) 
        {
            cachedPoses.insert(poseNodeHash);
            log->info("Frame '{}': Pose caches available. Node '{}' / pose ID: '{}'.", frame, poseNodeHash, poseId.asChar());
        } else {
            log->info("Frame '{}': Uncached pose. Node '{}' / pose ID: '{}'. Evaluate.", frame, poseNodeHash, poseId.asChar());
        }
    }

    if (cachedPoses.size() == poseNodes.length())  
    {
        fullyCached = true;
    }

    // CALC EXEC TIME
    HdUtils::time_point endTime = HdUtils::getCurrentTimePoint();
    log->debug("Pre-Eval Exec time: {}", HdUtils::getTimeDiffString(startTime, endTime));
}

void HdEvaluator::postEvaluate(const MEvaluationGraph* graph)
{
}

// called during scheduling
bool HdEvaluator::clusterInitialize (const MCustomEvaluatorClusterNode* cluster)
{
    return true;
}

void HdEvaluator::clusterEvaluate(const MCustomEvaluatorClusterNode* cluster)
{
    MProfilingScope profilingScope(_profilerCategory, MProfiler::kColorD_L1, "Evaluate Hyperdrive cluster.");

    if (hdAvailable && fullyCached) 
    {
        MStatus status = MS::kSuccess;
        MGraphNodeIterator iterator(cluster, &status);
        if (status == MS::kSuccess)
        {
            while (!iterator.isDone())
            {
                iterator.next(&status);

                MEvaluationNode currEvalNode = iterator.currentEvaluationNode(&status);
                MObject oNode = currEvalNode.dependencyNode(&status);

                MFnDependencyNode depNodeFn(oNode, &status);
                if (status != MS::kSuccess) continue;
                std::string nodeName = depNodeFn.name().asChar();

                unsigned int nodeHash = MObjectHandle::objectHashCode(oNode);
                bool isWhitelisted = whitelistNodes.find(nodeHash) != whitelistNodes.end();
                        
                if (isWhitelisted)
                {
                    log->debug("Evaluate whitelisted node: {}", nodeName);
                    cluster->evaluateNode(currEvalNode, &status);
                    CHECK_MSTATUS(status);
                }
                else if (!isHyperdriveNode(oNode, status))
                {
                    if (oNode.hasFn(MFn::kMesh)) 
                    {
                        bool isOutputMesh = outputMeshes.find(nodeHash) != outputMeshes.end();
                        if (!isOutputMesh) continue;

                        log->debug("Evaluate mesh node: {}", nodeName);
                        cluster->evaluateNode(currEvalNode, &status);
                        CHECK_MSTATUS(status);
                    } 
                } else 
                {
                    log->debug("Evaluate Hyperdrive node: {}", nodeName);
                    cluster->evaluateNode(currEvalNode, &status);
                    CHECK_MSTATUS(status);
                }
                continue;
            }
            return;
        }
    }
    
    // evaluate normally
    cluster->evaluate();
}

void HdEvaluator::clusterTerminate(const MCustomEvaluatorClusterNode* cluster)
{
    if (evaluatorInitialized) 
    {
        evaluatorReset();
    }
}

void HdEvaluator::collectOutputMeshes()
{
    MStatus status;
    MItDependencyNodes nodeIt(MFn::kPluginDependNode);
    if (status == MS::kSuccess)
    {
        for (; !nodeIt.isDone(); nodeIt.next()) 
        {
            MFnDependencyNode depNodeFn(nodeIt.thisNode(), &status);
            if (status != MS::kSuccess) continue;

            if (depNodeFn.typeId() != HdCacheNode::HdCacheNode::id) continue;

            std::string cacheNodeName = depNodeFn.name().asChar();

            log->debug("Found HdCacheNode: '{}'", cacheNodeName);

            // GET CONNECTED RIG NODES
            MPlug outMeshesPlug = depNodeFn.findPlug(HdCacheNode::HdCacheNode::aOutMeshes, true, &status);
            CHECK_MSTATUS(status);
            if (status != MS::kSuccess) continue;

            MObjectArray meshNodes;
            status = getNodesFromArrayPlug(outMeshesPlug, &meshNodes, false, true);
            CHECK_MSTATUS(status);
            if (status != MS::kSuccess) continue;

            for (unsigned int i=0; i<meshNodes.length(); i++) 
            {
                MObject oMeshNode = meshNodes[i];
                MFnDependencyNode meshNodeDepFn(oMeshNode, &status);
                CHECK_MSTATUS(status);
                if (status != MS::kSuccess) continue;

                unsigned int outputMeshHash = MObjectHandle::objectHashCode(oMeshNode);
                outputMeshes.insert(outputMeshHash);
                log->debug("Cache Node '{}' - collected output mesh node: '{}' ('{}')", 
                cacheNodeName, meshNodeDepFn.name().asChar(), outputMeshHash);
            }
        }
        log->debug("Total output mesh nodes: {}", outputMeshes.size());
    }
}

void HdEvaluator::collectWhitelistNodes()
{
    MStatus status;
    MItDependencyNodes nodeIt(MFn::kPluginDependNode);
    if (status == MS::kSuccess)
    {
        for (; !nodeIt.isDone(); nodeIt.next()) 
        {
            MFnDependencyNode depNodeFn(nodeIt.thisNode(), &status);
            if (status != MS::kSuccess) continue;

            if (depNodeFn.typeId() != HdPoseNode::HdPoseNode::id) continue;

            std::string poseNodeName = depNodeFn.name().asChar();

            // GET CONNECTED RIG NODES
            MPlug outMeshesPlug = depNodeFn.findPlug(HdPoseNode::HdPoseNode::aInWhitelist, true, &status);
            CHECK_MSTATUS(status);
            if (status != MS::kSuccess) continue;

            MObjectArray oNodes;
            status = getNodesFromArrayPlug(outMeshesPlug, &oNodes, true, false);
            CHECK_MSTATUS(status);
            if (status != MS::kSuccess) continue;

            for (unsigned int i=0; i<oNodes.length(); i++) 
            {
                MObject oNode = oNodes[i];
                MFnDependencyNode poseNodeDepFn(oNode, &status);
                CHECK_MSTATUS(status);
                if (status != MS::kSuccess) continue;

                unsigned int nodeHash = MObjectHandle::objectHashCode(oNode);
                whitelistNodes.insert(nodeHash);
                log->debug("Pose Node '{}' - collected whitelisted node: '{}' ('{}')", 
                poseNodeName, poseNodeDepFn.name().asChar(), nodeHash);
            }
        }
        log->debug("Total whitelisted nodes: {}", whitelistNodes.size());
    }
}


void HdEvaluator::collectHdPoseNodes()
{
    MStatus status;
    MItDependencyNodes nodeIt(MFn::kPluginDependNode);
    if (status == MS::kSuccess)
    {
        for (; !nodeIt.isDone(); nodeIt.next()) 
        {
            MFnDependencyNode depNodeFn(nodeIt.thisNode(), &status);
            if (status != MS::kSuccess) continue;

            if (depNodeFn.typeId() != HdPoseNode::HdPoseNode::id) continue;

            log->debug("Found HdPoseNode: '{}'", depNodeFn.name().asChar());
            poseNodes.append(depNodeFn.object());
        }
        log->debug("Total pose nodes: {}", poseNodes.length());
    }
}