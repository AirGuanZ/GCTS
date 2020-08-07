#pragma once

#include <array>
#include <set>

#include "synthesizer.h"

GCTS_BEGIN

struct Edge;

struct Vertex
{
    enum Type
    {
        DummySrc,
        Texel,
        HoriSeam,
        VertSeam,
    };

    bool isSrc  = false;
    bool isSink = false;

    Edge *bfsAccesser = nullptr;

    Type type = Texel;
    Int2 position;

    static constexpr int EDGE_POS_X = 0;
    static constexpr int EDGE_NEG_X = 1;
    static constexpr int EDGE_POS_Y = 2;
    static constexpr int EDGE_NEG_Y = 3;
    static constexpr int EDGE_HORI_SEAM_TO_SRC = 2;
    static constexpr int EDGE_VERT_SEAM_TO_SRC = 0;

    std::array<Edge *, 4> edges = { nullptr, nullptr, nullptr, nullptr };
};

struct Edge
{
    Vertex *a = nullptr;
    Vertex *b = nullptr;

    int capacity = 0;
    int a2bFlow  = 0;

    int a2bRes() const noexcept { return capacity - a2bFlow; }
    int b2aRes() const noexcept { return capacity + a2bFlow; }
};

class Graph
{
public:

    struct MinCutResult
    {
        std::set<Vertex *> reachableVertices;
        std::set<Edge *> cut;
    };

    explicit Graph(std::set<Vertex *> allVertices) noexcept;

    MinCutResult findMinCut();

private:

    int runMaxFlowIteration();

    std::set<Vertex *> vtces_;
    std::set<Vertex *> srcs_;
};

GCTS_END
