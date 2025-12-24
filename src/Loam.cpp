#include "plugin.hpp"
#include "mymath.hpp"

struct Loam : Module {

	//Copy Pase Channel Themes Stuff
	int theme = -1;
	void setTheme(int newTheme){
		theme = newTheme;
	}
	int getTheme(){
		return theme;
	}
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "theme", json_integer(theme));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "theme");
		if (modeJ){
			setTheme(json_integer_value(modeJ));
		}
	}
	
	//Module
	enum ParamId {
		FACTOR_PARAM,
		RES_PARAM,
		FEED_PARAM,
		CHAOS_PARAM,
		DRIVE_PARAM,
		FACTOR_MOD_PARAM,
		RES_MOD_PARAM,
		FEED_MOD_PARAM,
		CHAOS_MOD_PARAM,
		DRIVE_MOD_PARAM,
		MOD_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		POS_INPUT,
		NEG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LP_OUTPUT,
		HP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Loam() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FACTOR_PARAM, 3.f, 14.f, 8.5f, "Frequency", "", 2.f);
		configParam(RES_PARAM, 0.f, 1.f, 0.f, "Resonance");
		configParam(FEED_PARAM, 0.0f, 1.0f, 0.f, "Feedback");
		configParam(CHAOS_PARAM,0.f,0.25f,0.f,"Radioactivity");
		configInput(AUDIO_INPUT, "");
		configOutput(LP_OUTPUT, "Lowpass");
		configOutput(HP_OUTPUT, "Highpass");
		configParam(DRIVE_PARAM,-3.f,3.f,0.f,"Gain");

		configSwitch(MOD_SWITCH, 0.f, 1.f, 1.f, "Mod Relationship", {"V","/"});
		configInput(POS_INPUT, "Mod +");
		configInput(NEG_INPUT, "Mod -");
		
		configParam(FACTOR_MOD_PARAM, -1.f, 1.f, 0.f, "Freq Mod");
		configParam(RES_MOD_PARAM, -1.f, 1.f, 0.f, "Res Mod");
		configParam(FEED_MOD_PARAM, -1.f, 1.f, 0.f, "Feedback Mod");
		configParam(CHAOS_MOD_PARAM, -1.f, 1.f, 0.f, "Radioactivity Mod");
		configParam(DRIVE_MOD_PARAM, -1.f, 1.f, 0.f, "Gain Mod");

		configBypass(AUDIO_INPUT,LP_OUTPUT);
	}

	float lp[16] = {0.f};
	float lpvel[16] = {0.f};
	float magicNumber=8000.f;
	float sRatio = 1.f;
	float minFreq = 0.001;
	float outLP;
	float outHP;
	float freq;
	float res;
	float feed;
	float drive;
	float chaos;


	void onSampleRateChange(const SampleRateChangeEvent& e) override{
		Module::onSampleRateChange(e);

		//REGRESSION BASED ON PLOT OF RESONANT FREQUENCIES FREQ = a*x^b
		magicNumber = 22.95117f*pow(e.sampleRate/1000.f,1.50054f);
		sRatio = e.sampleRate/48000;
		minFreq = pow(8/magicNumber,2);
	}

	int channels = 1;

	void process(const ProcessArgs& args) override {

		channels = inputs[AUDIO_INPUT].getChannels();
		outputs[LP_OUTPUT].setChannels(channels);
		outputs[HP_OUTPUT].setChannels(channels);

		//Read Params
		float freqParam = params[FACTOR_PARAM].getValue();
		float resParam = params[RES_PARAM].getValue();
		float feedParam = params[FEED_PARAM].getValue();
		float driveParam = params[DRIVE_PARAM].getValue();
		float chaosParam = params[CHAOS_PARAM].getValue();

		float freqModParam = params[FACTOR_MOD_PARAM].getValue();
		float resModParam = params[RES_MOD_PARAM].getValue();
		float feedModParam = params[FEED_MOD_PARAM].getValue();
		float driveModParam = params[DRIVE_MOD_PARAM].getValue();
		float chaosModParam = params[CHAOS_MOD_PARAM].getValue();

		for(int i=0;i<channels;i++){
			//Process Mods
			float modPositive = inputs[POS_INPUT].getVoltage(i);
			float modNegative;
			if(params[MOD_SWITCH].getValue()>0.5f){
				modNegative = inputs[NEG_INPUT].getNormalVoltage(modPositive,i);
			}else{
				modNegative = -inputs[NEG_INPUT].getNormalVoltage(modPositive,i);
			}

			float freqMod = 
				freqModParam * modPositive * float(freqModParam>0.f) +
				freqModParam * modNegative * float(freqModParam<0.f);
			float resMod = 
				resModParam * modPositive * float(resModParam>0.f) +
				resModParam * modNegative * float(resModParam<0.f);
			float feedMod = 
				feedModParam * modPositive * float(feedModParam>0.f) +
				feedModParam * modNegative * float(feedModParam<0.f);
			float driveMod = 
				driveModParam * modPositive * float(driveModParam>0.f) +
				driveModParam * modNegative * float(driveModParam<0.f);
			float chaosMod = 
				chaosModParam * modPositive * float(chaosModParam>0.f) +
				chaosModParam * modNegative * float(chaosModParam<0.f);
			
			//Process Audio
			float drive = dsp::exp2_taylor5(driveParam+driveMod/10*3);

			chaos = (lp[i])*clamp(chaosParam+chaosMod/10.f,0.f,1.f);

			float x = dsp::exp2_taylor5(freqParam+freqMod)*dsp::exp2_taylor5(chaos);

			freq = pow(x/magicNumber,2);
			freq = clamp(freq,minFreq,1.99f);

			res = clamp(resParam+resMod/10.f,0.f,1.f);
			if(res>0){
				res = pow(res,0.05f);
			}

			float in = inputs[AUDIO_INPUT].getVoltage(i)*drive;
			float feed = clamp(feedParam+feedMod/10.f,0.f,1.f);
			feed = (lp[i])*feed;

			lpvel[i] += (softClip((in+feed),7.f) - lp[i])*freq;
			lp[i] += lpvel[i]*sRatio;
			lpvel[i] *= res;
			lpvel[i] = softClip(lpvel[i],5.f);
			//Output
			outLP=softClip((lp[i]),7.f);
			outHP=softClip((in-lp[i]-feed),7.f);
			outputs[LP_OUTPUT].setVoltage(outLP,i);
			outputs[HP_OUTPUT].setVoltage(outHP,i);
		}
	}
};


