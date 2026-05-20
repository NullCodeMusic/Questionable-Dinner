//Module Slug Weather - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"

struct Weather : Module {
	int theme = -1;
	
	enum ParamId {
		FREQ_PARAM,
		FREQTYPE_SWITCH,
		CHAOS_PARAM,
		LENGTH_PARAM,
		POLY_PARAM,
		SPREAD_PARAM,
		MONO_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		FREQ_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TAP_OUTPUT,
		SLOPE_OUTPUT,
		CLOCK_OUTPUT,
		PULSAR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Weather() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM,-5.f,5.f,0.f,"Frequency","hz",2.f,dsp::FREQ_C4);
		configParam(CHAOS_PARAM,0.f,2.f,0.5f,"Randomness","%",0.f,100.f);
		configParam(LENGTH_PARAM,0,2,1,"Length","x");
		configParam(SPREAD_PARAM,0,2,0,"Spread","x");
		configSwitch(FREQTYPE_SWITCH,0,1,1,"Freq Range",{"Low","High"});
		configParam(POLY_PARAM,1,16,1,"Unison Voices")->snapEnabled = true;
		configSwitch(MONO_SWITCH,0,1,0,"Outputs",{"Mono","Poly"});

		configOutput(CLOCK_OUTPUT,"Clock");
		configOutput(TAP_OUTPUT,"Rain");
		configOutput(PULSAR_OUTPUT,"Bubble");
		configOutput(SLOPE_OUTPUT,"Cloud");

		configInput(FREQ_INPUT,"V/Oct");
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

	dsp::PulseGenerator clock[16];
	float offset[16] = {};
	int timer[16] = {};
	float slopeLast[16] = {};
	float slopeNext[16] = {};
	dsp::RCFilter slopeHP[16];
	float phase[16] = {};
	float tapEnv[16] = {};
	float freqMult;

	inline void processOcean(float freq, float length, float lfreq, float chaos, int voices,const ProcessArgs& args){
		if(params[MONO_SWITCH].getValue()){

			for(int i=0; i<OUTPUTS_LEN; i++){
				outputs[i].setChannels(voices);
			}

			for(int i=0; i<voices; i++){
				//Spread
				float freqSpread = 1+i*params[SPREAD_PARAM].getValue();

				//Push phase forward
				phase[i] += args.sampleTime*freq*freqSpread;
				float phaseMax = 1.f+offset[i];
				if(phase[i]>=phaseMax){
					//Reset Phase
					phase[i]-=phaseMax;
					//Randomize Next Offset
					offset[i] = random::uniform()*2-1;
					offset[i] *= offset[i];
					offset[i] *= params[CHAOS_PARAM].getValue();
					//Send Clock Pulse
					clock[i].trigger(length);
					//Set Slope Positions
					slopeLast[i] = slopeNext[i];
					slopeNext[i] = 1/(1+offset[i]);
					//Add to Tap Envelope
					tapEnv[i] += 1;
				}
				float progress = phase[i]/phaseMax;

				//Process Clock
				clock[i].process(args.sampleTime);

				//Process Slope
				float slopeOut = cerp(slopeLast[i],slopeNext[i],progress);
				slopeHP[i].setCutoffFreq(10*args.sampleTime);
				slopeHP[i].process(slopeOut);

				//Process Pulsar
				float wave = sin_2pi_9(phase[i]*phase[i]*lfreq*4);
				float window = sin_2pi_9(progress/2);

				//ProcessTap
				tapEnv[i] = exp2Decay(tapEnv[i],length,args.sampleRate);

				//Send Outputs
				outputs[SLOPE_OUTPUT].setVoltage(softClip(slopeHP[i].highpass(),1.f)*25/sqrt(chaos),i);
				outputs[CLOCK_OUTPUT].setVoltage(clock[i].isHigh()*10.f,i);
				outputs[PULSAR_OUTPUT].setVoltage(wave*window*5.f,i);
				outputs[TAP_OUTPUT].setVoltage(softClip(tapEnv[i],2.f)*5,i);
			}
		
		}else{

			for(int i=0; i<OUTPUTS_LEN; i++){
				outputs[i].setChannels(1);
			}

			float slopeSum = 0;
			float clockSum = 0;
			float pulsarSum = 0;
			float tapSum = 0;
			for(int i=0; i<voices; i++){
				//Spread
				float freqSpread = 1+i*params[SPREAD_PARAM].getValue();

				//Push phase forward
				phase[i] += args.sampleTime*freq*freqSpread;
				float phaseMax = 1.f+offset[i];
				if(phase[i]>=phaseMax){
					//Reset Phase
					phase[i]-=phaseMax;
					//Randomize Next Offset
					offset[i] = random::uniform()*2-1;
					offset[i] *= offset[i];
					offset[i] *= params[CHAOS_PARAM].getValue();
					//Send Clock Pulse
					clock[i].trigger(length);
					//Set Slope Positions
					slopeLast[i] = slopeNext[i];
					slopeNext[i] = 1/(1+offset[i]);
					//Add to Tap Envelope
					tapEnv[i] += 1;
				}
				float progress = phase[i]/phaseMax;

				//Process Clock
				clock[i].process(args.sampleTime);

				//Process Slope
				float slopeOut = cerp(slopeLast[i],slopeNext[i],progress);
				slopeHP[i].setCutoffFreq(freq*args.sampleTime/16);
				slopeHP[i].process(slopeOut);

				//Process Pulsar
				float wave = sin_2pi_9(phase[i]*phase[i]*lfreq*4);
				float window = sin_2pi_9(progress/2);

				//ProcessTap
				tapEnv[i] = exp2Decay(tapEnv[i],length,args.sampleRate);

				//Send Outputs
				slopeSum += softClip(slopeHP[i].highpass(),1.f)*25/sqrt(chaos);
				clockSum += clock[i].isHigh()*10.f;
				pulsarSum += wave*window*5.f;
				tapSum += softClip(tapEnv[i],2.f)*5;
			}

			outputs[SLOPE_OUTPUT].setVoltage(slopeSum/float(voices));
			outputs[CLOCK_OUTPUT].setVoltage(clockSum/float(voices));
			outputs[PULSAR_OUTPUT].setVoltage(pulsarSum/float(voices));
			outputs[TAP_OUTPUT].setVoltage(tapSum/float(voices));
		}
	}

