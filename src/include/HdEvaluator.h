/* * -----------------------------------------------------------------------------
 * This source file has been developed within the scope of the
 * Technical Director course at Filmakademie Baden-Wuerttemberg.
 * http://technicaldirector.de
 *
 * Written by Tim Lehr
 * Copyright (c) 2019 Animationsinstitut of Filmakademie Baden-Wuerttemberg
 * -----------------------------------------------------------------------------
 */

#ifndef HD_EVALUATOR_H
#define HD_EVALUATOR_H

#include <map>
#include <set>
#include <maya/MPxCustomEvaluator.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MObjectArray.h>
#include <maya/MProfiler.h>

#include "spdlog/spdlog.h"

class HdEvaluator : public MPxCustomEvaluator
{
    public:
        typedef std::map<unsigned int, unsigned int>                HashMap;
        typedef std::map<unsigned int, std::vector<std::string>>    StringMap;

                            HdEvaluator();
        virtual             ~HdEvaluator();

        static void*        creator();
        virtual const char* evaluatorName();
        bool                markIfSupported(const MEvaluationNode* node) override;
        
        void                preEvaluate         (const MEvaluationGraph* graph) override;
        void                postEvaluate        (const MEvaluationGraph* graph) override;
        bool                clusterInitialize   (const MCustomEvaluatorClusterNode* cluster) override;
        void                clusterEvaluate     (const MCustomEvaluatorClusterNode* cluster) override;
        void                clusterTerminate    (const MCustomEvaluatorClusterNode* cluster) override;

        void                evaluatorInit();
        void                evaluatorReset();
        void                collectHdPoseNodes();
        void                collectOutputMeshes();
        void                collectWhitelistNodes();

        MStatus             getNodesFromArrayPlug(MPlug arrayPlug, MObjectArray* nodes, bool asDst, bool asSrc);
        bool                isHyperdriveNode(MObject oNode, MStatus& status);

    private:
        std::shared_ptr<spdlog::logger> log;
        HashMap                         evalNodeMap;

        std::set<unsigned int>          cachedPoses;
        std::set<unsigned int>          outputMeshes;
        std::set<unsigned int>          whitelistNodes;
        
        MObjectArray                    poseNodes;

        bool                            hdAvailable = false;
        bool                            fullyCached = false;
        bool                            evaluatorInitialized = false;
};


#endif