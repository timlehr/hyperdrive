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

#include "HdCommands.h"
#include "HdMeshCache.h"

#include <maya/MGlobal.h>
#include <maya/MObject.h>

HdCmdCache::HdCmdCache(){}
HdCmdCache::~HdCmdCache(){}
HdCmdStats::HdCmdStats(){}
HdCmdStats::~HdCmdStats(){}
HdCmdLog::HdCmdLog(){}
HdCmdLog::~HdCmdLog(){}

void* HdCmdCache::creator()
{
    // Maya internal function used to allocate memory etc.
    return new HdCmdCache;
}

MStatus HdCmdCache::doIt( const MArgList& args )
{
    MStatus status;
    MString help("Usage: \"hdCache [cache_id] -myFlag\"\n\n " \
    "Available flags:\n" \
    "hdCache some-cache-id -clear\n" \
    "hdCache some-cache-id -setMaxMemSize 1024000");
    std::shared_ptr<HdMeshCache> meshCache;
    // Parse the arguments.

    if (args.length() < 2) 
    {   
        displayError(MString("Invalid arguments.\n\n") + help);
        return MS::kFailure;
    }

    std::string cacheId = args.asString( 0, &status ).asChar();
    

    if (status != MS::kSuccess || !HdCacheMap::exists(cacheId)) 
    {
        std::string msg = "Invalid / unknown cache ID: '" + cacheId + "'";
        displayError(MString(msg.c_str()));
        return MS::kFailure;
    } 

    meshCache = HdCacheMap::get(cacheId, status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for ( int i = 1; i < args.length(); i++ )
    {
        if ( MString( "-clear" ) == args.asString( i, &status ) && MS::kSuccess == status )
        {
            meshCache->clear();
        }
        else if ( MString( "-setMaxMemSize" ) == args.asString( i, &status ) && MS::kSuccess == status )
        {
            double size = args.asDouble( ++i, &status );
            if ( MS::kSuccess == status )
                meshCache->setMaxMemSize(size);
        }
        else
        {
            displayError(MString("Invalid arguments.\n\n") + help);
            return MS::kFailure;
        }
    }
    return MS::kSuccess;
}

void* HdCmdStats::creator()
{
    // Maya internal function used to allocate memory etc.
    return new HdCmdStats;
}

MStatus HdCmdStats::doIt( const MArgList& args )
{
    MStatus status;
    MString help("Usage: \"hdStats -json\"\n\n " \
    "Available flags:\n" \
    "hdStats -json");

     // Parse the arguments.
    for ( int i = 0; i < args.length(); i++ )
    {
        if ( MString( "-json" ) == args.asString( i, &status ) && MS::kSuccess == status )
        {
            MString result(HdCacheMap::getStatsJson().c_str());
            setResult(result);
        } else
        {
            displayError( MString("Invalid arguments.\n\n") + help );
            return MS::kFailure;
        }
    }
    return MS::kSuccess;
}

void* HdCmdLog::creator()
{
    // Maya internal function used to allocate memory etc.
    return new HdCmdLog;
}

MStatus HdCmdLog::doIt( const MArgList& args )
{
    MStatus status;
    MString help("Usage: \"hdLog -verbosity 3\"\n\n " \
    "Available flags:\n" \
    "hdLog -verbosity [0 - 3]\n"\
    "0 -> off\n"\
    "1 -> errors / warnings\n"\
    "2 -> info\n"\
    "3 -> debug\n"\
    "hdLog -verbosity [0 - 3]");

     // Parse the arguments.
    if (args.length() == 2) 
    {
        if ( MString( "-verbosity" ) == args.asString( 0, &status ) && 
        MS::kSuccess == status ) 
        {
            int level = args.asInt(1, &status );
            CHECK_MSTATUS_AND_RETURN_IT(status);
            HdUtils::setLogLevel(level);
            return MS::kSuccess;
        }
    }

    status = MS::kInvalidParameter;
    displayError(MString("Invalid arguments.\n\n") + help);
    return MS::kSuccess;
}