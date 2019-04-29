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

#include "HdCacheNode.h"

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
#include <maya/MFloatPointArray.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MEvaluationNode.h>
#include <maya/MUuid.h>

#include "HdUtils.h"

MTypeId HdCacheNode::id(0x00151216);
MObject HdCacheNode::aInMeshes;
MObject HdCacheNode::aOutMeshes;
MObject HdCacheNode::aInCacheId;
MObject HdCacheNode::aInPoseId;

HdCacheNode::HdCacheNode(): currentPoseValid(false), lastPoseId(""){}
HdCacheNode::~HdCacheNode(){}

void HdCacheNode::postConstructor()
{   
    MStatus status = MStatus::kSuccess;

    // setup logger
    std::string loggerName(typeName().asChar());
    log = HdUtils::getLoggerInstance(loggerName);

    // set icon
    MFnDependencyNode nodeDepFn(thisMObject());
    nodeDepFn.setIcon("hyperdriveCache.png");
}

HdCacheNode::SchedulingType HdCacheNode::schedulingType() const
{
    return HdCacheNode::kParallel;
}

std::shared_ptr<HdMeshCache> HdCacheNode::getMeshCache(std::string cacheId, MStatus &status)
{   
    log->debug("Get Cache for cache ID: {}", cacheId);
    std::shared_ptr<HdMeshCache> cache_ptr = HdCacheMap::get(cacheId, status);
    CHECK_MSTATUS(status);
    return cache_ptr;
}

std::string HdCacheNode::getCacheId(MDataBlock& data, MStatus& status)
{
    MPlug plug(thisMObject(), aInCacheId);

    if (!plug.isConnected()) 
    {
        log->error("No connection to 'inCacheId'. Could not retrieve valid cache ID.");
        status = MS::kFailure;
        return nullptr;
    } else 
    {
        MDataHandle hInCacheNodeId = data.inputValue(aInCacheId, &status);
        CHECK_MSTATUS(status);
        std::string cacheId = hInCacheNodeId.asString().asChar();
        if (cacheId.length() < 1) 
        {
            log->error("Invalid cache ID retrieved from 'inCacheId': '{}'", cacheId);
            status = MS::kInvalidParameter;
            return nullptr;
        }
        status = MS::kSuccess;
        return cacheId;
    }
}

MStatus HdCacheNode::skipCompute(const MPlug& plug, MDataBlock& data) 
{
    MStatus status = MS::kSuccess;
        log->debug("Skip compute. Passthrough in-meshes.");
    status = setOutMeshes(data, nullptr, "", true);
    CHECK_MSTATUS(status);
    return status;
}

