//Module Slug MyModule - find and replace this to make a new one

#include "plugin.hpp"

struct MyModule : Module {
	int theme = -1;
	
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	MyModule() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
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

	}
};


struct MyModuleWidget : ModuleWidget {
	int theme = -1;

	MyModuleWidget(MyModule* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/MyModule.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
        //addParam(createParamCentered<QKnob8mm>(mm2px(Vec(0.0, 0.0)), module, MyModule::XYZ_PARAM));
	}

	void appendContextMenu(Menu* menu) override {
		MyModule* module = getModule<MyModule>();
		
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
		MyModule* module = getModule<MyModule>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/MyModule.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelMyModule = createModel<MyModule, MyModuleWidget>("MyModule");