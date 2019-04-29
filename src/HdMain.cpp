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
#include "HdCacheNode.h"
#include "HdPoseNode.h"
#include "HdCommands.h"
#include "HdEvaluator.h"

#include <maya/MFnPlugin.h>
#include "spdlog/spdlog.h"

const char* hd_version = "0.1 alpha";

MStatus initializePlugin(MObject obj)
{
    // LOGGER SETUP
    spdlog::set_level(spdlog::level::debug);

    MStatus status;
    MFnPlugin fnPlugin(obj, "Tim Lehr", hd_version, "Any");

    // register Evaluator
    status = fnPlugin.registerEvaluator("hdEvaluator", 2000000, HdEvaluator::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Cache Command
    status = fnPlugin.registerCommand("hdCache", HdCmdCache::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Stats Command
    status = fnPlugin.registerCommand("hdStats", HdCmdStats::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Log Command
    status = fnPlugin.registerCommand("hdLog", HdCmdLog::creator);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Cache Node
    status = fnPlugin.registerNode("hyperdriveCache", 
    HdCacheNode::id, 
    HdCacheNode::creator,
    HdCacheNode::initialize);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Pose Node
    status = fnPlugin.registerNode("hyperdrivePose", 
    HdPoseNode::id, 
    HdPoseNode::creator,
    HdPoseNode::initialize);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    std::cout << std::endl << 
    "##################################" << std::endl <<
    "HYPERDRIVE v" << hd_version << std::endl <<
    "##################################" << std::endl;

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj);

    // deregister custom evaluator
    status =  fnPlugin.deregisterEvaluator("hdEvaluator");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Cache Command
    status = fnPlugin.deregisterCommand("hdCache");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Status Command
    status = fnPlugin.deregisterCommand("hdStatus");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Stats Command
    status = fnPlugin.deregisterCommand("hdStats");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Log Command
    status = fnPlugin.deregisterCommand("hdLog");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Cache Node
    status = fnPlugin.deregisterNode(HdCacheNode::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Hyperdrive Pose Node
    status = fnPlugin.deregisterNode(HdPoseNode::id);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    HdCacheMap::clearMap();
    return MS::kSuccess;
}