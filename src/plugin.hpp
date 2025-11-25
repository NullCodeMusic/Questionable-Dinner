#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelASawB;
extern Model* modelPilfer;
extern Model* modelSurgeon;
extern Model* modelElasticTwins;
extern Model* modelLoam;

//Custom Components & Widgets
struct QKnob8mm : RoundKnob {
	QKnob8mm() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/8mm-fg.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/8mm-bg.svg")));
	}
};

struct QKnob10mm : RoundKnob {
	QKnob10mm() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/10mm-fg.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/10mm-bg.svg")));
	}
};

struct QKnob18mm : RoundKnob {
	QKnob18mm() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/18mm-fg.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/18mm-bg.svg")));
	}
};

struct QKnob18mmClock : RoundKnob {
	QKnob18mmClock() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;

		setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/18mm-fg-clock.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/knobs/18mm-bg.svg")));
	}
};

struct QPort : app::SvgPort {
	QPort() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/gen/port.svg")));
	}
};

struct QSegParam : SvgKnob {
	QSegParam()	{
		forceLinear = true;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/gen/14SegX3BG.svg")));
		snap = true;
	}

	void draw(const DrawArgs& args) override {}
	void drawLayer(const DrawArgs& args, int layer) override {
		if(layer == 1){
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
	}
};

struct QBigNumber : SvgKnob {
	QBigNumber()	{
		forceLinear = true;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/gen/14SegX3BG.svg")));
		snap = true;
	}

	void draw(const DrawArgs& args) override {}
	void drawLayer(const DrawArgs& args, int layer) override {
		if(layer == 1){
			// Load font from cache
			std::string fontPath = asset::plugin(pluginInstance, "res/fonts/terminal-grotesque_open.otf");
			std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
			// Don't draw text if font failed to load

			if (font && module) {
				// Select font handle
				nvgFontFaceId(args.vg, font->handle);
				// Set font size and alignment
				nvgFontSize(args.vg, 25);
				nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

				// Generate your text
				int paramAmount = module->params[paramId].getValue();
				std::string textFG = string::f("%02i",paramAmount);
				// Draw the text at a position
				nvgFillColor(args.vg, nvgRGBAf(1,1,1,0.82));
				nvgText(args.vg, mm2px(4.5), mm2px(2), textFG.c_str(), NULL);
			}
		}
	}
};

struct QInfoText : LedDisplay {
	std::string text = "SURGEON";
	NVGcolor color = nvgRGBAf(1,1,1,0.82);
	void draw(const DrawArgs& args) override {}
	void drawLayer(const DrawArgs& args, int layer) override {
		if(layer == 1){
			// Load font from cache
			std::string fontPath = asset::plugin(pluginInstance, "res/fonts/vcr-jp.regular.ttf");
			std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
			// Don't draw text if font failed to load

			if (font) {
				// Select font handle
				nvgFontFaceId(args.vg, font->handle);
				// Set font size and alignment
				nvgFontSize(args.vg, 13.4);
				nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

				// Draw the text at a position
				nvgFillColor(args.vg, color);
				nvgText(args.vg, 0, 0, text.c_str(), NULL);
			}
		}
	}
};

struct QDataEntry : LedDisplayTextField{
	bool hovered = false;
	QDataEntry(){
		fontPath = asset::plugin(pluginInstance, "res/fonts/DepartureMono-Regular.otf");
		color = nvgRGBAf(1,1,1,0.5);
		bgColor = nvgRGBAf(0,0,0,0);
		textOffset = mm2px(Vec(0, 0));
	}
	void onHover(const HoverEvent& e) override {
		color = nvgRGBAf(1,1,1,0.82);
		textOffset = mm2px(Vec(0.5, 0));
		Widget::onHover(e);
		e.stopPropagating();
		// Consume if not consumed by child
		if (!e.isConsumed())
			e.consume(this);
		hovered = true;
	}
	void onLeave(const LeaveEvent& e) override{
		color = nvgRGBAf(1,1,1,0.5);
		textOffset = mm2px(Vec(0, 0));
		hovered = false;
	}
};
