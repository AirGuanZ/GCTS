#pragma once

#include <agz/utility/alloc.h>

#include "graph.h"
#include "patchHistory.h"

GCTS_BEGIN

struct Texel
{
    int patchIndex   = -1; // covered by which patch
    int xPosSeamCost = 0;  // seam with +x neighbor
    int yPosSeamCost = 0;  // seam with +y neighbor

    bool keepXPosSeam = false;
    bool keepYPosSeam = false;

    Vertex texelVertex;
};

class GraphBuilder
{
public:

    Graph build(
        Image2D<Texel>         &texels,
        const ImageView2D<RGB> &patch,
        const Int2             &patchBeg,
        const Int2             &patchOverlapSize,
        const PatchHistory     &patchHistory);

private:

    using EdgeArena   = agz::alloc::fixed_obj_arena_t<Edge>;
    using VertexArena = agz::alloc::fixed_obj_arena_t<Vertex>;

    enum class Region
    {
        Nil,
        Old,
        New,
        Overlap
    };

    Image2D<Region> buildRegionDistribution(
        Image2D<Texel>         &texels,
        const ImageView2D<RGB> &patch,
        const Int2             &patchBeg,
        const Int2             &patchOverlapSize) const;

    int computeSeamCost(
        const RGB &As, const RGB &At,
        const RGB &Bs, const RGB &Bt) const;

    int computeSeamCost(
        int AIndex, int BIndex,
        const Int2 &s, const Int2 &t,
        const PatchHistory &patches) const;

    // returns: (seam vertex, seam src vertex)
    std::pair<Vertex *, Vertex *> addSeam(
        const Int2 &aPos, Texel &aTexel, Vertex *aVertex,
        const Int2 &bPos, Texel &bTexel, Vertex *bVertex,
        int seamCost, Vertex::Type seamVertexType,
        const PatchHistory &patches);

    void addEdge(
        const Int2 &aPos, Texel &aTexel, Vertex *aVertex,
        const Int2 &bPos, Texel &bTexel, Vertex *bVertex,
        const PatchHistory &patches,
        bool isVertical);

    void handleNeighbors(
        std::set<Vertex*> &allVertices,
        const Int2 &aPos, Texel &aTexel, Region aRegion,
        const Int2 &bPos, Texel &bTexel, Region bRegion,
        const PatchHistory &patches);

    EdgeArena   edgeArena_;
    VertexArena vertexArena_;
};

GCTS_END
