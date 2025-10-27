#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelASawB;
extern Model* modelPilfer;

//Custom Components & Widgets
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

struct QSegParam : SvgKnob {
	QSegParam()	{
		forceLinear = true;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/gen/14SegX3BG.svg")));
		snap = true;
	}

	void draw(const DrawArgs& args) override {

		// Load font from cache
		std::string fontPath = asset::plugin(pluginInstance, "res/fonts/DSEG14Classic-Regular.ttf");
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		// Don't draw text if font failed to load

		if (font && module) {
			// Select font handle
			nvgFontFaceId(args.vg, font->handle);
			// Set font size and alignment
			nvgFontSize(args.vg, 19.843);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

			// Generate your text
			int paramAmount = module->params[paramId].getValue();
			std::string textBG = "~~~";
			std::string textFG = string::f("*%02i",paramAmount);
			// Draw the text at a position
			nvgFillColor(args.vg, nvgRGBAf(1,1,1,0.1));
			nvgText(args.vg, mm2px(0.01), mm2px(0.51), textBG.c_str(), NULL);
			nvgFillColor(args.vg, nvgRGBAf(1,1,1,0.8));
			nvgText(args.vg, 0, mm2px(0.5), textFG.c_str(), NULL);
			
		}
	}
};

struct TestWidget : SvgWidget {
	TestWidget(){
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/gen/14SegX3BG.svg")));
	}

	void draw(const DrawArgs& args) override {

		// Load font from cache
		std::string fontPath = asset::plugin(pluginInstance, "res/fonts/DSEG14Classic-Regular.ttf");
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		// Don't draw text if font failed to load

		if (font) {
			// Select font handle
			nvgFontFaceId(args.vg, font->handle);
			// Set font size and alignment
			nvgFontSize(args.vg, 19.843);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

			// Generate your text
			std::string textBG = "~~~";
			std::string textFG = "*01";
			// Draw the text at a position
			nvgFillColor(args.vg, nvgRGBAf(0,0,0,0.1));
			nvgText(args.vg, 1.0, 0.5, textBG.c_str(), NULL);
			nvgFillColor(args.vg, nvgRGBAf(0,0,0,1));
			nvgText(args.vg, 1.0, 0.5, textFG.c_str(), NULL);

		}
	}
};