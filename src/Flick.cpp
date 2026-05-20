#include "plugin.hpp"
#include "mymath.hpp"

struct Flick : Module {

	int theme = -1;

	enum ParamId {
		DECAYA_PARAM,
		NOISEA_PARAM,
		SHAPEA_PARAM,
		DECAYB_PARAM,
		NOISEB_PARAM,
		SHAPEB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIGA_INPUT,
		TRIGB_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		WAVEA_OUTPUT,
		WAVEB_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float impulseA = 0;
	float lastInA = 0;
	float shapeA = 0;

	float impulseB = 0;
	float lastInB = 0;
	float shapeB = 0;
	
	Flick() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DECAYA_PARAM, 1.f, 1000.f, 400.f, "Tone A");
		configParam(NOISEA_PARAM, 0.f, 1.f, 0.f, "Fuzz A");
		configParam(SHAPEA_PARAM, 0.f, 5.f, 1.f, "Variance A");
		configInput(TRIGA_INPUT, "Trig A");
		configOutput(WAVEA_OUTPUT, "Out A");

		configParam(DECAYB_PARAM, 1.f, 1000.f, 400.f, "Tone B");
		configParam(NOISEB_PARAM, 0.f, 1.f, 0.f, "Fuzz B");
		configParam(SHAPEB_PARAM, 0.f, 5.f, 1.f, "Variance B");
		configInput(TRIGB_INPUT, "Trig B");
		configOutput(WAVEB_OUTPUT, "Out B");
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

	void process(const ProcessArgs& args) override {

		//EXTENSION
		float pitchA=1.f;
		float lvlA=1.f;
		float pitchB=1.f;
		float lvlB=1.f;
		Module* extL = getLeftExpander().module;
		if(extL && extL->getModel() == modelFlickX){
			pitchA = dsp::exp2_taylor5(extL->getInput(0).getVoltage());
			pitchB = dsp::exp2_taylor5(extL->getInput(2).getVoltage());
			lvlA = extL->getInput(1).getNormalVoltage(10.f)/10.f;
			lvlB = extL->getInput(3).getNormalVoltage(10.f)/10.f;
		}
		
		//TOP
		float decay = 1/params[DECAYA_PARAM].getValue()/pitchA;
		if(inputs[TRIGA_INPUT].getVoltage()>lastInA){
			impulseA = 1;
			shapeA = params[SHAPEA_PARAM].getValue()*(rand()%1000)/1000.f;
		}
		float noise = (rand()%1000)/1000.f*params[NOISEA_PARAM].getValue()*5;
		impulseA = exp2Decay(impulseA,decay,args.sampleRate);
		lastInA = inputs[TRIGA_INPUT].getVoltage();
		float wave1 = softFold05(impulseA*10-8.81512+noise)*6.68270515905f*impulseA;
		float wave2 = softFold05(impulseA*10-5+shapeA)*6.68270515905f*impulseA;
		outputs[WAVEA_OUTPUT].setVoltage((wave1-wave2)*lvlA);

		//BOTTOM
		decay = 1/params[DECAYB_PARAM].getValue()/pitchB;
		if(inputs[TRIGB_INPUT].getVoltage()>lastInB){
			impulseB = 1;
			shapeB = params[SHAPEB_PARAM].getValue()*(rand()%1000)/1000.f;
		}
		noise = (rand()%1000)/1000.f*params[NOISEB_PARAM].getValue()*5;
		impulseB = exp2Decay(impulseB ,decay,args.sampleRate);
		lastInB = inputs[TRIGB_INPUT].getVoltage();
		wave1 = softFold05(impulseB*10-8.81512+noise)*6.68270515905f*impulseB;
		wave2 = softFold05(impulseB*10-5+shapeB)*6.68270515905f*impulseB;
		outputs[WAVEB_OUTPUT].setVoltage((wave1-wave2)*lvlB);

	}
};


struct FlickWidget : ModuleWidget {
	int theme = -1;
	FlickWidget(Flick* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Flick.svg",
			getPalette(PAL_LIGHT)
		));

		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 11.0)), module, Flick::TRIGA_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 22.0)), module, Flick::DECAYA_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 33.0)), module, Flick::NOISEA_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 44.0)), module, Flick::SHAPEA_PARAM));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(5.08, 55.0)), module, Flick::WAVEA_OUTPUT));

		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 70.0)), module, Flick::TRIGB_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 81.0)), module, Flick::DECAYB_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 92.0)), module, Flick::NOISEB_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(5.08, 103.0)), module, Flick::SHAPEB_PARAM));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(5.08, 114.0)), module, Flick::WAVEB_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Flick* module = getModule<Flick>();

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
		Flick* module = getModule<Flick>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Flick.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelFlick = createModel<Flick, FlickWidget>("Flick");