/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_COMMANDS_H
#define HD_COMMANDS_H

#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>

class HdCmdCache : public MPxCommand
{
    public:
                                HdCmdCache();
                                ~HdCmdCache();
        MStatus                 doIt( const MArgList& args);
        static void*            creator();
};

class HdCmdStats : public MPxCommand
{
    public:
                                HdCmdStats();
                                ~HdCmdStats();
        MStatus                 doIt( const MArgList& args);
        static void*            creator();
};

class HdCmdLog : public MPxCommand
{
    public:
                                HdCmdLog();
                                ~HdCmdLog();
        MStatus                 doIt( const MArgList& args);
        static void*            creator();
};

#endif