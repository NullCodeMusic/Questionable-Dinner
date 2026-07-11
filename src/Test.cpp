//Module Slug Test - find and replace this to make a new one

#include "plugin.hpp"
#include "ADAA.hpp"

struct Test : Module {
	int theme = -1;
	
	enum ParamId {
		K_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_IN,
		INPUTS_LEN
	};
	enum OutputId {
		BASE_OUT,
		FIRST_OUT,
		SECOND_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Test() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(K_PARAM, -1, 0.9999, 0, "Knee");
		configInput(AUDIO_IN,"Audio");
		configOutput(0,"No ADAA");
		configOutput(1,"1st Order ADAA");
		configOutput(2,"2nd Order ADAA");
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

	ADAAClipF4Tanh clipper;

    void process(const ProcessArgs& args) override {
		float x = inputs[AUDIO_IN].getVoltage();
		float k = params[K_PARAM].getValue();
		clipper.process(x,k);
		//outputs[0].setVoltage(clipper.f(inputs[AUDIO_IN].getVoltage(),k));
		outputs[1].setVoltage(clipper.out[0]);
		//outputs[2].setVoltage(clipper.F1(x));
	}
};


struct TestWidget : ModuleWidget {
	int theme = -1;

	TestWidget(Test* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Plastic.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
        addParam(createParamCentered<QKnob8mm>(mm2px(Vec(10.0, 10.0)), module, Test::K_PARAM));
		addInput(createInputCentered<QPort>(mm2px(Vec(10.0, 50.0)), module, 0));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(10.0, 20.0)), module, 0));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(10.0, 30.0)), module, 1));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(10.0, 40.0)), module, 2));
		
	}

	void appendContextMenu(Menu* menu) override {
		Test* module = getModule<Test>();
		
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
		Test* module = getModule<Test>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Plastic.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelTest = createModel<Test, TestWidget>("Test");