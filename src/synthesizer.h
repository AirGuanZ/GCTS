#pragma once

#include <random>
#include <set>

#include <agz/utility/texture.h>

#define GCTS_BEGIN namespace gcts {
#define GCTS_END   }

GCTS_BEGIN

using RGB  = agz::math::color3b;
using Int2 = agz::math::vec2i;

template<typename T>
using Image2D = agz::texture::texture2d_t<T>;

template<typename T>
using ImageView2D = agz::texture::texture2d_view_t<T, true>;

struct Edge;
struct Texel;

class Synthesizer
{
public:

    enum PatchPlacementStrategy
    {
        Random
    };

    void setParams(
        const Int2            &patchSize,
        int                    additionalPatchCount,
        PatchPlacementStrategy patchPlacement) noexcept;

    Image2D<RGB> generate(
        const Image2D<RGB> &src,
        const Int2         &dstSize) const;

private:

    ImageView2D<RGB> pickPatchRandomly(
        const Image2D<RGB>         &src,
        std::default_random_engine &rng) const;

    Int2 pickPatchBegRandomly(
        const Image2D<Texel>       &texels,
        std::default_random_engine &rng,
        bool                       *hasHole,
        Int2                       *holeBeg) const;

    Int2 findNextHoleBeg(const Image2D<Texel> &texels) const;

    void updateSeamInfo(
        Image2D<Texel>        &texels,
        const std::set<Edge*> &cut) const;

    int additionalPatchCount_ = 0;

    Int2 patchSize_;

    PatchPlacementStrategy patchPlacement_ = Random;
};

GCTS_END
