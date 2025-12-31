//Module Slug Organism - find and replace this to make a new one

#include "plugin.hpp"
#include "lookups.hpp"
#include "additive.hpp"

struct Organism : Module {
	int theme = -1;
	
	enum ParamId {
		SPECIMEN_PARAM,
		STRUCT_PARAM,
		MORPH_PARAM,
		SPECIMEN_CV_PARAM,
		STRUCT_CV_PARAM,
		MORPH_CV_PARAM,
		ATTACK_PARAM,
		DECAY_PARAM,
		SUSTAIN_SWITCH,
		FM_PARAM,
		NUMBER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SPECIMEN_CV_INPUT,
		STRUCT_CV_INPUT,
		MORPH_CV_INPUT,
		VOCT_INPUT,
		FM_INPUT,
		GATE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Organism() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(STRUCT_PARAM,-1.f,1.f,0,"Structure");
		configParam(MORPH_PARAM,-1.f,1.f,0,"Morph");
		configParam(STRUCT_CV_PARAM,-1.f,1.f,0,"Structure CV");
		configParam(MORPH_CV_PARAM,-1.f,1.f,0,"Morph CV");
		configParam(SPECIMEN_CV_PARAM,-1.f,1.f,0,"Specimen CV");
		configParam(ATTACK_PARAM,-9.f,2.f,-9.f,"Attack","",0.f,1.f,9.f);
		configParam(DECAY_PARAM,-9.f,2.f,-3.f,"Decay","",0.f,1.f,9.f);
		configButton(SUSTAIN_SWITCH,"Sustain");
		configParam(FM_PARAM,-1.f,1.f,0,"FM Amount");
		configParam(NUMBER_PARAM,1.f,64.f,0,"Partials")->snapEnabled=true;

		configInput(SPECIMEN_CV_INPUT,"Specimen CV");
		configInput(STRUCT_CV_INPUT,"Structure CV");
		configInput(MORPH_CV_INPUT,"Morph CV");
		configInput(VOCT_INPUT,"V/Oct");
		configInput(FM_INPUT,"FM");
		configInput(SPECIMEN_CV_INPUT,"Gate");

		configOutput(AUDIO_OUTPUT,"Audio");
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

	Algorithm* specimens[1] = {new Basic};
	float env = 0;
	int envStage = 0;
	float phases[128] = {0};
	bool queuedReset = false;
	float lastAudio = 0;


    void process(const ProcessArgs& args) override {
		//Get Params
		int number = params[NUMBER_PARAM].getValue();
		float freq = dsp::FREQ_C4*dsp::exp2_taylor5(inputs[VOCT_INPUT].getVoltage());
		float structure = params[STRUCT_PARAM].getValue();
		float morph = params[MORPH_PARAM].getValue();
		int algo = 0;

		//ENV
		processEnv(args.sampleTime);

		//MAIN OSC
		
		specimens[algo]->process(freq,number,args.sampleTime,env,lastAudio,structure,morph);

		float out = 0;
		for(int i = 0; i < number; i++){
			out += lu_sin(phases[i]+specimens[algo]->partials[PI_PHASE][i])*specimens[algo]->partials[PI_AMP][i];
			phases[i] += args.sampleTime*(i+1)*freq*specimens[algo]->partials[PI_FREQ][i];
			phases[i] -= floor(phases[i]);
		}

		//Reset Phases without clicking
		if(queuedReset && out>=0 && lastAudio<=0){
			for(int i = 0; i < number; i++){
				phases[i] = 0;
			}
			queuedReset = false;
		}
		
		//Output
		outputs[AUDIO_OUTPUT].setVoltage(env*out);
		lastAudio = env*out;
	}

	void processEnv(float deltaTime){
		//Stages
			//0 - Decay
			//1 - Attack
			//2 - Sustain
			//-1 - Release/Stable

		if(!inputs[GATE_INPUT].isConnected()){
			env = 1;
			return;
		}

		float attack = 1/dsp::exp2_taylor5(params[ATTACK_PARAM].getValue());
		float decay = 1/dsp::exp2_taylor5(params[DECAY_PARAM].getValue());
		bool sustain = params[SUSTAIN_SWITCH].getValue();
		float gate = inputs[GATE_INPUT].getVoltage()/10;

		if(gate>0 && envStage == -1){
			envStage = 1;
			queuedReset = true;
		}
		if(env>=gate && sustain && envStage == 1){
			envStage = 2;
		}
		if(env>=gate && !sustain){
			envStage = 0;
		}
		if(!gate){
			envStage = -1;
		}

		switch(envStage){
			case 0:
			case -1:
				env-=decay*deltaTime*(0.1+env);
				break;
			case 1:
				env+=attack*deltaTime*(1.1-env);
				break;
			case 2:
				env=gate;
				break;
		}
		env = clamp(env,0.f,1.f);
	}

};


struct OrganismWidget : ModuleWidget {
	int theme = -1;
	Widget* info;

	OrganismWidget(Organism* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Organism.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
		QInfoText* info = createWidget<QInfoText>(mm2px(Vec(4.635, 19.864)));
		info->text = "0000";
		info->size = 14;
		addChild(info);
		
        addParam(createParamCentered<QKnob10mm>(mm2px(Vec(26.75, 20.5)), module, Organism::SPECIMEN_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(8.89, 36.5)), module, Organism::STRUCT_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(26.67, 36.5)), module, Organism::MORPH_PARAM));

		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(6.35, 55.0)), module, Organism::STRUCT_CV_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(17.78, 55.0)), module, Organism::SPECIMEN_CV_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(29.2, 55.0)), module, Organism::MORPH_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 68.0)), module, Organism::STRUCT_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(17.78, 68.0)), module, Organism::SPECIMEN_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(29.2, 68.0)), module, Organism::MORPH_CV_INPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.35, 82.0)), module, Organism::ATTACK_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(17.78, 82.0)), module, Organism::DECAY_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(29.2, 82.0)), module, Organism::SUSTAIN_SWITCH));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 101.5)), module, Organism::VOCT_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(17.78, 101.5)), module, Organism::FM_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(29.2, 101.5)), module, Organism::FM_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 114.5)), module, Organism::GATE_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(17.78, 114.5)), module, Organism::NUMBER_PARAM));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(29.2, 114.5)), module, Organism::AUDIO_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Organism* module = getModule<Organism>();
		
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
		Organism* module = getModule<Organism>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Organism.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelOrganism = createModel<Organism, OrganismWidget>("Organism");