//Module Slug FourSpoons - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"
#include "mydsp.hpp"

template <typename T>
inline T clipFunction(T v, T k){
	T vr = abs(v);
	T vs = sgn(v);
	T a = clamp(vr-k,0.f,999.f)*vs/(1-k);
	T b = clamp(vr,-999.f,k)*vs;
	
	return (a/(1+abs(a)))*(1-k)+b;
}

struct FourSpoons : Module {
	int theme = -1;
	
	enum ParamId {
		TRIG_BUTTON,
		PITCH_PARAM,
		TENSION_PARAM,
		DECAY_PARAM = 6,
		EVIL_PARAM = 10,
		HALO_PARAM,
		LIGHTS_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_IN,
		IMPULSE_IN,
		VOCT_IN,
		EXPR_IN,
		EVIL_IN,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=4
	};
	
	float defaultTunings[4] = {0,1,1.58496250072, 2};

	FourSpoons() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(TRIG_BUTTON,"Manual Trigger");
		for(int i = 0;i<4;i++){
			configParam(TENSION_PARAM + i,-3,3,defaultTunings[i],"Tune "+std::to_string(i+1),"x",2);
			configParam(DECAY_PARAM + i,-6,6,0,"Decay "+std::to_string(i+1));
		}
		configParam(PITCH_PARAM,-2,6,1.9998,"Pitch","hz",2.0,65.4145);
		configParam(EVIL_PARAM,0,1.1,0,"Vile");
		configParam(HALO_PARAM,-1,1,0,"Feedback");

		configInput(IMPULSE_IN,"Impulse");
		configInput(TRIG_IN, "Trigger");
		configInput(VOCT_IN, "V/Oct");
		configInput(EXPR_IN, "Expression");

		configBypass(IMPULSE_IN, AUDIO_OUT);
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

	Resonator<simd::float_4> reso[16];
	float lastTrig[16];
	DelayLineDeque<15000> delay[16];
	dsp::RCFilter lp[32];
	dsp::TPeakFilter<float> env[20];
	
	void process(const ProcessArgs& args) override {
		processBase(args);
	}

