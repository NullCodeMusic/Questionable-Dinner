#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelASawB;
extern Model* modelPilfer;

//Custom Components
struct KnobQ001 : RoundKnob {

	KnobQ001() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/qd-001/knob-fg.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/qd-001/knob-bg.svg")));
	}
};
struct PortQ001 : app::SvgPort {
	PortQ001() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/qd-001/port.svg")));
	}
};