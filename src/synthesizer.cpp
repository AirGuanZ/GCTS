#include <agz/utility/alloc.h>

#include "graphBuilder.h"

GCTS_BEGIN

Image2D<RGB> Synthesizer::generate(
    const Image2D<RGB> &src,
    const Int2         &dstSize) const
{
    Image2D<Texel> texels(dstSize.y, dstSize.x);
    PatchHistory patchHistory;

    for(int i = 0; i < 3; ++i)
    {
        // select a new patch

        auto patch = src.subview(0, src.height(), 0, src.width());
        const Int2 patchBeg = i == 0 ? Int2{ 0, 0 } : (i == 1 ? Int2{ 20, 20 } : Int2(60, 0));

        // update patch history

        patchHistory.addNewPatch(patch, patchBeg);

        // build texel graph

        GraphBuilder graphBuilder;

        auto graph = graphBuilder.build(
            texels, patch, patchBeg, { 800, 800 }, patchHistory);

        // find min cut

        auto minCut = graph.findMinCut();

        // update texels

        for(auto v : minCut.reachableVertices)
        {
            if(v->type == Vertex::Type::Texel)
            {
                const auto [x, y] = v->position;
                texels(y, x).patchIndex = i;
            }
        }
    }

    // resolve texels

    Image2D<RGB> result(dstSize.y, dstSize.x);

    for(int y = 0; y < dstSize.y; ++y)
    {
        for(int x = 0; x < dstSize.x; ++x)
        {
            auto &t = texels(y, x);
            if(t.patchIndex >= 0)
                result(y, x) = /*RGB(60 * (t.patchIndex + 1)); //*/patchHistory.getRGB(t.patchIndex, x, y);
        }
    }

    return result;
}

GCTS_END
