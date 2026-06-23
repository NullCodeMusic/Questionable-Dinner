//Module Slug CLatch - find and replace this to make a new one

#include "plugin.hpp"

struct CLatch : Module {
	int theme = -1;
	
	enum ParamId {
		TIME_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		MASTER_IN=8,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN=8
	};
	enum LightId {
		LIGHTS_LEN=2
	};
	
	CLatch() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(TIME_SWITCH,0,2,0,"Tolerance",{"1ms","16ms","64ms"});
		configInput(MASTER_IN,"Master Gate/Clock");
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

	bool queuedUp[8];
	bool queuedDown[8];
	bool outGates[8];
	dsp::PulseGenerator leewayUp;
	dsp::PulseGenerator leewayDown;
	float lastVals[INPUTS_LEN] = {0};
	float trigLengths[3] = {0.001,0.016,0.064};
    void process(const ProcessArgs& args) override {
		float len = trigLengths[(int)params[TIME_SWITCH].getValue()];

		if(lastVals[MASTER_IN]<inputs[MASTER_IN].getVoltage()){
			leewayUp.trigger(len);
		}
		if(lastVals[MASTER_IN]>inputs[MASTER_IN].getVoltage()){
			leewayDown.trigger(len);
		}
		for(int i = 0; i<8;i++){
			if(inputs[i].getVoltage()>lastVals[i]){
				queuedUp[i] = true;
			}else if(inputs[i].getVoltage()<lastVals[i]||(inputs[i].getVoltage()==0&&inputs[i].getVoltage()==lastVals[i])){
				queuedDown[i] = true;
			}
			if(leewayUp.isHigh()&&queuedUp[i]){
				outGates[i] = true;
				queuedUp[i] = false;
			}else if(leewayDown.isHigh()&&queuedDown[i]){
				outGates[i] = false;
				queuedDown[i] = false;
			}
			
			outputs[i].setVoltage(outGates[i]*10);
		}
		for(int i =0;i<INPUTS_LEN;i++){
			lastVals[i]=inputs[i].getVoltage();
		}
		leewayUp.process(args.sampleTime);
		leewayDown.process(args.sampleTime);
	}
};


struct CLatchWidget : ModuleWidget {
	int theme = -1;

	CLatchWidget(CLatch* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/CLatch.svg",
			getPalette(PAL_LIGHT)
		));
		const float UHEIGHT = 12;
		addParam(createLightParamCentered<VCVLightLatch<SmallSimpleLight<GreenRedLight>>>(mm2px(Vec(18.415, 13.00)), module, CLatch::TIME_SWITCH, 0));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.985,13.00)),module,CLatch::MASTER_IN));
		for(int i = 0;i<8;i++){
			addInput(createInputCentered<QPort>(mm2px(Vec(6.985,30.50+UHEIGHT*i)),module,i));
			addOutput(createOutputCentered<QPort>(mm2px(Vec(18.415,30.50+UHEIGHT*i)),module,i));
		}
        //Add widgets here: 
        //addParam(createParamCentered<QKnob8mm>(mm2px(Vec(0.0, 0.0)), module, CLatch::XYZ_PARAM));
	}

	void appendContextMenu(Menu* menu) override {
		CLatch* module = getModule<CLatch>();
		
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
		CLatch* module = getModule<CLatch>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/CLatch.svg",
				getPalette(theme)
			));
		}
		switch((int) module->params[CLatch::TIME_SWITCH].getValue()){
			case 0:
				module->lights[0].setBrightness(1.f);
				module->lights[1].setBrightness(0.f);
				break;
			case 1:
				module->lights[0].setBrightness(.8f);
				module->lights[1].setBrightness(.8f);
				break;
			case 2:
				module->lights[0].setBrightness(0.f);
				module->lights[1].setBrightness(1.f);
				break;
		}
	}
};


Model* modelCLatch = createModel<CLatch, CLatchWidget>("CLatch");