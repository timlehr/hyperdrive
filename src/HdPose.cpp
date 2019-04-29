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

#include "HdPose.h"

HdPose::HdPose(std::string rigTag)
{
    m_rigTag = rigTag;
}

HdPose::~HdPose(){}

std::string HdPose::rigTag() const
{
    return m_rigTag;
}

std::string HdPose::hash() const
{
    size_t hash = std::hash<HdPose>()(*this);
    return std::to_string(hash);
}