struct LoamWidget : ModuleWidget {
	int theme = -1;

	LoamWidget(Loam* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Loam.svg")));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.54,14.00)), module, Loam::POS_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(23.94,14.00)), module, Loam::NEG_INPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54,30.00)), module, Loam::FACTOR_MOD_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54,43.00)), module, Loam::RES_MOD_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54,56.00)), module, Loam::FEED_MOD_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54,69.00)), module, Loam::DRIVE_MOD_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54,82.00)), module, Loam::CHAOS_MOD_PARAM));

		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(21.94,32.00)), module, Loam::FACTOR_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(21.94,49.00)), module, Loam::RES_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(21.94,66.00)), module, Loam::FEED_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(21.94,83.00)), module, Loam::DRIVE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(21.94,100.00)), module, Loam::CHAOS_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(15.24,14.00)), module, Loam::MOD_SWITCH));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.54, 102.00)), module, Loam::AUDIO_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(23.94, 114.5)), module, Loam::LP_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(6.54, 114.5)), module, Loam::HP_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Loam* module = getModule<Loam>();

		menu->addChild(new MenuSeparator);
	
		menu->addChild(createIndexSubmenuItem(
			"Panel Theme", 
			getPaletteNames(),	
			[=](){
				return module->getTheme();
			},
			[=](int newTheme) {
				module->setTheme(newTheme);
			}
		));

		menu->addChild(createMenuItem("Use Classic Theme","",[=](){
			module->setTheme(-2);
		}));
	}

	void step() override {
		ModuleWidget::step();
		Loam* module = getModule<Loam>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			if(theme>=0){
				setPanel(createTintPanel(
					"res/panels/LoamTintLayers.svg",
					getPalette(theme)
				));
			}else{
				setPanel(createPanel(asset::plugin(pluginInstance,"res/panels/Loam.svg")));
			}
		}
	}
};


Model* modelLoam = createModel<Loam, LoamWidget>("Loam");