//Module Slug Spoon - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"

struct Spoon : Module {
	int theme = -1;
	
	enum ParamId {
		FRICTION_PARAM,
		TENSION_PARAM,
		TRIG_BUTTON,
		FEEDBACK_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_IN,
		IMPULSE_IN,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Spoon() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FRICTION_PARAM,0,10,5,"Friction");
		configParam(TENSION_PARAM,0.8,12,6.4,"Tension");
		configButton(TRIG_BUTTON,"Trig");
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

	float lastTrigIn = 0;
	float lastTrigBtn = 0;
	float state = 0;
	float momentum = 0;
	float lastIn = 0;
    void process(const ProcessArgs& args) override {
		float friction = dsp::exp2_taylor5(-params[FRICTION_PARAM].getValue());
		float tension = dsp::exp2_taylor5(params[TENSION_PARAM].getValue())*8000;
		float slope = inputs[IMPULSE_IN].getVoltage()-lastIn;
		float freq = 2;
		lastIn = inputs[IMPULSE_IN].getVoltage();

		if(inputs[TRIG_IN].getVoltage()>lastTrigIn||params[TRIG_BUTTON].getValue()>lastTrigBtn){
			state = 1;
		}
		lastTrigIn = inputs[TRIG_IN].getVoltage();
		lastTrigBtn = params[TRIG_BUTTON].getValue();

		momentum += state * -tension * args.sampleTime *freq;
		state += momentum * args.sampleTime *freq + slope/5;
		momentum = exp2Decay(momentum,friction,args.sampleRate);

		outputs[AUDIO_OUT].setVoltage(state*5.f);
	}
};


struct SpoonWidget : ModuleWidget {
	int theme = -1;

	SpoonWidget(Spoon* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/ASawBTintLayers.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
        addParam(createParamCentered<QKnob8mm>(mm2px(Vec(10.0, 10.0)), module, Spoon::FRICTION_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(10.0, 20.0)), module, Spoon::TENSION_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(10.0, 50.0)), module, Spoon::FEEDBACK_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(10.0, 30.0)), module, Spoon::TRIG_BUTTON));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(10.0,40.0)),module, Spoon::AUDIO_OUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(10.0,60.0)),module, Spoon::IMPULSE_IN));
	}

	void appendContextMenu(Menu* menu) override {
		Spoon* module = getModule<Spoon>();
		
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
		Spoon* module = getModule<Spoon>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Spoon.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelSpoon = createModel<Spoon, SpoonWidget>("Spoon");