#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelASawB;

//Custom Components
struct WinterKnob : RoundKnob {

	WinterKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundSmallBlackKnob.svg")));
		bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundSmallBlackKnob_bg.svg")));
	}
};
