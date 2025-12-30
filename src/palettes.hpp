#pragma once
#include <vector>
#include <nanovg.h>
#include <string>

std::array<unsigned int, 4> getPalette(int id);
std::vector<std::string> getPaletteNames();
int getChromaKey(unsigned int color);

enum paletteID {
    PAL_LIGHT,
    PAL_DARK,
    PAL_TANGERINE,
    PAL_BUBBLEGUM,
    PAL_PEACHBERRY,
    PAL_MINT,
    PAL_SNOT
};