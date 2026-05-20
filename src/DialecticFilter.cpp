//Module Slug DialecticFilter - find and replace this to make a new one

#include "plugin.hpp"
#include "mydsp.hpp"
#include "mymath.hpp"

struct DialecticFilter : Module {
	int theme = -1;
	
	enum ParamId {
		CUTOFF_PARAM,
		HARSH_PARAM,
		RESO_PARAM,
		BIAS_PARAM,
		HARSH_SWITCH,
		CUTOFF_CV_PARAM,
		HARSH_CV_PARAM,
		RESO_CV_PARAM,
		BIAS_CV_PARAM,
		MODE_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		CUTOFF_INPUT,
		HARSH_INPUT,
		RESO_INPUT,
		BIAS_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		LOGIC_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	DialecticFilter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CUTOFF_PARAM,2,14.3,9.15,"Cutoff","Hz",2);
		configParam(HARSH_PARAM,0,5,2.5,"Bite");
		configParam(BIAS_PARAM,-5,5,0,"Bias"," volts");
		configParam(RESO_PARAM,0,0.99,0,"Resonance","%",0,100,0);
		configSwitch(HARSH_SWITCH,0,1,0,"Bite x Cutoff",{"Disabled","Enabled"});
		configSwitch(MODE_SWITCH,0,1,0,"Mode",{"A","B"});
		configParam(CUTOFF_CV_PARAM,-1,1,0,"Cutoff CV");
		configParam(HARSH_CV_PARAM,-1,1,0,"Bite CV");
		configParam(RESO_CV_PARAM,-1,1,0,"Resonance CV");
		configParam(BIAS_CV_PARAM,-1,1,0,"Bias CV");
		configBypass(AUDIO_INPUT,AUDIO_OUTPUT);
		configInput(AUDIO_INPUT,"Audio");
		configInput(CUTOFF_INPUT,"Cutoff CV");
		configInput(HARSH_INPUT,"Bite CV");
		configInput(BIAS_INPUT,"Bias CV");
		configInput(RESO_INPUT,"Resonance CV");
		configOutput(AUDIO_OUTPUT,"Audio");
		configOutput(LOGIC_OUTPUT,"Flips");
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

	CytomicAllpass ap[16];
	dsp::RCFilter lp[16];
	dsp::RCFilter hp[16];
	float lerp[16];
	bool mode;
	float out[16];
	float out2[16];

    void process(const ProcessArgs& args) override {
		int voices = inputs[AUDIO_INPUT].getChannels();
		outputs[AUDIO_OUTPUT].setChannels(voices);
		outputs[LOGIC_OUTPUT].setChannels(voices);
		for(int i = 0; i < voices; i++){
			float audio = inputs[AUDIO_INPUT].getVoltage(i);
			float intmod = abs(audio);
			float cutoff = dsp::exp2_taylor5(params[CUTOFF_PARAM].getValue()+params[CUTOFF_CV_PARAM].getValue()*inputs[CUTOFF_INPUT].getNormalVoltage(intmod,i));
			cutoff = clamp(cutoff,0.f,args.sampleRate);
			float harsh;
			if(params[HARSH_SWITCH].getValue()){
				harsh = dsp::exp2_taylor5(params[HARSH_PARAM].getValue()+params[HARSH_CV_PARAM].getValue()*inputs[HARSH_INPUT].getNormalVoltage(intmod,i))*cutoff;
			}else{
				harsh = dsp::exp2_taylor5(params[HARSH_PARAM].getValue()*1.5f-7.5f+params[HARSH_CV_PARAM].getValue()*inputs[HARSH_INPUT].getNormalVoltage(intmod,i))*40000.f;
			}
			float reso = clamp(params[RESO_PARAM].getValue()+params[RESO_CV_PARAM].getValue()*inputs[RESO_INPUT].getNormalVoltage(intmod,i)/10);
			float sRatioLerp = clamp(args.sampleRate/44100.f-1);
			float bias = params[BIAS_PARAM].getValue()+params[BIAS_CV_PARAM].getValue()*inputs[BIAS_INPUT].getNormalVoltage(intmod,i);
			mode = params[MODE_SWITCH].getValue();

			//Sample rate bullshit
			float prev = out2[i]*sRatioLerp + out[i]*(1-sRatioLerp);

			//Feedback
			hp[i].process(prev);
			audio += hp[i].highpass()*reso;
			audio = clamp(audio,-1000.f,1000.f);

			//Filter Cutoffs
			ap[i].setCutoff(cutoff,args.sampleRate);
			lp[i].setCutoffFreq(harsh*args.sampleTime);
			hp[i].setCutoffFreq(cutoff*args.sampleTime);
			ap[i].process(audio);
			float phased = ap[i].get();

			//Compare allpassed and input
			lerp[i] = (abs(phased)>abs(audio+bias))*mode+(abs(phased)<=abs(audio+bias))*!mode;
			lp[i].process(lerp[i]);
			lerp[i] = lp[i].lowpass();

			//Crossfade
			out2[i] = out[i];
			out[i] = lerp[i]*phased+(1.f-lerp[i])*audio;
			//Output
			outputs[AUDIO_OUTPUT].setVoltage(softClip(out[i],10,0.5),i);
			outputs[LOGIC_OUTPUT].setVoltage(lerp[i]*10.f,i);
		}
	}
};


struct DialecticFilterWidget : ModuleWidget {
	int theme = -1;

	DialecticFilterWidget(DialecticFilter* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/DeadMeat.svg",//"res/panels/DialecticFilter.svg",
			getPalette(PAL_TANGERINE)
		));
        //Add widgets here: 
        //addParam(createParamCentered<QKnob8mm>(mm2px(Vec(0.0, 0.0)), module, DialecticFilter::XYZ_PARAM));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 114.5)), module, DialecticFilter::AUDIO_INPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(17.78, 84.0)), module, DialecticFilter::LOGIC_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(29.21, 114.5)), module, DialecticFilter::AUDIO_OUTPUT));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(10.16, 29.0)), module, DialecticFilter::CUTOFF_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(25.4, 29.0)), module, DialecticFilter::HARSH_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(10.16, 45.0)), module, DialecticFilter::RESO_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(25.4, 45.0)), module, DialecticFilter::BIAS_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(17.78, 62.0)), module, DialecticFilter::HARSH_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(29.334, 14.0)), module, DialecticFilter::MODE_SWITCH));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 100.5)), module, DialecticFilter::CUTOFF_INPUT));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(6.35, 100.5-13)), module, DialecticFilter::CUTOFF_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(29.21, 100.5)), module, DialecticFilter::HARSH_INPUT));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(29.21, 100.5-13)), module, DialecticFilter::HARSH_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(29.21, 74)), module, DialecticFilter::BIAS_INPUT));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(29.21, 74-13)), module, DialecticFilter::BIAS_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 74)), module, DialecticFilter::RESO_INPUT));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(6.35, 74-13)), module, DialecticFilter::RESO_CV_PARAM));


	}

	void appendContextMenu(Menu* menu) override {
		DialecticFilter* module = getModule<DialecticFilter>();
		
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
		DialecticFilter* module = getModule<DialecticFilter>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/DeadMeat.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelDialecticFilter = createModel<DialecticFilter, DialecticFilterWidget>("DialecticFilter");