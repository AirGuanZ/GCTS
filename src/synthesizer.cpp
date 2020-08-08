#include <agz/utility/alloc.h>
#include <agz/utility/console.h>

#include "graphBuilder.h"

GCTS_BEGIN

void Synthesizer::setParams(
    const Int2            &patchSize,
    int                    additionalPatchCount,
    PatchPlacementStrategy patchPlacement) noexcept
{
    patchSize_            = patchSize;
    additionalPatchCount_ = additionalPatchCount;
    patchPlacement_       = patchPlacement;
}

Image2D<RGB> Synthesizer::generate(
    const Image2D<RGB> &src,
    const Int2         &dstSize) const
{
    Image2D<Texel> texels(dstSize.y, dstSize.x);
    PatchHistory patchHistory;
    std::default_random_engine rng{ std::random_device()() };

    auto runIter = [&](
        const ImageView2D<RGB> &patch,
        const Int2             &patchBeg,
        int                     patchIndex)
    {
        // update patch history

        patchHistory.addNewPatch(patch, patchBeg);

        // build texel graph

        GraphBuilder graphBuilder;

        const Int2 overlapSize = {
            std::max(1, (patchSize_.x - 2) / 2),
            std::max(1, (patchSize_.y - 2) / 2)
        };

        auto graph = graphBuilder.build(
            texels, patch, patchBeg, overlapSize, patchHistory);

        // find min cut

        auto minCut = graph.findMinCut();

        // update texels

        for(auto v : minCut.reachableVertices)
        {
            if(v->type == Vertex::Type::Texel)
            {
                const auto [x, y] = v->position;
                texels(y, x).patchIndex = patchIndex;
            }
        }

        // update seam info

        updateSeamInfo(texels, minCut.cut);
    };

    std::cout << "fill holes..." << std::endl;

    agz::console::progress_bar_f_t pbar(80, '=');
    pbar.display();

    int nextPatchIndex = 0;

    for(;;)
    {
        bool hasHole;
        Int2 holeBeg;

        const auto patch    = pickPatchRandomly(src, rng);
        const Int2 patchBeg = pickPatchBegRandomly(
            texels, rng, &hasHole, &holeBeg);

        if(!hasHole)
            break;

        runIter(patch, patchBeg, nextPatchIndex++);

        const int holeTexels = holeBeg.y * texels.width() + holeBeg.x;
        pbar.set_percent(100.0f * holeTexels / texels.size().product());
        pbar.display();
    }

    pbar.done();

    std::cout << "paste additional patches (" << additionalPatchCount_
              << " in total)..." << std::endl;

    pbar = agz::console::progress_bar_f_t(80, '=');
    pbar.display();

    for(int i = 0; i < additionalPatchCount_; ++i)
    {
        auto patch = pickPatchRandomly(src, rng);
        const Int2 patchBeg = pickPatchBegRandomly(texels, rng, nullptr, nullptr);

        runIter(patch, patchBeg, nextPatchIndex++);

        pbar.set_percent(100.0f * (i + 1) / additionalPatchCount_);
        pbar.display();
    }

    pbar.done();

    // resolve texels

    Image2D<RGB> result(dstSize.y, dstSize.x);

    for(int y = 0; y < dstSize.y; ++y)
    {
        for(int x = 0; x < dstSize.x; ++x)
        {
            auto &t = texels(y, x);
            if(t.patchIndex >= 0)
                result(y, x) = patchHistory.getRGB(t.patchIndex, x, y);
        }
    }

    return result;
}

ImageView2D<RGB> Synthesizer::pickPatchRandomly(
    const Image2D<RGB>         &src,
    std::default_random_engine &rng) const
{
    assert(patchSize_.x <= src.width() && patchSize_.y <= src.height());

    std::uniform_int_distribution disX(0, src.width() - patchSize_.x);
    std::uniform_int_distribution disY(0, src.height() - patchSize_.y);

    const int x = disX(rng);
    const int y = disY(rng);

    return src.subview(y, y + patchSize_.y, x, x + patchSize_.x);
}

