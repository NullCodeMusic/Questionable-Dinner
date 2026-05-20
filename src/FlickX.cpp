#include "plugin.hpp"

struct FlickX : Module {
	int theme = -1;
	
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		VOCTA_INPUT,
		LEVELA_INPUT,
		VOCTB_INPUT,
		LEVELB_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	FlickX() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(VOCTA_INPUT, "v/Oct A");
		configInput(VOCTB_INPUT, "v/Oct B");

		configInput(LEVELA_INPUT, "Level A");
		configInput(LEVELB_INPUT, "Level B");
	}

};


struct FlickXWidget : ModuleWidget {
	int theme = -1;

	FlickXWidget(FlickX* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/FlickEXT.svg",
			getPalette(PAL_LIGHT)
		));

		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 11.0)), module, FlickX::LEVELA_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 22.0)), module, FlickX::VOCTA_INPUT));

		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 70.0)), module, FlickX::LEVELB_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(5.08, 81.0)), module, FlickX::VOCTB_INPUT));
	}

	void appendContextMenu(Menu* menu) override {
		FlickX* module = getModule<FlickX>();
		
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
		FlickX* module = getModule<FlickX>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/FlickEXT.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelFlickX = createModel<FlickX, FlickXWidget>("FlickX");