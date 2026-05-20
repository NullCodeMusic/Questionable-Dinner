//Module Slug Simmer - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"

struct Simmer : Module {
	int theme = -1;
	
	enum ParamId {
		SCALE_PARAM,
		RANGE_PARAM,
		EDGE_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		U1_INPUT,
		D1_INPUT,
		W1_INPUT,
		U2_INPUT,
		D2_INPUT,
		W2_INPUT,
		U5_INPUT,
		D5_INPUT,
		W5_INPUT,
		U7_INPUT,
		D7_INPUT,
		W7_INPUT,
		U12_INPUT,
		D12_INPUT,
		W12_INPUT,
		RST_NOTE_INPUT,
		RST_LOGIC_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VOCT_OUTPUT,
		TRIG_OUTPUT,
		EDGE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		VOCT_LIGHT,
		TRIG_LIGHT,
		EDGE_LIGHT,
		LIGHTS_LEN
	};
	
	Simmer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(EDGE_SWITCH,0,1,0,"Edge Behavior",{"Wrap","Kill"});
		configInput(0,"1 Step Up");
		configInput(1,"1 Step Down");
		configInput(2,"1 Step Alternating");
		configInput(3,"2 Steps Up");
		configInput(4,"2 Steps Down");
		configInput(5,"2 Steps Alternating");
		configInput(6,"5 Steps Up");
		configInput(7,"5 Steps Down");
		configInput(8,"5 Steps Alternating");
		configInput(9,"7 Steps Up");
		configInput(10,"7 Steps Down");
		configInput(11,"7 Steps Alternating");
		configInput(12,"12 Steps Up");
		configInput(13,"12 Steps Down");
		configInput(14,"12 Steps Alternating");
		configInput(15,"Reset Note");
		configInput(16,"Reset Logic");
		configParam(SCALE_PARAM,4,24,12,"Scale"," notes per octave")->snapEnabled=true;
		configParam(RANGE_PARAM,2,12,4,"Range"," volts",0,0.5,0)->snapEnabled=true;
		configOutput(VOCT_OUTPUT,"V/Oct");
		configOutput(TRIG_OUTPUT,"Trigger Through");
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

	bool trigs[INPUTS_LEN] = {false}; 
	bool gates[INPUTS_LEN] = {false};
	float voltages[INPUTS_LEN] = {false};
	bool logic[5] = {true};
	float note = 0;
	float offsets[5] = {1,2,5,7,12};
	dsp::PulseGenerator trigOut;
	dsp::PulseGenerator edgeOut;

    void process(const ProcessArgs& args) override {
		float range = params[RANGE_PARAM].getValue()/2.f;
		float scale = params[SCALE_PARAM].getValue();
		float edgeBehavior = params[EDGE_SWITCH].getValue();
		bool sendTrig = false;
		updateTrigs();
		//Reset
		if(trigs[RST_LOGIC_INPUT]){
			logic[0]=true;
			logic[1]=true;
			logic[2]=true;
			logic[3]=true;
			logic[4]=true;
		}
		if(trigs[RST_NOTE_INPUT]){
			note = 0;
		}

		//Offset note via trigs
		for(int i=0; i<5; i++){
			float alt = float(logic[i])*2.f-1.f;
			note = note+(float(trigs[U1_INPUT+i*3])-float(trigs[D1_INPUT+i*3])+float(trigs[W1_INPUT+i*3])*alt)*offsets[i];
			sendTrig = sendTrig||trigs[U1_INPUT+i*3]||trigs[D1_INPUT+i*3]||trigs[W1_INPUT+i*3];
			if(float(trigs[W1_INPUT+i*3])){
				logic[i]=!logic[i];
			}
		}

		//Send output trigger if any input triggers detected
		if(sendTrig){
			trigOut.trigger();
			lights[TRIG_LIGHT].setBrightness(1);
		}

		//Calculate note voltage
		float voltage = note/scale;
		if(voltage>range/2&&edgeBehavior==0){
			voltage -= range;
			note -= range*scale;
			edgeOut.trigger();
			lights[EDGE_LIGHT].setBrightness(1);
		}
		if(voltage<-range/2&&edgeBehavior==0){
			voltage += range;
			note += range*scale;
			edgeOut.trigger();
		}
		if(edgeBehavior>0&&(voltage<-range/2||voltage>range/2)){
			voltage = 0;
			note = 0;
			edgeOut.trigger();
		}

		//Process trigger generators
		edgeOut.process(args.sampleTime);
		trigOut.process(args.sampleTime);

		//Send outputs
		outputs[EDGE_OUTPUT].setVoltage(edgeOut.isHigh()*10);
		outputs[TRIG_OUTPUT].setVoltage(trigOut.isHigh()*10);
		outputs[VOCT_OUTPUT].setVoltage(voltage);
		lights[VOCT_LIGHT].setBrightness(voltage/range+0.5);
	}

