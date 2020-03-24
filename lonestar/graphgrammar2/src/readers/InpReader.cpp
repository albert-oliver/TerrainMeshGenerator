#include "InpReader.h"

#include "../model/Coordinates.h"
#include "../model/Graph.h"
#include "../model/Map.h"
#include "../model/NodeData.h"
#include "../utils/ConnectivityManager.h"
#include "../utils/Utils.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using inpEdge = std::pair<size_t, size_t>;

void inpRead(const std::string& filename, Graph& graph, Map& map,
             bool version2D) {
  auto connManager = ConnectivityManager{graph};
  std::ifstream file(filename, std::ios_base::in);

  if (!file.is_open()) {
    // XXX[AOS]: Maybe you prefer to raise an exception or if there's some
    //          function to abort
    std::cerr << "File " << filename << " cannot be opened!" << std::endl;
    exit(EXIT_FAILURE);
  }

  size_t numberOfNodes, numberOfElements, dummy;

  file >> numberOfNodes;
  file >> numberOfElements;
  file >> dummy;
  file >> dummy;
  file >> dummy;

  auto nodes = std::vector<GNode>(numberOfNodes);

  // Read all coordinates and generating the nodes
  for (auto i = 0; i < numberOfNodes; ++i) {
    file >> dummy;
    double x, y, z;
    file >> x;
    file >> y;
    file >> z;
    const auto& coordinates = Coordinates{Utils::convertToUtm(x, y, map), map};

    // XXX[AOS]: I don't know what the first and second false mean...
    //           I assume one is hanging-node, but the other, no idea...
    //           Maybe we need to change them.
    nodes.push_back(
        connManager.createNode(NodeData{false, coordinates, false}));
  }

  // Containers for edges
  // Since the same edge can be in two triangles, we need to make sure to create
  // only one
  auto edgeSet = std::set<inpEdge>{};
  // We will keep track if one edge is on the boundary or not
  auto isEdgeBoundary = std::map<inpEdge, bool>{};

  // Read elements, create interiors, and populate edgeSet and isEdgeBoundary
  for (auto i = 0; i < numberOfElements; ++i) {
    file >> dummy;
    file >> dummy;
    std::string dummy_str;
    file >> dummy_str;
    size_t conc1, conc2, conc3;
    file >> conc1;
    file >> conc2;
    file >> conc3;

    connManager.createInterior(nodes[conc1], nodes[conc2], nodes[conc3]);

    const auto edges = std::vector<inpEdge>{
        inpEdge{std::min(conc1, conc2), std::max(conc1, conc2)},
        inpEdge{std::min(conc2, conc3), std::max(conc2, conc3)},
        inpEdge{std::min(conc3, conc1), std::max(conc3, conc1)}};

    for (const auto edge : edges) {
      if (edgeSet.insert(edge).second) {
        // First time it's inserted we assume it's on the boundary.
        isEdgeBoundary[edge] = true;
      } else {
        // If we try to insert it again, it means that the edge is shared by two
        // elements, therefore is not on the boundary
        isEdgeBoundary[edge] = false;
      }
    }
  }

  // Finally, generate edges
  for (const auto edge : edgeSet) {
    auto node1 = nodes[edge.first];
    auto node2 = nodes[edge.second];

    const auto node1Coords = node1->getData().getCoords();
    const auto node2Coords = node2->getData().getCoords();

    auto midPointCoords = (node1Coords + node2Coords) / 2.;
    // Change z coordinate to the map height
    midPointCoords.setZ(
        map.get_height(midPointCoords.getX(), midPointCoords.getY()));

    const auto length = node1Coords.dist(node2Coords, version2D);

    connManager.createEdge(node1, node2, isEdgeBoundary.at(edge),
                           midPointCoords, length);
  }

  file.close();
}