MStatus HdCacheNode::preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode)
{
    // Only check pose validity in normal content
    // Reference: https://knowledge.autodesk.com/search-result/caas/CloudHelp/cloudhelp/2017/ENU/Maya-SDK/py-ref/class-open-maya-1-1-m-d-g-context-html.html
    currentPoseValid = false;
    hdDisabled = false;
    
    if( context.isNormal() ) 
    {
        MStatus status;
        if(evaluationNode.dirtyPlugExists(aInPoseId, &status) && status)
        {   
            MPlug plug = evaluationNode.dirtyPlug(aInPoseId, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            std::string newPoseId = plug.asString().asChar();
            log->debug("Pre-Eval - Dirty plug: {} // New Pose ID: '{}'", plug.info().asChar(), newPoseId);
            
            // Set current pose validation state
            currentPoseValid = ((lastPoseId == newPoseId) && (newPoseId != ""));
            hdDisabled = (newPoseId == "");
            log->debug("Pre-Eval - Current Pose Valid: {}", currentPoseValid);
        } else {
            currentPoseValid = true;
        }
    }

    needsEvaluation = !HdUtils::playbackActive();
    log->debug("Needs evaluation: {}", needsEvaluation);

    return MS::kSuccess;
}

MStatus HdCacheNode::setDependentsDirty(MPlug const & inPlug, MPlugArray  & affectedPlugs)
    {
        if ( inPlug.attribute() != aInPoseId) {
            return MS::kSuccess;
        }

        log->debug("inPoseId dirty. Set outMeshes dirty.");

        MPlug outMeshesPlug(thisMObject(), aOutMeshes);

        // Mark the parent output plug as dirty.
        affectedPlugs.append(outMeshesPlug);

        // Mark each mesh output
        unsigned int i,n = outMeshesPlug.numElements();
        for (i = 0; i < n; i++) {
            MPlug elemPlug = outMeshesPlug.elementByPhysicalIndex(i);
            affectedPlugs.append(elemPlug);
        }
        return MS::kSuccess;
    }

MStatus HdCacheNode::compute(const MPlug& plug, MDataBlock& data)
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

    if (currentPoseValid && !needsEvaluation){
        // skip compute since pose did not change since last eval
        log->debug("Current Pose ID identical to last. Skip compute for plug: {}", plug.info().asChar());
        return MS::kSuccess;
    }

    log->debug("Dirty plug: {}", plug.info().asChar());

    if (plug != aOutMeshes || plug.isElement())
    {
        // check the plug (internal attribute) if it's requesting an unknown attribute
        log->debug("Ignore plug: {}", plug.info().asChar());
        return MS::kUnknownParameter;
    } 

    // STOP TIME
    HdUtils::time_point startTime = HdUtils::getCurrentTimePoint();
    double startTimeDouble = HdUtils::timePointToDouble(startTime);
    log->debug("Compute plug: {} ", plug.info().asChar());

    MDataHandle stateData = data.inputValue( state, &status );
    CHECK_MSTATUS(status);

    std::string cacheId = getCacheId(data, status);
    CHECK_MSTATUS(status);

     // get mesh cache object
    std::shared_ptr<HdMeshCache> meshCache = getMeshCache(cacheId, status);
    CHECK_MSTATUS(status);


    // **********************************************
    // CHECK IF CACHING OR BYPASSING
    // **********************************************

    if (stateData.asShort() == 1 || 
        meshCache == nullptr || 
        needsEvaluation || 
        hdDisabled ||
        status != MS::kSuccess) 
    {       
       log->warn("Bypass cache node. Forced evaluation or invalid cache. Cache ID: '{}'", cacheId);
       status = skipCompute(plug, data);
       CHECK_MSTATUS(status);
       logExecutionTime(startTime);
       return status;
    } 
    
    MPlug cacheIdPlug = MPlug(thisMObject(), aInCacheId);
    MPlug poseIdPlug = MPlug(thisMObject(), aInPoseId);
    
    if (!cacheIdPlug.isConnected() || !poseIdPlug.isConnected())
    {
        log->warn("Bypass cache node. 'inCacheId' and / or 'inPoseId' not connected.");
        status = skipCompute(plug, data);
        CHECK_MSTATUS(status);
        logExecutionTime(startTime);
        return status;
    }
    

    // ***********************************************
    // GET CACHE, CREATE POSE AND SET DEBUG HASH ATTR
    // ***********************************************

   

    MDataHandle hPoseId = data.inputValue(aInPoseId, &status);
    std::string poseId = hPoseId.asString().asChar();

    log->debug("Compute cache for Pose ID: {}", poseId);

    if(!meshCache->exists(poseId)) // ... if there is no cache for the current Pose
    {

        // Since there is none, create a new cache data object for this pose
        // Note: This needs to be after the node state has been changed to guarantee proper results.
        std::shared_ptr<HdMeshSet> meshSetPtr = createCacheMeshData(data, meshCache, status);

        status = setOutMeshes(data, meshCache, poseId, true);
        CHECK_MSTATUS(status);
        
        if(meshSetPtr) // ... if there is a valid mesh, store it
        {
            meshCache->put(poseId, meshSetPtr);
            log->info("Stored new pose cache. Pose ID: {} (Cache Size: '{}')", poseId, meshCache->size());
        }
    } 
    else 
    {   
        log->debug("Retrieve Pose Cache: {}", poseId);
        status = setOutMeshes(data, meshCache, poseId, false);
        CHECK_MSTATUS(status);
    }

    // remove dirty so it won't be recalculated
    data.setClean(plug); 

    logExecutionTime(startTime);
    
    return MS::kSuccess;
}

