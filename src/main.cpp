#include <iostream>

#include <agz/utility/image.h>

#include "synthesizer.h"

int main()
{
    auto src = gcts::Image2D<gcts::RGB>(
        agz::img::load_rgb_from_file("asset/input0.png"));

    auto output = gcts::Synthesizer().generate(src, { 256, 256 });

    agz::img::save_rgb_to_png_file("asset/output0.png", output.get_data());
}
