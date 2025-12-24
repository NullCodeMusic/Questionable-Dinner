#include "plugin.hpp"
#include "mymath.hpp"


struct Yare : Module {
	int theme = -1;
	enum ParamId {
		RELEASE_PARAM,
		AMOUNT_PARAM,
		MODE_SWITCH,
		THRESH_PARAM,
		SHAPE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		VCA_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHT_LIGHT,
		LIGHTS_LEN=2
	};

	Yare() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RELEASE_PARAM, 0.001f, 0.2f, 0.05f, "Length", "ms", 0.0f,1000);
		configParam(AMOUNT_PARAM,0.f,1.f,0.5f,"Mix","%",0,100);
		configParam(THRESH_PARAM,-8.f,0.f,0.f,"Low Threshold","v",2.0f);
		configParam(SHAPE_PARAM,-1.f,1.f,0.f,"Shape");
		configSwitch(MODE_SWITCH,0.f,1.f,0.f,"Mode",{"Release Env","Sample & Hold"});

		configInput(VCA_INPUT, "VCA");
		configInput(AUDIO_INPUT, "Audio");
		configOutput(AUDIO_OUTPUT, "Audio");
		configBypass(AUDIO_INPUT,AUDIO_OUTPUT);
	}

	float env;
	float runningMax=0.f;
	float timer;
	dsp::RCFilter shapeFilter;

	void process(const ProcessArgs& args) override {
		shapeFilter.setCutoffFreq(20.f/args.sampleRate);
		float hpAmt = clamp(params[SHAPE_PARAM].getValue(),0.f,1.f);
		float lpAmt = clamp(-params[SHAPE_PARAM].getValue(),0.f,1.f);
		float dryAmt = 1-hpAmt-lpAmt;

		int mode = params[MODE_SWITCH].getValue();
		float audio = inputs[AUDIO_INPUT].getVoltage();
		float amount = params[AMOUNT_PARAM].getValue();
		float dec = params[RELEASE_PARAM].getValue();
		float vca = clamp(inputs[VCA_INPUT].getNormalVoltage(10.f)/10.f,0.f,1.f);
		float threshold = dsp::exp2_taylor5(params[THRESH_PARAM].getValue());
		if(mode == 0){
			hpAmt = 1-hpAmt;
			lpAmt = 1-lpAmt;
			getParamQuantity(RELEASE_PARAM)->displayMultiplier = 1000.f;
			env = std::max(abs(audio),exp2Decay(env,dec,args.sampleRate));
		}else{
			getParamQuantity(RELEASE_PARAM)->displayMultiplier = 1000.f*args.sampleTime*20000;
			timer+=args.sampleTime;
			runningMax = std::max(abs(audio),runningMax);
			if(timer>=dec*args.sampleTime*20000){
				env = runningMax;
				runningMax = 0.f;
				timer = 0.f;
			}
			env = std::max(abs(audio),env);
		}
		env = clamp(env, threshold, 10000.f);
		shapeFilter.process(env);
		float divisor = shapeFilter.highpass()*hpAmt+shapeFilter.lowpass()*lpAmt+env*dryAmt;
		float scaledAudio = audio/divisor;
		if(std::isnan(scaledAudio)){
			scaledAudio=0;
		}

		lights[LIGHT_LIGHT].setBrightness((5.f-env)/5.f*amount*float(env>threshold));
		lights[LIGHT_LIGHT+1].setBrightness((env-5.f)/5.f*amount*float(env>5.f));
		outputs[AUDIO_OUTPUT].setVoltage(softClip(scaledAudio*amount*5,10.f,0.75f)*vca+audio*(1-amount)*vca);
	}
};


struct YareWidget : ModuleWidget {
	int theme = -1;

	YareWidget(Yare* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Yare.svg",
			getPalette(PAL_PEACHBERRY)
		));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 27)), module, Yare::RELEASE_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 66)), module, Yare::AMOUNT_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 53)), module, Yare::THRESH_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 40)), module, Yare::SHAPE_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(7.62, 16)), module, Yare::MODE_SWITCH));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 87.5)), module, Yare::AUDIO_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 101.0)), module, Yare::VCA_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.62, 114.5)), module, Yare::AUDIO_OUTPUT));

		addChild(createLightCentered<SmallSimpleLight<GreenRedLight>>(mm2px(Vec(7.62, 78)), module, Yare::LIGHT_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Yare* module = getModule<Yare>();

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
		Yare* module = getModule<Yare>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Yare.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelYare = createModel<Yare, YareWidget>("Yare");