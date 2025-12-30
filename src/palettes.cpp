#include "palettes.hpp"
#include <vector>
#include <array>
#include <nanovg.h>
#include <string>

//BG, Detail, Indicator, Highlight
static const std::vector<std::array<unsigned int, 4>> palettes = {
    {0xFFd4d5d6,  0xFFf7f3fb,  0xFF112a33,  0xFFf7f3fb},//Light
    {0xFF131419,  0xFF112a33,  0xFFd4d5d6,  0xFF112a33},//Dark
    {0xFF0062c4,  0xFF002827,  0xFF002827,  0xFF0048db},//Tangerine
    {0xFF9357d9,  0xFF194b4b,  0xFF194b4b,  0xFF0080ef},//Bubblegum
    {0xFFc36b40,  0xFF8a8ee6,  0xFF8a8ee6,  0xFF7a3fab},//Peach-Berry
    {0xff772f57,  0xff7336a5,  0xff4dc8a1,  0xff7336a5/*0xff4cb9ca*/},//Snot
    {0xff95bbcf,  0xffa4ccdb,  0xff7c99ac,  0xff9fc7e0}//Brazen
};

static const std::vector<std::string> names = {
    "Light",
    "Dark",
    "Tangerine",
    "Bubblegum",
    "Peach-Berry",
    "Snot",
    "Brazen"
};

std::array<unsigned int, 4> getPalette(int id){
    return palettes.at(id);
}

std::vector<std::string> getPaletteNames(){
    return names;
}

int getChromaKey(unsigned int color){
    switch (color)
    {
    case 0xFFFFFFFF: //WHITE FOR BG
        return 0;
    case 0xFF0000FF: //RED FOR DETAIL
        return 1;
    case 0xFF00FF00: //GREEN FOR INDICATOR
        return 2;
    case 0xFFFF0000: //BLUE FOR HIGHLIGHT
        return 3;
    default:
        return -1;
    }
}