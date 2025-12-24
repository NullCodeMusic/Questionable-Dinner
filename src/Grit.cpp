#include "plugin.hpp"
#include "mymath.hpp"


struct Grit : Module {
	int theme = -1;
	enum ParamId {
		HISS_PARAM,
		BITE_PARAM,
		DRIVE_PARAM,
		KNEE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MAIN_INPUT,
		HISS_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MAIN_OUTPUT,
		ENV_OUTPUT,
		AUX_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "theme", json_integer(theme));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* themeJ = json_object_get(rootJ, "theme");
		if (themeJ){
			theme = json_integer_value(themeJ);
		}
	}

	Grit() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(HISS_PARAM, 0.f, 6.f, 0.f, "Rasp");
		configParam(BITE_PARAM, 0.f, 14.f, 7.f, "Bite");
		configParam(DRIVE_PARAM, -0.5f, 4.f, 1.f, "Drive");
		configParam(KNEE_PARAM, 0.f, 0.999f, 0.f, "Knee");
		configInput(MAIN_INPUT,"Audio");
		configInput(HISS_INPUT,"Rasp");
		configOutput(MAIN_OUTPUT,"Main");
		configOutput(AUX_OUTPUT,"Aux");
		configOutput(ENV_OUTPUT,"Env");
		configBypass(MAIN_INPUT,MAIN_OUTPUT);
	}

	float env;

	void process(const ProcessArgs& args) override {
		float audio = inputs[MAIN_INPUT].getVoltageSum()/std::max(inputs[MAIN_INPUT].getChannels(),1);
		float dec = 1/dsp::exp2_taylor5(params[BITE_PARAM].getValue());
		float noise = rand()%1000-500;
		noise = inputs[HISS_INPUT].getNormalVoltage(noise/100.f)/5.f;
		noise = noise*params[HISS_PARAM].getValue();
		env = std::max(abs(audio)-noise,exp2Decay(env,dec,args.sampleRate));
		outputs[ENV_OUTPUT].setVoltage(env);
		float scaledAudio = audio/env;
		if(std::isnan(scaledAudio)){
			scaledAudio=0;
		}
		float drive = dsp::exp2_taylor5(params[DRIVE_PARAM].getValue());
		scaledAudio *= drive;
		scaledAudio = softClip(scaledAudio,1,params[KNEE_PARAM].getValue());
		scaledAudio /= softClip(drive,1,params[KNEE_PARAM].getValue());
		scaledAudio *= env;
		outputs[MAIN_OUTPUT].setVoltage(scaledAudio);
		outputs[AUX_OUTPUT].setVoltage(scaledAudio-audio);
	}
};


struct GritWidget : ModuleWidget {
	int theme = -1;

	GritWidget(Grit* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Graze.svg",
			getPalette(PAL_TANGERINE)
		));

		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(22.860, 70.0)), module, Grit::HISS_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(11.43 , 56.5)), module, Grit::BITE_PARAM));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(29.211, 41.0)), module, Grit::DRIVE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(34.290, 83.50)), module, Grit::KNEE_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.618, 114.0)),module,Grit::MAIN_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.618, 79.0)),module,Grit::HISS_INPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(17.78, 114.0)),module,Grit::MAIN_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(27.94, 114.0)),module,Grit::AUX_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(38.1, 114.0)),module,Grit::ENV_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Grit* module = getModule<Grit>();

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
		Grit* module = getModule<Grit>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Graze.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelGrit = createModel<Grit, GritWidget>("Grit");