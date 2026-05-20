//Module Slug OTamer - find and replace this to make a new one

#include "plugin.hpp"

struct BlendSwitch : SvgSwitch {
	BlendSwitch() {
		shadow->opacity = 0.0;
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/BlendBTN1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/BlendBTN2.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/BlendBTN3.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/BlendBTN4.svg")));
	}
};
struct WaveSwitch : SvgSwitch {
	WaveSwitch() {
		shadow->opacity = 0.0;
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/WaveBTN1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/WaveBTN2.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switches/WaveBTN3.svg")));
	}
};

struct OTamer : Module {
	int theme = -1;
	
	enum ParamId {
		FTYPE_SWITCH,
		BASEWAVE_SWITCH,
		FUND_BUTTON,
		FWIDTH_PARAM,
		FFREQ_PARAM,
		FBIAS_PARAM,
		FAMT_PARAM,
		DITHER_PARAM,
		ENV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FWIDTH_INPUT,
		FFREQ_INPUT,
		FBIAS_INPUT,
		FAMT_INPUT,
		DITHER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENV_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	OTamer() {
		
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(FTYPE_SWITCH,0,3,0,"Filter Type",{"Lowpass","Highpass","Bandpass","Notch"});
		configSwitch(BASEWAVE_SWITCH,0,2,0,"Wave",{"Saw","Square","Silence"});
		configButton(FUND_BUTTON, "Lock Fundamental");
		configParam(FWIDTH_PARAM,0,5,2.5,"Filter Width"," partials",2,4);
		configParam(FFREQ_PARAM,0,5,2.5,"Filter Center","",2,4);
		configParam(FBIAS_PARAM, 0, 1, 0,"Filter Base");
		configParam(FAMT_PARAM,0,1,1,"Filter Amount");
		configParam(DITHER_PARAM,-16,16,0,"Dither")->snapEnabled = true;

		configParam(ENV_PARAM, -1, 1, 0, "Envelope Amp", "%", 0, 100);
		configOutput(ENV_OUT, "Env Out");

		configInput(FWIDTH_INPUT, "Filter Width");
		configInput(FFREQ_INPUT, "Filter Center");
		configInput(FBIAS_INPUT, "Filter Base");
		configInput(FAMT_INPUT, "Filter Amount");
		configInput(DITHER_INPUT, "Dither");
	}

    json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "theme", json_integer(theme));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "theme");
		if (modeJ){
			theme = json_integer_value(modeJ);
		}
	}
};


struct OTamerWidget : ModuleWidget {
	int theme = -1;

	OTamerWidget(OTamer* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/OTamer.svg",
			getPalette(PAL_SNOT)
		));
        //Add widgets here: 
        addParam(createParamCentered<BlendSwitch>(mm2px(Vec(6.691,19.500)), module, OTamer::FTYPE_SWITCH));
        addParam(createParamCentered<WaveSwitch>(mm2px(Vec(18.590,19.500)), module, OTamer::BASEWAVE_SWITCH));
        addParam(createParamCentered<CKSS>(mm2px(Vec(18.590,31.000)), module, OTamer::FUND_BUTTON));
        addParam(createParamCentered<QKnob8mm>(mm2px(Vec(18.590,42.5)), module, OTamer::FFREQ_PARAM));
        addParam(createParamCentered<QKnob8mm>(mm2px(Vec(18.590,42.5+15)), module, OTamer::FWIDTH_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(18.590,42.5+15*2)), module, OTamer::FBIAS_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(18.590,42.5+15*3)), module, OTamer::FAMT_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(18.590,101.5)), module, OTamer::DITHER_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.691,114.5)), module, OTamer::ENV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.691,42.5)), module, OTamer::FFREQ_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.691,42.5+15)), module, OTamer::FWIDTH_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.691,42.5+15*2)), module, OTamer::FBIAS_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.691,42.5+15*3)), module, OTamer::FAMT_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.691,101.5)), module, OTamer::DITHER_INPUT));
		
		addOutput(createOutputCentered<QPort>(mm2px(Vec(18.590,114.5)), module, OTamer::ENV_OUT));
	}

	void appendContextMenu(Menu* menu) override {
		OTamer* module = getModule<OTamer>();
		
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexSubmenuItem(
			"Panel Theme", 
			getPaletteNames(),	
			[=](){
				return module->theme;
			},
			[=](int newTheme) {
				module->theme=newTheme;
			}
		));
	}

	void step() override{
		ModuleWidget::step();
		OTamer* module = getModule<OTamer>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/OTamer.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelOTamer = createModel<OTamer, OTamerWidget>("OTamer");