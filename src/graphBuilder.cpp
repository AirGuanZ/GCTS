#include "graphBuilder.h"

GCTS_BEGIN

Graph GraphBuilder::build(
    Image2D<Texel>         &texels,
    const ImageView2D<RGB> &patch,
    const Int2             &patchBeg,
    const Int2             &patchOverlapSize,
    const PatchHistory     &patchHistory)
{
    // clear texel vertices

    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            auto &t = texels(y, x);
            t.texelVertex          = {};
            t.texelVertex.position = { x, y };
        }
    }

    const auto regions = buildRegionDistribution(
        texels, patch, patchBeg, patchOverlapSize);

    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            if(regions(y, x) == Region::New)
                texels(y, x).patchIndex = patchHistory.getCurrentIndex();
        }
    }

    // create vertices and edges

    std::set<Vertex *> allVertices;

    for(int y = 0; y < texels.height() - 1; ++y)
    {
        for(int x = 0; x < texels.width() - 1; ++x)
        {
            const Region cenRegion = regions(y, x);
            const Region xPosRegion = regions(y, x + 1);
            const Region yPosRegion = regions(y + 1, x);

            Texel &cenTexel = texels(y, x);
            Texel &xPosTexel = texels(y, x + 1);
            Texel &yPosTexel = texels(y + 1, x);

            handleNeighbors(
                allVertices,
                { x,     y }, cenTexel, cenRegion,
                { x + 1, y }, xPosTexel, xPosRegion,
                patchHistory);

            handleNeighbors(
                allVertices,
                { x, y     }, cenTexel, cenRegion,
                { x, y + 1 }, yPosTexel, yPosRegion,
                patchHistory);
        }
    }

    // a vertex cannot be 'src' and 'sink' simultaneously

    for(auto v : allVertices)
    {
        if(v->isSrc && v->isSink)
            v->isSrc = false;
    }

    return Graph(allVertices);
}

Image2D<GraphBuilder::Region> GraphBuilder::buildRegionDistribution(
    Image2D<Texel>         &texels,
    const ImageView2D<RGB> &patch,
    const Int2             &patchBeg,
    const Int2             &patchOverlapSize) const
{
    Image2D<Region> regions(
        texels.height(), texels.width());

    const Int2 patchEnd     = patchBeg + patch.size();

    bool hasNew = false;

    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            auto &texel = texels(y, x);
            auto &region = regions(y, x);

            const bool coveredByOldPatch = texel.patchIndex >= 0;
            const bool coveredByNewPatch =
                patchBeg.x <= x && x < patchEnd.x &&
                patchBeg.y <= y && y < patchEnd.y;

            if(coveredByOldPatch)
                region = coveredByNewPatch ? Region::Overlap : Region::Old;
            else
                region = coveredByNewPatch ? Region::New : Region::Nil;

            if(region == Region::New)
                hasNew = true;
        }
    }

    if(hasNew)
        return regions;

    const Int2 patchCoreBeg = patchBeg + patchOverlapSize;
    const Int2 patchCoreEnd = patchEnd - patchOverlapSize;

    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            auto &region = regions(y, x);

            const bool coveredByNewCore =
                patchCoreBeg.x <= x && x < patchCoreEnd.x &&
                patchCoreBeg.y <= y && y < patchCoreEnd.y;

            if(coveredByNewCore)
                region = Region::New;
        }
    }

    return regions;
}

int GraphBuilder::computeSeamCost(
    const RGB &As, const RGB &At,
    const RGB &Bs, const RGB &Bt) const
{
    const int diffs = std::abs(As.r - Bs.r)
                    + std::abs(As.g - Bs.g)
                    + std::abs(As.b - Bs.b);
    const int difft = std::abs(At.r - Bt.r)
                    + std::abs(At.g - Bt.g)
                    + std::abs(At.b - Bt.b);
    return diffs + difft;
}

int GraphBuilder::computeSeamCost(
    int AIndex, int BIndex,
    const Int2 &s, const Int2 &t,
    const PatchHistory &patches) const
{
    const RGB As = patches.getRGB(AIndex, s.x, s.y);
    const RGB At = patches.getRGB(AIndex, t.x, t.y);
    const RGB Bs = patches.getRGB(BIndex, s.x, s.y);
    const RGB Bt = patches.getRGB(BIndex, t.x, t.y);
    return computeSeamCost(As, At, Bs, Bt);
}

