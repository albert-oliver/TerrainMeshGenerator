#ifndef GALOIS_PRODUCTION3_H
#define GALOIS_PRODUCTION3_H

#include "Production.h"
#include "../utils/ConnectivityManager.h"
#include "../utils/utils.h"

class Production3 : public Production {
private:

    bool checkApplicabilityCondition(const std::vector<optional<EdgeIterator>> &edgesIterators) const {
        return connManager.countBrokenEdges(edgesIterators) == 1;
    }

public:

    using Production::Production;

    bool execute(ProductionState &pState, galois::UserContext<GNode> &ctx) override {
        if (!checkApplicabilityCondition(pState.getEdgesIterators())) {
            return false;
        }

        int brokenEdge = pState.getAnyBrokenEdge();
        assert(brokenEdge != -1);

        if (checkIfBrokenEdgeIsTheLongest(brokenEdge, pState.getEdgesIterators(), pState.getVertices(),
                                          pState.getVerticesData())) {
            return false;
        }

//        logg(pState.getInteriorData(), pState.getVerticesData());

        const vector<int> &longestEdges = pState.getLongestEdges();

        int le = -1;
        bool found = false;
        for (int longest : longestEdges) {
            if (pState.getEdgesData()[longest].get().isBorder()) {
                le = longest;
                found = true;
                break;
            }
        }
        if (!found) {
            for (int longest : longestEdges) {
                if (!pState.getVerticesData()[getEdgeVertices(longest).first].isHanging() &&
                    !pState.getVerticesData()[getEdgeVertices(longest).second].isHanging()) {
                    le = longest;
                    break;
                }
            }
        }

        const std::pair<GNode, GNode> &pair = breakElementWithoutHangingNode(le, pState, ctx);

        for (auto edge : connManager.getTriangleEdges(pair.first)) {
            if (edge.get() == pState.getEdgesIterators()[brokenEdge].get()) {
                ctx.push(pair.first);
                return true;
            }
        }
        for (auto edge : connManager.getTriangleEdges(pair.second)) {
            if (edge.get() == pState.getEdgesIterators()[brokenEdge].get()) {
                ctx.push(pair.second);
                return true;
            }
        }

//                std::cout << "P3 executed ";

    }

};

#endif //GALOIS_PRODUCTION3_H