	void updateTrigs(){
		for(int i = 0; i<INPUTS_LEN; i++){
			trigs[i]=false;
			if(inputs[i].getVoltage()>voltages[i]&&!gates[i]){//rising
				trigs[i] = true;
				gates[i] = true;
			}
			if(inputs[i].getVoltage()<voltages[i]){//falling
				gates[i] = false;
			}
			voltages[i]=inputs[i].getVoltage();
		}
	}
};


struct SimmerWidget : ModuleWidget {
	int theme = -1;

	SimmerWidget(Simmer* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Simmer.svg",
			getPalette(PAL_BUBBLEGUM)
		));
        //Add widgets here: 
		constexpr double column[5] = {7.62,19.05,30.48,48.26,53.34};
		constexpr double row = 44.5; //+-14
        addParam(createParamCentered<QKnob18mm>(mm2px(Vec(15.24, 23.00)), module, Simmer::SCALE_PARAM));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(40.64, 23.00)), module, Simmer::RANGE_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(column[3], row+14*5)), module, Simmer::EDGE_SWITCH));
		for(int i = 0; i<5; i++){
			addInput(createInputCentered<QPort>(mm2px(Vec(column[0], row+14*i)), module, Simmer::U1_INPUT+3*i));
			addInput(createInputCentered<QPort>(mm2px(Vec(column[1], row+14*i)), module, Simmer::D1_INPUT+3*i));
			addInput(createInputCentered<QPort>(mm2px(Vec(column[2], row+14*i)), module, Simmer::W1_INPUT+3*i));
		}
		addInput(createInputCentered<QPort>(mm2px(Vec(column[0], row+14*5)), module, Simmer::RST_NOTE_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(column[1], row+14*5)), module, Simmer::RST_LOGIC_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(column[3], row)), module, Simmer::VOCT_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(column[3], row+14)), module, Simmer::TRIG_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(column[3], row+28)), module, Simmer::EDGE_OUTPUT));

		addChild(createLightCentered<SmallSimpleLight<BlueLight>>(mm2px(Vec(column[4], row)),module,Simmer::VOCT_LIGHT));
		addChild(createLightCentered<SmallSimpleLight<BlueLight>>(mm2px(Vec(column[4], row+14)),module,Simmer::TRIG_LIGHT));
		addChild(createLightCentered<SmallSimpleLight<BlueLight>>(mm2px(Vec(column[4], row+28)),module,Simmer::EDGE_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Simmer* module = getModule<Simmer>();
		
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
		Simmer* module = getModule<Simmer>();
		if(!module){
			return;
		}
		module->lights[Simmer::EDGE_LIGHT].setBrightness(module->lights[Simmer::EDGE_LIGHT].getBrightness()*0.7);
		module->lights[Simmer::TRIG_LIGHT].setBrightness(module->lights[Simmer::TRIG_LIGHT].getBrightness()*0.7);
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Simmer.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelSimmer = createModel<Simmer, SimmerWidget>("Simmer");