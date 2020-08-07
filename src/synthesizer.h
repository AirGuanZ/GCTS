#pragma once

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

class Synthesizer
{
public:

    Image2D<RGB> generate(
        const Image2D<RGB> &src,
        const Int2         &dstSize) const;
};

GCTS_END