Int2 Synthesizer::pickPatchBegRandomly(
    const Image2D<Texel>       &texels,
    std::default_random_engine &rng,
    bool                       *hasHole,
    Int2                       *holeBeg) const
{
    if(hasHole)
    {
        *holeBeg = findNextHoleBeg(texels);
        *hasHole = holeBeg->x >= 0;
    }

    if(!hasHole || !*hasHole)
    {
        std::uniform_int_distribution disX(-patchSize_.x + 1, texels.width() - 1);
        std::uniform_int_distribution disY(-patchSize_.y + 1, texels.height() - 1);

        const int x = disX(rng);
        const int y = disY(rng);

        return { x, y };
    }

    std::uniform_int_distribution disX(
        holeBeg->x - patchSize_.x * 2 / 3, holeBeg->x - patchSize_.x / 3);
    std::uniform_int_distribution disY(
        holeBeg->y - patchSize_.y * 2 / 3, holeBeg->y - patchSize_.y / 3);

    const int x = disX(rng);
    const int y = disY(rng);

    return { x, y };
}

Int2 Synthesizer::findNextHoleBeg(const Image2D<Texel> &texels) const
{
    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            if(texels(y, x).patchIndex < 0)
                return { x, y };
        }
    }
    return { -1, -1 };
}

void Synthesizer::updateSeamInfo(
    Image2D<Texel>         &texels,
    const std::set<Edge *> &cut) const
{
    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            texels(y, x).keepXPosSeam = false;
            texels(y, x).keepYPosSeam = false;
        }
    }

    for(auto e : cut)
    {
        Vertex *a = e->a, *b = e->b;

        // normal edge

        if(a->type == Vertex::Texel && b->type == Vertex::Texel)
        {
            Int2 pos; bool hori;

            if(a->position.y == b->position.y)
            {
                hori = true;
                pos = (a->position.x < b->position.x ? a : b)->position;
            }
            else
            {
                hori = false;
                pos = (a->position.y < b->position.y ? a : b)->position;
            }

            auto &texel = texels(pos.y, pos.x);
            if(hori)
            {
                texel.xPosSeamCost = e->capacity;
                texel.keepXPosSeam = true;
            }
            else
            {
                texel.yPosSeamCost = e->capacity;
                texel.keepYPosSeam = true;
            }

            continue;
        }

        // dummy edge

        if(b->type == Vertex::DummySrc)
            std::swap(a, b);

        if(a->type == Vertex::DummySrc)
        {
            assert(b->type == Vertex::HoriSeam ||
                   b->type == Vertex::VertSeam);

            if(b->type == Vertex::HoriSeam)
            {
                const Int2 pos = b->edges[Vertex::EDGE_NEG_X]
                    ->another(b)->position;
                texels(pos.y, pos.x).keepXPosSeam = true;
            }
            else
            {
                const Int2 pos = b->edges[Vertex::EDGE_NEG_Y]
                    ->another(b)->position;
                texels(pos.y, pos.x).keepYPosSeam = true;
            }

            continue;
        }

        // seam edge

        if(b->type == Vertex::HoriSeam || b->type == Vertex::VertSeam)
            std::swap(a, b);

        if(a->type == Vertex::HoriSeam)
        {
            assert(b->type == Vertex::Texel);
            const auto [x, y] = a->position.x < b->position.x ?
                a->position : b->position;
            texels(y, x).keepXPosSeam = true;

            continue;
        }

        if(a->type == Vertex::VertSeam)
        {
            assert(b->type == Vertex::Texel);
            const auto [x, y] = a->position.y < b->position.y ?
                a->position : b->position;
            texels(y, x).keepYPosSeam = true;
        }
    }

    for(int y = 0; y < texels.height(); ++y)
    {
        for(int x = 0; x < texels.width(); ++x)
        {
            auto &texel = texels(y, x);
            if(!texel.keepXPosSeam)
                texel.xPosSeamCost = 0;
            if(!texel.keepYPosSeam)
                texel.yPosSeamCost = 0;
        }
    }
}

GCTS_END
