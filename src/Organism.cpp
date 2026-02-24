//Module Slug Organism - find and replace this to make a new one

#include "plugin.hpp"
#include "additive.hpp"
#include "mymath.hpp"

using namespace simd;

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
		LPG_PARAM,
		FREQ_PARAM,
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
		configParam(ATTACK_PARAM,-10.f,2.f,-9.f,"Attack","",0.f,1.f,10.f);
		configParam(DECAY_PARAM,-10.f,2.f,-3.f,"Decay","",0.f,1.f,10.f);
		configSwitch(SPECIMEN_PARAM,0,13,0,"Specimen",{
			"Basic","Organ","Low Quality","Fish",
			"Combed Saw","Metallic","Keys","Fractal-ish",
			"Particles","Prism","Chaotic","Noise",
			"Phase Mod","Per-Partial AM"
		});
		configButton(SUSTAIN_SWITCH,"Sustain");
		configParam(FM_PARAM,-1.f,1.f,0,"FM Amount");
		configParam(NUMBER_PARAM,0.f,7.f,4.f,"Partials","",2.0);
		configParam(LPG_PARAM,0,1,0.1,"LPG Curve");
		configParam(FREQ_PARAM,-3,3,0,"Tune"," Semitones",0,12);

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

	additive::Algorithm* specimens[16][14] = {
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM},
		{new additive::Basic,new additive::Organ,new additive::LowQual,new additive::Fish,
		new additive::CombedSaw,new additive::Metallic,new additive::PianoBrass,new additive::Fractal,
		new additive::Particles,new additive::Prism,new additive::Chaotic,
		new additive::Noise,new additive::PhaseMod,new additive::PerPartialAM}
	};
	float env[16] = {0};
	int envStage[16] = {0};
	float_4 phases[16][32] = {0};
	float_4 mask = {1,0,0,0};
	bool queuedReset[16] = {false};
	float lastAudio[16] = {0};
	int algo = 0;
	int number = 1;
	int maxidx = 0;

    void process(const ProcessArgs& args) override {

		//Nyquist
		float nyquist = args.sampleRate/2;

		//Number
		if(number != int(dsp::exp2_taylor5(params[NUMBER_PARAM].getValue()))){
			setNumPartials(dsp::exp2_taylor5(params[NUMBER_PARAM].getValue()));
		}

		//Declaring all float_4 variables!
		float_4 out = 0;
		float_4 lpg = 0;
		float_4 i = 0;
		float_4 pFreq = 0;
		float_4 freqMask = 0;

		//Polyphony
		int channels = std::max({
			inputs[0].getChannels(),
			inputs[1].getChannels(),
			inputs[2].getChannels(),
			inputs[3].getChannels(),
			inputs[5].getChannels(),
			1
		});
		int idx;
		outputs[AUDIO_OUTPUT].setChannels(channels);

		for(int v = 0;v<channels;v++){
		

		//Params
		float freq = dsp::FREQ_C4*dsp::exp2_taylor5(inputs[VOCT_INPUT].getVoltage(v)+params[FREQ_PARAM].getValue());
		float lpgParam = params[LPG_PARAM].getValue();
		lpgParam *= lpgParam;

		//ENV
		processEnv(args.sampleTime,v);

		//Get cvs
		freq *= dsp::exp2_taylor5(inputs[FM_INPUT].getNormalVoltage(sin_2pi_9(phases[v][0][0]),v)*params[FM_PARAM].getValue());
		float intlMod = env[v]*env[v]*10.f;
		float structure = params[STRUCT_PARAM].getValue()+params[STRUCT_CV_PARAM].getValue()*inputs[STRUCT_CV_INPUT].getNormalVoltage(intlMod,v)/5.f;
		structure = clamp(structure,-1.f,1.f);
		float morph = params[MORPH_PARAM].getValue()+params[MORPH_CV_PARAM].getValue()*inputs[MORPH_CV_INPUT].getNormalVoltage(intlMod,v)/5.f;
		morph = clamp(morph,-1.f,1.f);
		algo = (params[SPECIMEN_PARAM].getValue()+params[SPECIMEN_CV_PARAM].getValue()*inputs[SPECIMEN_CV_INPUT].getNormalVoltage(intlMod,v)/5.f*14);
		algo = (algo+28) % 14;

		//MAIN OSC
		specimens[v][algo]->process(freq,number,args.sampleTime,lastAudio[v],structure,morph);
		out = 0;
		float lpgCurve = 1+(lpgParam)*env[v]-(lpgParam);
		lpg = {
			1,
			lpgCurve,
			lpgCurve*lpgCurve,
			lpgCurve*lpgCurve*lpgCurve};
		float lpg4 = lpg[2]*lpg[2];
		i = {1,2,3,4};
		
		pFreq = 0;
		freqMask = 1;
		
		for(idx = 0; idx < maxidx; idx++){
			phases[v][idx] -= floor(phases[v][idx]);
			pFreq = i*freq*specimens[v][algo]->partials[PI_FREQ][idx];
			freqMask = (pFreq<=nyquist)&(pFreq>=-nyquist);
			out += sin_2pi_9(phases[v][idx]+specimens[v][algo]->partials[PI_PHASE][idx])*specimens[v][algo]->partials[PI_AMP][idx]*lpg&freqMask;
			phases[v][idx] += args.sampleTime * pFreq;
			i+=4;
			lpg*=lpg4;
		}
		phases[v][idx] -= floor(phases[v][idx]);
		pFreq = i*freq*specimens[v][algo]->partials[PI_FREQ][idx];
		freqMask = (pFreq<=nyquist)&(pFreq>=-nyquist);
		out += sin_2pi_9(phases[v][idx]+specimens[v][algo]->partials[PI_PHASE][idx])*specimens[v][algo]->partials[PI_AMP][idx]*mask*lpg&freqMask;
		phases[v][idx] += args.sampleTime*i*freq*specimens[v][algo]->partials[PI_FREQ][idx];

		float sum = (out[0]+out[1]+out[2]+out[3])*env[v];

		//Reset Phases without clicking
		if(queuedReset[v] && sum>0 && lastAudio[v]<=0){
			for(int i = 0; i < 16; i++){
				phases[v][i] = {0,0,0,0};
			}
			specimens[v][algo]->reset();
			queuedReset[v] = false;
		}
		
		//Output
		outputs[AUDIO_OUTPUT].setVoltage(softClip(sum*2.5,10,0.5),v);
		lastAudio[v] = sum;
		}
	}

	void processEnv(float deltaTime, int voice){
		//Stages
			//0 - Decay
			//1 - Attack
			//2 - Sustain
			//-1 - Release/Stable

		if(!inputs[GATE_INPUT].isConnected()){
			env[voice] = 1;
			return;
		}

		float attack = 1/dsp::exp2_taylor5(params[ATTACK_PARAM].getValue());
		float decay = 1/dsp::exp2_taylor5(params[DECAY_PARAM].getValue());
		bool sustain = params[SUSTAIN_SWITCH].getValue();
		float gate = inputs[GATE_INPUT].getVoltage(voice)/10;

		if(gate>0 && envStage[voice] == -1){
			envStage[voice] = 1;
			queuedReset[voice] = specimens[voice][algo]->phaseResettable;
		}
		if(env[voice]>=gate && sustain && envStage[voice] == 1){
			envStage[voice] = 2;
		}
		if(env[voice]>=gate && !sustain){
			envStage[voice] = 0;
		}
		if(gate<=0){
			envStage[voice] = -1;
		}

		switch(envStage[voice]){
			case 0:
			case -1:
				env[voice]-=decay*deltaTime*(0.1+env[voice]);
				break;
			case 1:
				env[voice]+=attack*deltaTime*(1.1-env[voice]);
				break;
			case 2:
				env[voice]=gate;
				break;
		}
		env[voice] = clamp(env[voice],0.f,1.f);
	}

	void setNumPartials(int newNumber){
		number = newNumber;
		queuedReset[0] = !inputs[GATE_INPUT].isConnected();
		switch (number % 4)
		{
		default:
		case 0:
			mask = {1,1,1,1};
			break;
		case 1:
			mask = {1,0,0,0};
			break;
		case 2:
			mask = {1,1,0,0};
			break;
		case 3:
			mask = {1,1,1,0};
			break;
		}
		maxidx = (number-1)/4;
	}
};


