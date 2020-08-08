#include <agz/utility/image.h>
#include <agz/utility/string.h>

#include <cxxopts.hpp>

#include "synthesizer.h"

struct Options
{
    std::string inputFilename;
    std::string outputFilename;

    int outputWidth  = 0;
    int outputHeight = 0;

    int patchWidth  = 0;
    int patchHeight = 0;

    int additionalPatchCount = 0;

    gcts::Synthesizer::PatchPlacementStrategy patchPlacement =
        gcts::Synthesizer::Random;
};

std::optional<Options> parseOptions(int argc, char *argv[])
{
    cxxopts::Options options("GCTS");
    options.add_options("")
        ("i,input",      "input filename (jpg, png, bmp)",    cxxopts::value<std::string>())
        ("o,output",     "output filename (jpg, png, bmp)",   cxxopts::value<std::string>())
        ("w,width",      "output width",                      cxxopts::value<int>())
        ("h,height",     "output height",                     cxxopts::value<int>())
        ("m,pwidth",     "patch width",                       cxxopts::value<int>()->default_value("-1"))
        ("n,pheight",    "patch height",                      cxxopts::value<int>()->default_value("-1"))
        ("c,patchCount", "additional patch count",            cxxopts::value<int>()->default_value("-1"))
        ("s,strategy",   "patch placement strategy (random)", cxxopts::value<std::string>()->default_value("random"))
        ("help",         "help information");
    const auto args = options.parse(argc, argv);

    if(args.count("help"))
    {
        std::cout << options.help({ "" }) << std::endl;
        return {};
    }

    Options result;

    try
    {
        result.inputFilename        = args["input"].as<std::string>();
        result.outputFilename       = args["output"].as<std::string>();
        result.outputWidth          = args["width"].as<int>();
        result.outputHeight         = args["height"].as<int>();
        result.patchWidth           = args["pwidth"].as<int>();
        result.patchHeight          = args["pheight"].as<int>();
        result.additionalPatchCount = args["patchCount"].as<int>();

        const std::string strategy = args["strategy"].as<std::string>();
        if(strategy == "random")
            result.patchPlacement = gcts::Synthesizer::Random;
        else
        {
            throw std::runtime_error(
                "unknown patch placement strategy: " + strategy);
        }
    }
    catch(...)
    {
        std::cout << options.help({ "" }) << std::endl;
        return {};
    }

    return result;
}

void run(int argc, char *argv[])
{
    auto options = parseOptions(argc, argv);
    if(!options)
        return;

    if(options->outputWidth <= 0 || options->outputHeight <= 0)
        throw std::runtime_error("invalid output size");

    const auto src = gcts::Image2D<gcts::RGB>(
        agz::img::load_rgb_from_file(options->inputFilename));
    if(!src.is_available())
        throw std::runtime_error("failed to load input image");

    if(options->patchWidth <= 0)
        options->patchWidth = src.width();
    if(options->patchHeight <= 0)
        options->patchHeight = src.height();
    options->patchWidth = std::min(options->patchWidth, src.width());
    options->patchHeight = std::min(options->patchHeight, src.height());

    if(options->additionalPatchCount < 0)
    {
        options->additionalPatchCount =
            options->outputWidth * options->outputHeight /
            (options->patchWidth * options->patchHeight);
    }

    gcts::Synthesizer syn;
    syn.setParams(
        { options->patchWidth, options->patchHeight },
        options->additionalPatchCount,
        options->patchPlacement);

    const auto out = syn.generate(
        src, { options->outputWidth, options->outputHeight });

    const auto hasExt =
        [lowerFilename = agz::stdstr::to_lower(options->outputFilename)]
        (const char *ext)
    {
        return agz::stdstr::ends_with(lowerFilename, ext);
    };

    if(hasExt("png"))
        agz::img::save_rgb_to_png_file(options->outputFilename, out.get_data());
    else if(hasExt("jpg") || hasExt("jpeg"))
        agz::img::save_rgb_to_jpg_file(options->outputFilename, out.get_data());
    else if(hasExt(".bmp"))
        agz::img::save_rgb_to_bmp_file(options->outputFilename, out.get_data());
    else
        throw std::runtime_error("unsupported output format");
}

int main(int argc, char *argv[])
{
    try
    {
        run(argc, argv);
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
