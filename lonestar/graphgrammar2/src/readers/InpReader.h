#ifndef INP_READER_H
#define INP_READER_H

#include "../model/Graph.h"
#include "../model/Map.h"

#include <string>

void inpRead(const std::string& filename, Graph& graph, Map& map, bool version2D);

#endif // INP_READER_H
