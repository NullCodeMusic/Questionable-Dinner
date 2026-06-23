//Module Slug Convection - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"
#include "ConvectionBase.hpp"

struct Convection : Module {
	int theme = -1;
	
	enum ParamId {
		UTYPE_SWITCH,
		ULEN_PARAM = 4,
		USCALE_PARAM = 8,
		UDENS_PARAM = 12,
		USKEW_PARAM = 16,
		TRIG_SWITCH = 20,
		UAMP_PARAM,
		LENSCALE_PARAM = 25,
		PARAMS_LEN
	};
	enum InputId {
		UTRIG_IN,
		RESET_IN = 4,
		INPUTS_LEN
	};
	enum OutputId {
		UEOC_OUT,
		UDIRECT_OUT = 4,
		RUN_OUT = 8,
		TRIG_OUT,
		SUM_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=12
	};

	float colors[7][3] = {
		{1.0f,0.0f, 0.0f},
		{0.0f,1.0f, 0.0f},
		{0.8f,0.4f, 0.0f},
		{0.0f,0.0f, 1.0f},
		{1.0f,1.0f, 1.0f},
	};

	void setLights(int seg, float arr[], float scale){
		lights[seg*3].setBrightness(arr[0]*scale);
		lights[seg*3+1].setBrightness(arr[1]*scale);
		lights[seg*3+2].setBrightness(arr[2]*scale);
	}
	
	Convection() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for(int i = 0; i < 4; i++){
			configSwitch(UTYPE_SWITCH+i,0,4,i,"Segment "+std::to_string(i+1)+" Type",{"Up","Down","Up-down","Down-up","Constant"});
			configParam(ULEN_PARAM+i,-2,4,1,"Segment "+std::to_string(i+1)+" Length","x",2);
			configParam(USCALE_PARAM+i,0,1,1,"Segment "+std::to_string(i+1)+" Completion","%",0,100);
			configParam(UDENS_PARAM+i,1,8,2,"Segment "+std::to_string(i+1)+" Trig Density","");
			configParam(USKEW_PARAM+i,-1,1,0,"Segment "+std::to_string(i+1)+" Skew");
			configParam(UAMP_PARAM+i,0,2,1,"Segment "+std::to_string(i+1)+" Amp");

			configInput(UTRIG_IN+i,"Segment "+std::to_string(i+1)+" Trig");
			configOutput(UEOC_OUT+i,"Segment "+std::to_string(i+1)+" End-of-Cycle");
			configOutput(UDIRECT_OUT+i,"Segment "+std::to_string(i+1)+" Exclusive CV");
		}
		configParam(LENSCALE_PARAM,6,10,8,"Time Scale","ms",2);
		configInput(RESET_IN, "Reset");
		configOutput(RUN_OUT, "Running CV");
		configOutput(SUM_OUT,"Scaled sum CV");
		configOutput(TRIG_OUT,"Trig");
		configSwitch(TRIG_SWITCH,0,1,0,"Trigger Distribution",{"X (Time)","Y (Current Value)"});
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

	Segment segments[4];
	float run=0;
	float sum=0;
	float prevTrigs[5] = {0};


    void process(const ProcessArgs& args) override {
		//RESET ON TRIG
		bool reset = false;
		if(inputs[RESET_IN].getVoltage() > prevTrigs[4]){
			run = 0;
			segments[0].reset();
			segments[1].reset();
			segments[2].reset();
			segments[3].reset();
			reset = true;
		}
		prevTrigs[4] = inputs[RESET_IN].getVoltage();

		//PROCESS MAIN
		bool mainTrig = false;
		sum = 0;
		for(int i = 0; i<4; i++){
			int type = params[UTYPE_SWITCH+i].getValue();
			float scale = params[USCALE_PARAM+i].getValue();

			if(inputs[UTRIG_IN+i].getVoltage() > prevTrigs[i]){
				segments[i].trigger(params[USKEW_PARAM+i].getValue(),type);
			}
			prevTrigs[i] = inputs[UTRIG_IN+i].getVoltage();

			segments[i].process(
				args.sampleTime,
				dsp::exp2_taylor5(params[ULEN_PARAM+i].getValue()+params[LENSCALE_PARAM].getValue()),
				scale,
				params[UDENS_PARAM+i].getValue(),
				params[USKEW_PARAM+i].getValue(),
				type,
				params[TRIG_SWITCH].getValue()>0
			);

			float mult = params[UAMP_PARAM+i].getValue();
			if(outputs[UDIRECT_OUT+i].isConnected()){
				outputs[UDIRECT_OUT+i].setVoltage(segments[i].single*5.f*mult);
			}else{
				mainTrig = mainTrig||segments[i].trig.isHigh();
				run+=segments[i].slope*mult;
				sum+=segments[i].direct*2.5*mult;
			}
			outputs[UEOC_OUT+i].setVoltage(segments[i].eoc.isHigh()*10.f);
			setLights(i,colors[type],segments[i].phase/scale+(segments[i].phase==0));
		}

		//HANDLE MASTER OUTPUT
		run = clamp(run,-10.f,10.f);
		sum = clamp(sum,-10.f,10.f);
		outputs[RUN_OUT].setVoltage(run);
		outputs[SUM_OUT].setVoltage(sum);
		outputs[TRIG_OUT].setVoltage(mainTrig*10.f);
	}
};


struct ConvectionWidget : ModuleWidget {
	int theme = -1;

	ConvectionWidget(Convection* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Convection.svg",
			getPalette(PAL_LIGHT)
		));
		
		const double UHEIGHT = 25.5;
		//UNIT LAYOUT FOR LOOP
		for(int i = 0; i < 4; i++){
			addInput(createInputCentered<QPort>(mm2px(Vec(7.62,12.50+i*UHEIGHT)), module, Convection::UTRIG_IN+i));
			addOutput(createOutputCentered<QPort>(mm2px(Vec(38.10,12.50+i*UHEIGHT)), module, Convection::UEOC_OUT+i));
			addOutput(createOutputCentered<QPort>(mm2px(Vec(48.260,12.50+i*UHEIGHT)), module, Convection::UDIRECT_OUT+i));

			addParam(createLightParamCentered<VCVLightLatch<SmallSimpleLight<RedGreenBlueLight>>>(mm2px(Vec(7.62, 23.50+i*UHEIGHT)), module, Convection::UTYPE_SWITCH+i, i*3));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(17.78, 23.50+i*UHEIGHT)), module, Convection::ULEN_PARAM+i));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(22.86, 12.50+i*UHEIGHT)), module, Convection::UAMP_PARAM+i));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(27.94, 23.50+i*UHEIGHT)), module, Convection::USCALE_PARAM+i));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(38.10, 23.50+i*UHEIGHT)), module, Convection::UDENS_PARAM+i));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(48.26, 23.50+i*UHEIGHT)), module, Convection::USKEW_PARAM+i));
		}

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62,114.5)), module, Convection::RESET_IN));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(27.94,114.5)), module, Convection::RUN_OUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(17.78, 114.5)), module, Convection::LENSCALE_PARAM));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(48.260,114.5)), module, Convection::TRIG_OUT));
		addParam(createParamCentered<CKSS>(mm2px(Vec(38.10, 114.5)), module, Convection::TRIG_SWITCH));
	}

	void appendContextMenu(Menu* menu) override {
		Convection* module = getModule<Convection>();
		
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
		Convection* module = getModule<Convection>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Convection.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelConvection = createModel<Convection, ConvectionWidget>("Convection");