struct OrganismWidget : ModuleWidget {
	int theme = -1;
	QInfoText* info;

	OrganismWidget(Organism* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/OrganismBig.svg",
			getPalette(PAL_SNOT)
		));
        //Add widgets here: 
		info = createWidget<QInfoText>(mm2px(Vec(5.905, 18.864)));
		info->text = "0000";
		info->size = 14;
		addChild(info);
		
        addParam(createParamCentered<QKnob10mm>(mm2px(Vec(22.860, 30)), module, Organism::SPECIMEN_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(10.16, 37.5)), module, Organism::STRUCT_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35.560, 37.5)), module, Organism::MORPH_PARAM));

		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(7.62, 55.0)), module, Organism::STRUCT_CV_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(22.860, 55.0)), module, Organism::SPECIMEN_CV_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(38.100, 55.0)), module, Organism::MORPH_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 68.0)), module, Organism::STRUCT_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(22.860, 68.0)), module, Organism::SPECIMEN_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(38.100, 68.0)), module, Organism::MORPH_CV_INPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 82.0)), module, Organism::ATTACK_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(17.78, 82.0)), module, Organism::DECAY_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(38.1, 82.0)), module, Organism::LPG_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(27.9, 82.0)), module, Organism::SUSTAIN_SWITCH));
		

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 114.5)), module, Organism::VOCT_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(38.100, 101.5)), module, Organism::FM_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(27.94, 101.5)), module, Organism::FM_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(12.7, 101.5)), module, Organism::FREQ_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(22.86, 114.5)), module, Organism::GATE_INPUT));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(34.290, 19.5)), module, Organism::NUMBER_PARAM));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(38.100, 114.5)), module, Organism::AUDIO_OUTPUT));
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
				"res/panels/OrganismBig.svg",
				getPalette(theme)
			));
		}
		info->text = module->specimens[0][module->algo]->text;
	}
};


Model* modelOrganism = createModel<Organism, OrganismWidget>("Organism");