std::shared_ptr<HdMeshSet> HdCacheNode::createCacheMeshData(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, MStatus& status) {
    // Create new mesh set
    HdMeshSet meshSet;

    // IN MESHES HANDLE
    MArrayDataHandle hInMeshes = data.inputArrayValue(aInMeshes, &status);
    CHECK_MSTATUS(status);

    int meshesCount = hInMeshes.elementCount(&status);
    CHECK_MSTATUS(status);

    if(meshesCount > 0)
    {
        for(int i=0; i<meshesCount; i++)
        {
            status = hInMeshes.jumpToElement(i);
            CHECK_MSTATUS(status);

            // get Mesh data
            MDataHandle hInMesh = hInMeshes.inputValue(&status);
            CHECK_MSTATUS(status);

            MObject oInMesh = hInMesh.asMesh();

            if(oInMesh.isNull()) // integrity check
            {
                // no valid mesh on this plug - abort caching.
                log->warn("No valid input mesh at plug index {}.", i);
                status = MS::kInvalidParameter;
                return nullptr;
            } 

            MFnMesh inMesh(oInMesh, &status);
            MFloatPointArray points;
            MFloatVectorArray normals;
            MIntArray polyVertCounts;
            MIntArray polyVertConnections;

            int totalPolyCount = inMesh.numPolygons(&status);
            int totalVertCount = inMesh.numVertices(&status);

            //CHECK_MSTATUS(inMesh.getNormals(normals));
            // TODO: !!! NEEDS TO BE UNSHARED NORMALS FOR HARD EDGES -> FACEVERTEXNORMALS !!!
            //std::cout << "Normals: " << normals.length() << endl;

            HdMeshData meshData(totalVertCount, totalPolyCount);

            // POINTS
            status = inMesh.getPoints(points);
            CHECK_MSTATUS(status);
            meshData.points = std::make_shared<MFloatPointArray>(points);

            // VERT ARRAYS
            CHECK_MSTATUS(inMesh.getVertices(polyVertCounts, polyVertConnections));
            meshData.polyVertCounts = std::make_shared<MIntArray>(polyVertCounts);
            meshData.polyVertConnections = std::make_shared<MIntArray>(polyVertConnections);

            // CACHE MOBJECT FOR LATER
            MObject mCopy = MObject(oInMesh);
            meshData.mayaObject = std::make_shared<MObject>(mCopy);

            meshSet.push_back(meshData);
        }
        return std::make_shared<HdMeshSet>(meshSet);
    } 
    return nullptr; // no valid cache could be created for this pose (missing in-meshes?)
}

MStatus HdCacheNode::loadMeshDataFromCache(std::shared_ptr<HdMeshCache> meshCache, MObject* oMesh, HdMeshData* meshDataPtr) 
{
    MStatus status;
    MFnMesh fnMesh(*oMesh);
    log->debug("Retrieved Points array size: {}", (*meshDataPtr->points).length());

    MPointArray vertices;
    MIntArray polyVertCounts;
    MIntArray polyVertConnections;

    MFloatVectorArray normals;
    
    fnMesh.create(
        meshDataPtr->totalVertCount, 
        meshDataPtr->totalPolyCount,
        *(meshDataPtr->points),
        *(meshDataPtr->polyVertCounts),
        *(meshDataPtr->polyVertConnections),
        *oMesh, // pass mesh MObject so Maya won't create a new one!
        &status
    );
    CHECK_MSTATUS(status);

    // restore normals
    // status = fnMesh.setNormals(normals);
    // CHECK_MSTATUS(status);

    return MS::kSuccess;
}

