#include "patchHistory.h"

GCTS_BEGIN

void PatchHistory::addNewPatch(
    const ImageView2D<RGB> &patch,
    const Int2             &patchBeg)
{
    patches_.push_back({ patch, patchBeg });
}

int PatchHistory::getCurrentIndex() const noexcept
{
    return static_cast<int>(patches_.size()) - 1;
}

const ImageView2D<RGB> &PatchHistory::getCurrentPatch() const noexcept
{
    return patches_.back().patch;
}

const Int2 &PatchHistory::getCurrentPatchBeg() const noexcept
{
    return patches_.back().patchBeg;
}

const RGB &PatchHistory::getRGB(
    int patch, int x, int y) const noexcept
{
    auto &rcd = patches_[patch];
    const Int2 localPos = { x - rcd.patchBeg.x, y - rcd.patchBeg.y };
    return rcd.patch(localPos.y, localPos.x);
}

GCTS_END