    inline void processBase(const ProcessArgs& args) {
		//DECLARE VARS THAT DON'T NEED TO BE DECLARED IN LOOP
		simd::float_4 pitchmod = {1,1,1,1};
		simd::float_4 decay;
		simd::float_4 tension;
		simd::float_4 pitch;
		
		//POLYPHONY
		int channels = std::max({uint8_t(1),inputs[VOCT_IN].channels,inputs[TRIG_IN].channels,inputs[IMPULSE_IN].channels});
		outputs[AUDIO_OUT].setChannels(channels);

		//EXPRESSION
		float decayMult[16] = {1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1};
		float haloMult[16] = {1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1};
		switch(inputs[EXPR_IN].getChannels()){
			case 0:
				
				break;
			case 1:
				for(int i=0;i<channels;i++){
					decayMult[i]=clamp(1+inputs[EXPR_IN].getVoltage()/10,0.01f,2.f);
					haloMult[i] = clamp(inputs[EXPR_IN].getVoltage()/10,0.f,1.f);
				}
				break;
			default:
				for(int i=0;i<channels;i++){
					decayMult[i]=1+inputs[EXPR_IN].getVoltage(i)/10;
					haloMult[i] = clamp(inputs[EXPR_IN].getVoltage(i)/10,0.f,1.f);
				}
				break;
		}
		float evilMult[16] = {1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1};
		switch(inputs[EVIL_IN].getChannels()){
			case 0:
				
				break;
			case 1:
				for(int i=0;i<channels;i++){
					evilMult[i] = clamp(inputs[EVIL_IN].getVoltage()/10,0.f,1.f);
				}
				break;
			default:
				for(int i=0;i<channels;i++){
					evilMult[i] = clamp(inputs[EVIL_IN].getVoltage(i)/10,0.f,1.f);
				}
				break;
		}


		//MAIN LOOP
		for(int c = 0;c < channels;c++){
			env[c].setTau(0.5);

			float rawFreq = dsp::exp2_taylor5(params[PITCH_PARAM].getValue()+inputs[VOCT_IN].getVoltage(c)-1)*65.4145*239.7/261.62;
			float delayTime = 1/rawFreq;
			float impulse = 0;
			impulse += delay[c].readLerp(clamp(delayTime*args.sampleRate-1,1.f,14998.f))/4*params[HALO_PARAM].getValue()*haloMult[c];

			if(inputs[TRIG_IN].getVoltage(c)+params[TRIG_BUTTON].getValue()>lastTrig[c]){
				reso[c].reset();
				reso[c].state = {2.5,2.5,2.5,2.5};
			}
			lastTrig[c] = inputs[TRIG_IN].getVoltage(c)+params[TRIG_BUTTON].getValue();
			
			if(params[EVIL_PARAM].getValue()>0){
			pitchmod = {reso[c].state[3],reso[c].state[0],reso[c].state[1],reso[c].state[2]};
			pitchmod = abs(pitchmod)*simd::float_4(params[EVIL_PARAM].getValue()*evilMult[c]/(params[PITCH_PARAM].getValue()+3)*2);
			pitchmod = {dsp::exp2_taylor5(pitchmod[0]),dsp::exp2_taylor5(pitchmod[1]),dsp::exp2_taylor5(pitchmod[2]),dsp::exp2_taylor5(pitchmod[3])};
			}

			pitch = simd::float_4(dsp::exp2_taylor5(params[PITCH_PARAM].getValue()+inputs[VOCT_IN].getVoltage(c)-1))*pitchmod;
			
			for(int i = 0;i<4;i++){
				decay[i] = dsp::exp2_taylor5(params[DECAY_PARAM+i].getValue());
				tension[i] = dsp::exp2_taylor5(params[TENSION_PARAM+i].getValue()*2);
			}
			
			reso[c].setParams(
				(decay)/(tension)/(pitch)*simd::float_4(decayMult[c]),
				(tension),
				(pitch)
			);

			impulse += inputs[IMPULSE_IN].getVoltage(c)/4.f;
			reso[c].excite(simd::float_4(impulse));
			for(int repeat=0;repeat<2;repeat++){
				reso[c].process(args.sampleTime/2.f);
				reso[c].state = clipFunction(reso[c].state/simd::float_4(4.f),simd::float_4(0.75f))*simd::float_4(4.f);
			}
			//reso[c].state = clipFunction(reso[c].state/simd::float_4(4.f),simd::float_4(0.75f))*simd::float_4(4.f);
			//reso[c].state = softClip(reso[c].state,simd::float_4(4.f),simd::float_4(0.75f));
			//reso.state = 0;

			float out;
			out = reso[c].state[0]+reso[c].state[1]+reso[c].state[2]+reso[c].state[3];
			float envscale = fmax(env[c].process(args.sampleTime,fabs(out)),0.0001f);
			out = out/envscale;
			envscale = clipFunction(envscale/6.f,5.f/6.f)*6.f;
			out *= envscale;
			lp[16+c].setCutoffFreq(8000/args.sampleRate);
			lp[16+c].process(out);
			outputs[AUDIO_OUT].setVoltage(lp[16+c].lowpass(),c);
			lp[c].setCutoffFreq(rawFreq*args.sampleTime);
			lp[c].process(out);
			delay[c].write(out*2);
		}

		//LIGHTS
		for(int i = 0; i<4; i++){
			env[16+i].setTau(0.1);
			float l = env[16+i].process(args.sampleTime,reso[0].state[i]);
			lights[i].setBrightness(l/4.f);
		}
	}
};


struct FourSpoonsWidget : ModuleWidget {
	int theme = -1;

	FourSpoonsWidget(FourSpoons* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/FourSpoons.svg",
			getPalette(PAL_SKY)
		));
        //Add widgets here: 
		for(int i = 0;i<4;i++){
			addParam(createLightParamCentered<VCVLightSlider<WhiteLight>>(mm2px(Vec(7.62+i*10.16, 63.0)), module, FourSpoons::DECAY_PARAM+i,i));
			addParam(createParamCentered<QKnob6mm>(mm2px(Vec(7.62+i*10.16, 86.5)), module, FourSpoons::TENSION_PARAM+i));
		}

        addParam(createParamCentered<VCVButton>(mm2px(Vec(22.860, 102.0)), module, FourSpoons::TRIG_BUTTON));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(22.860, 30.0)), module, FourSpoons::PITCH_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(36.830, 41.5)), module, FourSpoons::EVIL_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(8.890, 41.5)), module, FourSpoons::HALO_PARAM));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.62+3*10.16,114.5)),module, FourSpoons::AUDIO_OUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62+10.16,114.5)),module, FourSpoons::IMPULSE_IN));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62,114.5)),module, FourSpoons::TRIG_IN));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62+2*10.16,114.5)),module, FourSpoons::VOCT_IN));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62,102)),module, FourSpoons::EXPR_IN));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62+3*10.16,102)),module, FourSpoons::EVIL_IN));
	}

	void appendContextMenu(Menu* menu) override {
		FourSpoons* module = getModule<FourSpoons>();
		
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
		FourSpoons* module = getModule<FourSpoons>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/FourSpoons.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelFourSpoons = createModel<FourSpoons, FourSpoonsWidget>("FourSpoons");