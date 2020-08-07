#pragma once

#include "synthesizer.h"

GCTS_BEGIN

class PatchHistory
{
public:

    void addNewPatch(const ImageView2D<RGB> &patch, const Int2 &patchBeg);

    int getCurrentIndex() const noexcept;

    const ImageView2D<RGB> &getCurrentPatch() const noexcept;

    const Int2 &getCurrentPatchBeg() const noexcept;

    const RGB &getRGB(int patch, int x, int y) const noexcept;

private:

    struct Record
    {
        ImageView2D<RGB> patch;
        Int2 patchBeg;
    };

    std::vector<Record> patches_;
};

GCTS_END