MStatus HdCacheNode::setOutMeshData(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, std::string poseId, int meshElementIndex, bool noEffect)
{
    MStatus status = MS::kSuccess;
    
    // OUT MESHES HANDLE
    MArrayDataHandle hOutMeshes = data.outputArrayValue(aOutMeshes, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    unsigned int outMeshCount = hOutMeshes.elementCount(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // INTEGRITY CHECK
    if (meshElementIndex >= outMeshCount) // (inMeshCount <= 0) || (meshElementIndex >= inMeshCount)
    {
        log->warn("Integrity check failed. Mesh Element at index {} out of array size {}.", meshElementIndex, outMeshCount);
        return MS::kFailure;
    }

    // ACTUALLY SET OUT MESHES FROM HERE ON ...

    status = hOutMeshes.jumpToElement(meshElementIndex);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDataHandle hOutMesh = hOutMeshes.outputValue(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (noEffect) // pass cache retrieval
    {
        // IN MESHES HANDLE
        MArrayDataHandle hInMeshes = data.outputArrayValue(aInMeshes, &status);
        // use outputArrayValue to prevent evaluation of inMeshes! ....
        CHECK_MSTATUS_AND_RETURN_IT(status);

        unsigned int inMeshCount = hInMeshes.elementCount(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if ((inMeshCount <= 0) || (meshElementIndex >= inMeshCount))
        {
            return MS::kNotFound;
        }

        status = hInMeshes.jumpToElement(meshElementIndex);
        MDataHandle hInMesh = hInMeshes.outputValue(&status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // SET OUTMESH DATA
        MObject oInMesh = hInMesh.asMesh();
        hOutMesh.set(oInMesh);
        hOutMesh.setClean();

        log->debug("Cache Node disabled. Passthrough meshes.");
        return MS::kSuccess;
    } 

    // ... IF HYPERDRIVE ACTIVE

    if(meshCache != nullptr && !meshCache->exists(poseId)) 
    {
        return MS::kNotFound;
    }

    // RETRIEVE CACHE
    std::shared_ptr<HdMeshSet> meshSetPtr = meshCache->get(poseId, status, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // cache integrity check
    if (meshElementIndex >= meshSetPtr->size())
    {
        return MS::kEndOfFile;
    } 

    // CHECK IF CACHED MOBJECT IS STILL VALID
    HdMeshData meshData = (*meshSetPtr)[meshElementIndex];
    if (meshData.mayaObject != nullptr && !(*meshData.mayaObject).isNull())
    {
        log->debug("Use existing MObject for ID: {}", poseId);
        hOutMesh.set(*meshData.mayaObject);
        hOutMesh.setClean();

        return status;
    }

    // INVALID MOBJECT, CONSTRUCT A NEW ONE
    log->debug("Cached MObject invalid. Reconstruct mesh for ID: {}", poseId);
    MFnMeshData fnMeshData;
    MObject oOutMesh = fnMeshData.create();
    status = loadMeshDataFromCache(meshCache, &oOutMesh, &meshData);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    
    hOutMesh.set(oOutMesh);
    hOutMesh.setClean();

    return status;
}

MStatus HdCacheNode::setOutMeshes(MDataBlock& data, std::shared_ptr<HdMeshCache> meshCache, 
                                  std::string poseId, bool noEffect)
{
    MStatus status = MS::kSuccess;
 
    // CACHED MESHES DATA
    std::shared_ptr<HdMeshSet> meshSetPtr = nullptr;
    
    // IN MESHES HANDLE
    MArrayDataHandle hInMeshes = data.outputArrayValue(aInMeshes, &status);
    // use outputArrayValue to prevent evaluation of inMeshes! ....
    CHECK_MSTATUS_AND_RETURN_IT(status);
 
    unsigned int inMeshCount = hInMeshes.elementCount(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    
    // OUT MESHES HANDLE
    MArrayDataHandle hOutMeshes = data.outputArrayValue(aOutMeshes, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
 
    unsigned int outMeshCount = hOutMeshes.elementCount(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
 
    // ACTUALLY SET OUT MESHES FROM HERE ON ...
    if (inMeshCount != outMeshCount)
    {
        log->warn("Cache Node '{}' has unequal inMeshes / outMeshes connections.", 
                  HdUtils::getNodeName(thisMObject()));
    }
 
    if ( inMeshCount > 0) 
    {
        for (int i=0; i < inMeshCount; i++)
        {
            // array integrity check
            if (i >= outMeshCount) {
                log->warn("Index {} is out of bounds of outMeshes count {}.", i, outMeshCount);
                return status;
            }
            log->debug("Set out mesh for index: {}", i);
            status = setOutMeshData(data, meshCache, poseId, i, noEffect);
        }

        hOutMeshes.setClean();
        hOutMeshes.setAllClean();
    }
    return status;
}

void HdCacheNode::logExecutionTime(HdUtils::time_point startTime)
{
    HdUtils::time_point endTime = HdUtils::getCurrentTimePoint();
    double endTimeDouble = HdUtils::timePointToDouble(endTime);
    log->debug("Exec Time: {}", HdUtils::getTimeDiffString(startTime, endTime));
}



void* HdCacheNode::creator()
{
    return new HdCacheNode();
}

MStatus HdCacheNode::initialize()
{
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // INPUT - CACHEID
    aInCacheId = tAttr.create("inCacheId", "inCacheId", MFnData::kString);
    tAttr.setWritable(true); // enable input
    tAttr.setStorable(false); 
    tAttr.setReadable(false); // disable output
    tAttr.setHidden(false);
    addAttribute(aInCacheId);
    attributeAffects(aInCacheId, aOutMeshes);

    // INPUT - MESHES
    aInMeshes = tAttr.create("inMeshes", "inMeshes", MFnData::kMesh);
    tAttr.setKeyable(true); // has to be true to be visible in node editor
    tAttr.setStorable(false);
    tAttr.setReadable(false); // disable output
    tAttr.setArray(true);
    addAttribute(aInMeshes);

    // INPUT - POSE ID
    aInPoseId = tAttr.create("inPoseId", "inPoseId",MFnData::kString);
    tAttr.setKeyable(true); // has to be true to be visible in node editor
    tAttr.setStorable(false);
    tAttr.setReadable(false); // disable output
    addAttribute(aInPoseId);
    attributeAffects(aInPoseId, aOutMeshes);

    // OUTPUT - MESHES
    aOutMeshes = tAttr.create("outMeshes", "outMeshes", MFnData::kMesh);
    tAttr.setWritable(false); // disable input
    tAttr.setStorable(true);
    tAttr.setHidden(false);
    tAttr.setArray(true);
    addAttribute(aOutMeshes);

    return MS::kSuccess;
}