std::pair<Vertex *, Vertex*> GraphBuilder::addSeam(
    const Int2 &aPos, Texel &aTexel, Vertex *aVertex,
    const Int2 &bPos, Texel &bTexel, Vertex *bVertex,
    int seamCost, Vertex::Type seamVertexType,
    const PatchHistory &patches)
{
    Vertex *seamVertex = vertexArena_.create();
    Vertex *seamSrc    = vertexArena_.create();

    Edge *seamSrcEdge = edgeArena_.create();
    Edge *a2Seam      = edgeArena_.create();
    Edge *b2Seam      = edgeArena_.create();

    const int a2SeamCost = computeSeamCost(
        aTexel.patchIndex, patches.getCurrentIndex(),
        aPos, bPos, patches);
    const int b2SeamCost = computeSeamCost(
        bTexel.patchIndex, patches.getCurrentIndex(),
        aPos, bPos, patches);

    seamSrcEdge->a = seamVertex;
    seamSrcEdge->b = seamSrc;
    a2Seam->a = aVertex;
    a2Seam->b = seamVertex;
    b2Seam->a = bVertex;
    b2Seam->b = seamVertex;

    seamSrcEdge->capacity = seamCost;
    a2Seam->capacity = a2SeamCost;
    b2Seam->capacity = b2SeamCost;

    seamVertex->position = aPos;
    seamVertex->type     = seamVertexType;

    seamSrc->type     = Vertex::Type::DummySrc;
    seamSrc->position = aPos;
    seamSrc->isSrc    = true;
    seamSrc->edges[0] = seamSrcEdge;

    if(seamVertexType == Vertex::Type::HoriSeam)
    {
        aVertex->edges[Vertex::EDGE_POS_X] = a2Seam;
        seamVertex->edges[Vertex::EDGE_NEG_X] = a2Seam;

        bVertex->edges[Vertex::EDGE_NEG_X] = b2Seam;
        seamVertex->edges[Vertex::EDGE_POS_X] = b2Seam;

        seamVertex->edges[Vertex::EDGE_HORI_SEAM_TO_SRC] = seamSrcEdge;
    }
    else
    {
        assert(seamVertexType == Vertex::Type::VertSeam);

        aVertex->edges[Vertex::EDGE_POS_Y] = a2Seam;
        seamVertex->edges[Vertex::EDGE_NEG_Y] = a2Seam;

        bVertex->edges[Vertex::EDGE_NEG_Y] = b2Seam;
        seamVertex->edges[Vertex::EDGE_POS_Y] = b2Seam;

        seamVertex->edges[Vertex::EDGE_VERT_SEAM_TO_SRC] = seamSrcEdge;
    }

    return { seamVertex, seamSrc };
}

void GraphBuilder::addEdge(
    const Int2 &aPos, Texel &aTexel, Vertex *aVertex,
    const Int2 &bPos, Texel &bTexel, Vertex *bVertex,
    const PatchHistory &patches,
    bool isVertical)
{
    Edge *e = edgeArena_.create();

    const int cost = computeSeamCost(
        aTexel.patchIndex, patches.getCurrentIndex(),
        aPos, bPos, patches);

    e->a = aVertex;
    e->b = bVertex;
    e->capacity = cost;

    if(isVertical)
    {
        aVertex->edges[Vertex::EDGE_POS_Y] = e;
        bVertex->edges[Vertex::EDGE_NEG_Y] = e;
    }
    else
    {
        aVertex->edges[Vertex::EDGE_POS_X] = e;
        bVertex->edges[Vertex::EDGE_NEG_X] = e;
    }
}

void GraphBuilder::handleNeighbors(
    std::set<Vertex *> &allVertices,
    const Int2 &aPos, Texel &aTexel, Region aRegion,
    const Int2 &bPos, Texel &bTexel, Region bRegion,
    const PatchHistory &patches)
{
    assert(bPos == aPos + Int2(0, 1) || bPos == aPos + Int2(1, 0));

    if(aRegion == Region::Old && bRegion == Region::Overlap)
    {
        auto bVertex = &bTexel.texelVertex;
        bVertex->isSink = true;
        allVertices.insert(bVertex);
    }
    else if(aRegion == Region::Overlap && bRegion == Region::Old)
    {
        auto aVertex = &aTexel.texelVertex;
        aVertex->isSink = true;
        allVertices.insert(aVertex);
    }
    else if(aRegion == Region::New && bRegion == Region::Overlap)
    {
        auto bVertex = &bTexel.texelVertex;
        bVertex->isSrc = true;
        allVertices.insert(bVertex);
    }
    else if(aRegion == Region::Overlap && bRegion == Region::New)
    {
        auto aVertex = &aTexel.texelVertex;
        aVertex->isSrc = true;
        allVertices.insert(aVertex);
    }
    else if(aRegion == Region::Overlap && bRegion == Region::Overlap)
    {
        auto aVertex = &aTexel.texelVertex;
        auto bVertex = &bTexel.texelVertex;
        allVertices.insert(aVertex);
        allVertices.insert(bVertex);

        if(aPos.y == bPos.y)
        {
            if(aTexel.xPosSeamCost)
            {
                auto [seam, seamSrc] = addSeam(
                    aPos, aTexel, aVertex,
                    bPos, bTexel, bVertex,
                    aTexel.xPosSeamCost, Vertex::Type::HoriSeam,
                    patches);

                allVertices.insert(seam);
                allVertices.insert(seamSrc);
            }
            else
            {
                addEdge(
                    aPos, aTexel, aVertex,
                    bPos, bTexel, bVertex,
                    patches, false);
            }
        }
        else
        {
            if(aTexel.yPosSeamCost)
            {
                auto [seam, seamSrc] = addSeam(
                    aPos, aTexel, aVertex,
                    bPos, bTexel, bVertex,
                    aTexel.yPosSeamCost, Vertex::Type::VertSeam,
                    patches);

                allVertices.insert(seam);
                allVertices.insert(seamSrc);
            }
            else
            {
                addEdge(
                    aPos, aTexel, aVertex,
                    bPos, bTexel, bVertex,
                    patches, true);
            }
        }
    }
}

GCTS_END