    void process(const ProcessArgs& args) override {

		//Params
		float chaos = params[CHAOS_PARAM].getValue()+0.00001;
		float freq = dsp::exp2_taylor5(params[FREQ_PARAM].getValue());

		//Unison
		int voices = params[POLY_PARAM].getValue();

		//Freq Mode & Input
		if(params[FREQTYPE_SWITCH].getValue()){
			freqMult = dsp::FREQ_C4;
		}else{
			freqMult = 10;
		}
		getParamQuantity(FREQ_PARAM)->displayMultiplier = freqMult;
		freq*=freqMult;

		float length = std::max(params[LENGTH_PARAM].getValue()/freq,args.sampleTime);
		float lfreq = 2-params[LENGTH_PARAM].getValue();

		//
		// Main Process
		//

		processOcean(freq,length,lfreq,chaos,voices,args);
	}
};


struct WeatherWidget : ModuleWidget {
	int theme = -1;

	WeatherWidget(Weather* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Weather.svg",
			getPalette(PAL_PEACHBERRY)
		));
        //Add widgets here: 
        addParam(createParamCentered<QKnob18mm>(mm2px(Vec(12.7, 36.0)), module, Weather::FREQ_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(8.89, 55.0)), module, Weather::CHAOS_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(26.67, 55.0)), module, Weather::LENGTH_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(26.67, 72.5)), module, Weather::POLY_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(8.89, 72.5)), module, Weather::SPREAD_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(28.750, 36)), module, Weather::FREQTYPE_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(17.78, 100.5)), module, Weather::MONO_SWITCH));
		
		addOutput(createOutputCentered<QPort>(mm2px(Vec(6.35, 114.5)), module, Weather::SLOPE_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(29.210, 100.5)), module, Weather::CLOCK_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(17.78, 114.5)), module, Weather::PULSAR_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(29.210, 114.5)), module, Weather::TAP_OUTPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(6.35, 100.5)), module, Weather::FREQ_INPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Weather* module = getModule<Weather>();
		
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
		Weather* module = getModule<Weather>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Weather.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelWeather = createModel<Weather, WeatherWidget>("Weather");