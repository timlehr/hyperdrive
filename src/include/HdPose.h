/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_POSE_H
#define HD_POSE_H

#include <vector>
#include <string>
#include "HdUtils.h"


static const double FLOATING_POINT_HASH_FIX_SCALE(4096.0); // Disney scale for fixing floating point hashing

class HdPose : public std::vector<double>
{
    private:
        std::string     m_rigTag;

    public:
                        HdPose(std::string rigTag);
        virtual         ~HdPose();
        std::string     rigTag() const;
        std::string     hash() const;
};
    
namespace std 
{
    template <> struct hash<HdPose>
    {
        inline size_t operator()(const HdPose &pose) const
        {
            uint64_t hash_key(hash<string>()(pose.rigTag()));
            const size_t size = pose.size();
            for (size_t i=0; i<size; ++i)
            {
                const double ctrlValue = pose[i] * FLOATING_POINT_HASH_FIX_SCALE;
                HdUtils::hash_combine(hash_key, ctrlValue);
            }
            return hash_key;
        }
    };

}
#endif