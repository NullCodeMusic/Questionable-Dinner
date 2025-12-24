#include "plugin.hpp"
#include "mymath.hpp"
#include "lookups.hpp"

struct Moxie : Module {
	int theme = -1;

	enum ParamId {
		BIAS_PARAM,
		BITE_PARAM,
		DRIVE_PARAM,
		FADE_PARAM,
		FM_PARAM,
		MODE_SWITCH,
		ALIVE_SWITCH,
		FORM_BUTTON,
		LIN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MAIN_INPUT,
		FM_INPUT,
		VOCT_INPUT,
		SYNC_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		POSITIVE_OUTPUT,
		NEGATIVE_OUTPUT,
		MAIN_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FORM_LIGHT,
		LIGHTS_LEN = 3
	};

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "clip", json_boolean(clip));
		json_object_set_new(rootJ, "dc", json_boolean(dcRemove));
		json_object_set_new(rootJ, "model", json_integer(form));
		json_object_set_new(rootJ, "theme", json_integer(theme));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* clipJ = json_object_get(rootJ, "clip");
		if (clipJ){
			clip=json_boolean_value(clipJ);
		}
		json_t* dcJ = json_object_get(rootJ, "dc");
		if (dcJ){
			dcRemove=json_boolean_value(dcJ);
		}
		json_t* formJ = json_object_get(rootJ, "model");
		if (formJ){
			form=json_integer_value(formJ);
		}
		json_t* themeJ = json_object_get(rootJ, "theme");
		if (themeJ){
			theme = json_integer_value(themeJ);
		}
	}

	Moxie() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BIAS_PARAM, -10.f, 10.f, 0.f, "Bias","",0.0f,0.2f);
		configParam(BITE_PARAM, 0.f, 14.f, 12.f, "Bite");
		configParam(FADE_PARAM, -1.f,1.f,0.f,"Morph");
		configParam(DRIVE_PARAM, -5.f, 5.f, 0.f, "Freq","",2.0f,261.63);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM Amount","%",0.0f,100.f);
		configParam(LIN_PARAM, 0.f, 0.99999f, 0.f, "Input Linearity");
		configButton(FORM_BUTTON, "Change Model");

		configSwitch(MODE_SWITCH, 0.f,1.f,0.f, "Mode", {"Continuous","Slope"});
		configSwitch(ALIVE_SWITCH, 0.f,1.f,0.f, "State", {"Alive","Dead"});

		configInput(MAIN_INPUT, "Voltage");
		configInput(FM_INPUT, "FM");
		configInput(VOCT_INPUT, "v/Oct");
		configInput(SYNC_INPUT, "Sync/Reset");
		configOutput(POSITIVE_OUTPUT, "Up");
		configOutput(NEGATIVE_OUTPUT, "Down");
		configOutput(MAIN_OUTPUT, "Main");

		configBypass(MAIN_INPUT,MAIN_OUTPUT);
	}
	float posSum[16]={0};
	float posEnv[16]={0};
	float negSum[16]={0};
	float negEnv[16]={0};
	float lastAudio[16]={0};
	float lastSync[16]={0};
	float flipflop[16] = {1};

	bool clip=true;
	bool mode=true;
	bool reset = true;
	bool dcRemove=true;
	bool flipflopEnabled= false;

	int oversampling = 8;
	int form = 0;

	float formButton = 0;

	dsp::RCFilter dcBlocker[16];
	dsp::PulseGenerator trig[16];

	void onAdd(const AddEvent& e) override{
		float sr = APP->engine->getSampleRate();

		for(int i = 0; i<16; i++){
			dcBlocker[i].setCutoffFreq(10/sr);
		}
	}

	void process(const ProcessArgs& args) override {
		const float TUNING = 1/softClip(2,2,params[LIN_PARAM].getValue());

		if(params[FORM_BUTTON].getValue()>formButton){
			form = (form+1)%5;
		}
		formButton = params[FORM_BUTTON].getValue();

		switch (form)
		{
		case 0:
			lights[FORM_LIGHT].setBrightness(0.f);
			lights[FORM_LIGHT+1].setBrightness(0.f);
			lights[FORM_LIGHT+2].setBrightness(0.f);
			moxie1(args,TUNING);
			break;
		
		case 1:
			lights[FORM_LIGHT].setBrightness(0.f);
			lights[FORM_LIGHT+1].setBrightness(0.f);
			lights[FORM_LIGHT+2].setBrightness(1.f);
			moxie2(args,form-1,TUNING);
			break;
		case 2:
			lights[FORM_LIGHT].setBrightness(0.f);
			lights[FORM_LIGHT+1].setBrightness(1.f);
			lights[FORM_LIGHT+2].setBrightness(0.f);
			moxie2(args,form-1,TUNING);
			break;
		case 3:
			lights[FORM_LIGHT].setBrightness(0.8f);
			lights[FORM_LIGHT+1].setBrightness(0.8f);
			lights[FORM_LIGHT+2].setBrightness(0.f);
			moxie2(args,form-1,TUNING);
			break;

		case 4:
			lights[FORM_LIGHT].setBrightness(1.f);
			lights[FORM_LIGHT+1].setBrightness(0.f);
			lights[FORM_LIGHT+2].setBrightness(0.f);
			moxieTrig(args,TUNING);
			break;
		
		default:
			moxie1(args,TUNING);
			break;
		}
	}

	void moxieTrig(const ProcessArgs& args, const float TUNING){

		mode = params[MODE_SWITCH].getValue();
		flipflopEnabled = params[ALIVE_SWITCH].getValue();

		float bias = params[BIAS_PARAM].getValue();

		float morph = params[FADE_PARAM].getValue()*2.5;

		int channels = std::max({
			inputs[MAIN_INPUT].getChannels(),
			inputs[FM_INPUT].getChannels(),
			inputs[VOCT_INPUT].getChannels(),
			inputs[SYNC_INPUT].getChannels(),
			1
		});

		for(int i = 0; i<OUTPUTS_LEN; i++){
			outputs[i].setChannels(channels);
		}

	//THE LOOP SHOULD START HERE
		for(int v = 0;v<channels;v++){

			//FREQUENCY/SLOPE MULT
			float freak = dsp::exp2_taylor5(params[DRIVE_PARAM].getValue()+
				inputs[FM_INPUT].getVoltage(v)*params[FM_PARAM].getValue()+
				inputs[VOCT_INPUT].getVoltage(v)
				)*20;
			freak = clamp(freak,0.f,1000.f);

			//FLIP FLOP
			float posMult = 1.f;
			float negMult = 1.f;
			if(flipflopEnabled){
				posMult=flipflop[v];
				negMult=1.f-flipflop[v];
			}

			//SYNC
			float sync = inputs[SYNC_INPUT].getVoltage(v);
			if(reset){
				posSum[v]=0;
				negSum[v]=0;
				lastAudio[v]=0;
			}
			reset = (sync>0)&&(lastSync[v]<=0);
			lastSync[v] = sync;

			//MAIN ALGO
			float audio = softClip(inputs[MAIN_INPUT].getVoltage(v)+bias,10.f,params[LIN_PARAM].getValue());
			if(mode){//SLOPE MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 4.f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "x";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Slope Multiplier";//Move this

				float slope = audio - lastAudio[v];
				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(slope > 0){
						posSum[v]+=slope*freak/oversampling/5*posMult;
					}else{
						negSum[v]-=slope*freak/oversampling/5*negMult;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posEnv[v]= 0;
						posSum[v]-=5;
						trig[v].trigger();
					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negEnv[v]= 0;
						negSum[v]-=5;
						trig[v].trigger();
					}
					posEnv[v]=posSum[v];
					negEnv[v]=negSum[v];
				}

			}else{//CONTINUOUS MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 20.f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "hz";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Frequency";//Move this

				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(audio>0){
						posSum[v]+=audio*freak*args.sampleTime/oversampling*posMult*TUNING;
					}else{
						negSum[v]-=audio*freak*args.sampleTime/oversampling*negMult*TUNING;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posEnv[v]= 0;
						posSum[v]-=5;
						trig[v].trigger();

					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negSum[v]-=5;
						negEnv[v]= 0;
						trig[v].trigger();
					}
					posEnv[v]=posSum[v];
					negEnv[v]=negSum[v];
				}

			}
			outputs[POSITIVE_OUTPUT].setVoltage((posEnv[v]<2.5f+morph)*10,v);
			outputs[NEGATIVE_OUTPUT].setVoltage((negEnv[v]<2.5f-morph)*10,v);
			trig[v].process(args.sampleTime);
			outputs[MAIN_OUTPUT].setVoltage(trig[v].isHigh()*10,v);
		}
	}
	
	void moxie2(const ProcessArgs& args, int form, const float TUNING){

		mode = params[MODE_SWITCH].getValue();
		flipflopEnabled = params[ALIVE_SWITCH].getValue();
		float bias = params[BIAS_PARAM].getValue();

		float morphpos = clamp(1.f+params[FADE_PARAM].getValue(),0.f,1.f);
		float morphneg = clamp(1.f-params[FADE_PARAM].getValue(),0.f,1.f);

		float shape = -(params[BITE_PARAM].getValue())*0.57142857142;

		int channels = std::max({
			inputs[MAIN_INPUT].getChannels(),
			inputs[FM_INPUT].getChannels(),
			inputs[VOCT_INPUT].getChannels(),
			inputs[SYNC_INPUT].getChannels(),
			1
		});

		for(int i = 0; i<OUTPUTS_LEN; i++){
			outputs[i].setChannels(channels);
		}

	//THE LOOP SHOULD START HERE
		for(int v = 0;v<channels;v++){

			//FREQUENCY/SLOPE MULT
			float freak = dsp::exp2_taylor5(params[DRIVE_PARAM].getValue()+
				inputs[FM_INPUT].getVoltage(v)*params[FM_PARAM].getValue()+
				inputs[VOCT_INPUT].getVoltage(v)
				)*261.63;
			freak = clamp(freak,0.f,32000.f);
			//float dec = 1/freak/8;
			//FLIP FLOP
			float posMult = 1.f;
			float negMult = 1.f;
			if(flipflopEnabled){
				posMult=flipflop[v];
				negMult=1.f-flipflop[v];
			}

			//SYNC
			float sync = inputs[SYNC_INPUT].getVoltage(v);
			if(reset){
				posSum[v]=0;
				negSum[v]=0;
				lastAudio[v]=0;
			}
			reset = (sync>0)&&(lastSync[v]<=0);
			lastSync[v] = sync;

			//MAIN ALGO
			float audio = softClip(inputs[MAIN_INPUT].getVoltage(v)+bias,10.f,params[LIN_PARAM].getValue());
			if(mode){//SLOPE MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 4.f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "x";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Slope Multiplier";//Move this

				float slope = audio - lastAudio[v];
				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(slope > 0){
						posSum[v]+=slope*freak/oversampling/65.4075f*posMult;
					}else{
						negSum[v]-=slope*freak/oversampling/65.4075f*negMult;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posSum[v]-=5;
					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negSum[v]-=5;
					}
					//posEnv[v]=std::max(posSum[v],exp2Decay(posEnv[v],dec,args.sampleRate*oversampling));
					//negEnv[v]=std::max(negSum[v],exp2Decay(negEnv[v],dec,args.sampleRate*oversampling));
				}

			}else{//CONTINUOUS MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 261.63f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "hz";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Frequency";//Move this

				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(audio>0){
						posSum[v]+=audio*freak*args.sampleTime/oversampling*posMult*TUNING;
					}else{
						negSum[v]-=audio*freak*args.sampleTime/oversampling*negMult*TUNING;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posEnv[v]=5;
						posSum[v]-=5;
					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negSum[v]-=5;
						negEnv[v]=5;
					}
					//posEnv[v]=std::max(posSum[v],exp2Decay(posEnv[v],dec,args.sampleRate*oversampling));
					//negEnv[v]=std::max(negSum[v],exp2Decay(negEnv[v],dec,args.sampleRate*oversampling));
				}

			}

			switch (form)
				{
				case 0:
					posEnv[v] = mxyRound(posSum[v]/5,shape)*5;
					negEnv[v] = mxyRound(negSum[v]/5,shape)*5;
					break;
				case 1:
					posEnv[v] = mxySpike(posSum[v]/5,shape)*5;
					negEnv[v] = mxySpike(negSum[v]/5,shape)*5;
					break;
				case 2:
					posEnv[v] = mxyRound(posSum[v]/5,shape)*5;
					negEnv[v] = mxySpike(negSum[v]/5,shape)*5;
					break;
				default:
					break;
			}

			float mainOut = posEnv[v]*morphpos-negEnv[v]*morphneg;
			dcBlocker[v].process(mainOut);
			mainOut = dcBlocker[v].highpass()*float(dcRemove)+mainOut*float(!dcRemove);
			outputs[POSITIVE_OUTPUT].setVoltage(clamp(posEnv[v],-24.f,24.f),v);
			outputs[NEGATIVE_OUTPUT].setVoltage(clamp(negEnv[v],-24.f,24.f),v);
			if(clip){
				outputs[MAIN_OUTPUT].setVoltage(softClip(mainOut,10.f,0.5f),v);
			}else{
				outputs[MAIN_OUTPUT].setVoltage(mainOut,v);
			}
		}
	}

	void moxie1(const ProcessArgs& args, const float TUNING){

		mode = params[MODE_SWITCH].getValue();
		flipflopEnabled = params[ALIVE_SWITCH].getValue();

		float dec = dsp::exp2_taylor5(-params[BITE_PARAM].getValue());
		float bias = params[BIAS_PARAM].getValue();

		float morphpos = clamp(1.f+params[FADE_PARAM].getValue(),0.f,1.f);
		float morphneg = clamp(1.f-params[FADE_PARAM].getValue(),0.f,1.f);

		int channels = std::max({
			inputs[MAIN_INPUT].getChannels(),
			inputs[FM_INPUT].getChannels(),
			inputs[VOCT_INPUT].getChannels(),
			inputs[SYNC_INPUT].getChannels(),
			1
		});

		for(int i = 0; i<OUTPUTS_LEN; i++){
			outputs[i].setChannels(channels);
		}

	//THE LOOP SHOULD START HERE
		for(int v = 0;v<channels;v++){

			//FREQUENCY/SLOPE MULT
			float freak = dsp::exp2_taylor5(params[DRIVE_PARAM].getValue()+
				inputs[FM_INPUT].getVoltage(v)*params[FM_PARAM].getValue()+
				inputs[VOCT_INPUT].getVoltage(v)
				)*261.63;
			freak = clamp(freak,0.f,32000.f);

			//FLIP FLOP
			float posMult = 1.f;
			float negMult = 1.f;
			if(flipflopEnabled){
				posMult=flipflop[v];
				negMult=1.f-flipflop[v];
			}

			//SYNC
			float sync = inputs[SYNC_INPUT].getVoltage(v);
			if(reset){
				posSum[v]=0;
				negSum[v]=0;
				lastAudio[v]=0;
			}
			reset = (sync>0)&&(lastSync[v]<=0);
			lastSync[v] = sync;

			//MAIN ALGO
			float audio = softClip(inputs[MAIN_INPUT].getVoltage(v)+bias,10.f,params[LIN_PARAM].getValue());
			if(mode){//SLOPE MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 4.f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "x";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Slope Multiplier";//Move this

				float slope = audio - lastAudio[v];
				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(slope > 0){
						posSum[v]+=slope*freak/oversampling/65.4075f*posMult;
					}else{
						negSum[v]-=slope*freak/oversampling/65.4075f*negMult;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posEnv[v]=5;
						posSum[v]-=5;
					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negSum[v]-=5;
						negEnv[v]=5;
					}
					posEnv[v]=std::max(posSum[v],exp2Decay(posEnv[v],dec,args.sampleRate*oversampling));
					negEnv[v]=std::max(negSum[v],exp2Decay(negEnv[v],dec,args.sampleRate*oversampling));
				}

			}else{//CONTINUOUS MODE
				getParamQuantity(DRIVE_PARAM)->displayMultiplier = 261.63f;//Move this
				getParamQuantity(DRIVE_PARAM)->unit = "hz";//Move this
				getParamQuantity(DRIVE_PARAM)->name = "Frequency";//Move this

				lastAudio[v] = audio;
				for(int i=0;i<oversampling;i++){
					if(audio>0){
						posSum[v]+=audio*freak*args.sampleTime/oversampling*posMult*TUNING;
					}else{
						negSum[v]-=audio*freak*args.sampleTime/oversampling*negMult*TUNING;
					}
					if(posSum[v]>5){
						flipflop[v]=0;
						posEnv[v]=5;
						posSum[v]-=5;
					}
					if(negSum[v]>5){
						flipflop[v]=1;
						negSum[v]-=5;
						negEnv[v]=5;
					}
					posEnv[v]=std::max(posSum[v],exp2Decay(posEnv[v],dec,args.sampleRate*oversampling));
					negEnv[v]=std::max(negSum[v],exp2Decay(negEnv[v],dec,args.sampleRate*oversampling));
				}

			}

			dcBlocker[v].process(posEnv[v]*morphpos-negEnv[v]*morphneg);
			float mainOut = dcBlocker[v].highpass()*float(dcRemove)+(posEnv[v]*morphpos-negEnv[v]*morphneg)*float(!dcRemove);
			outputs[POSITIVE_OUTPUT].setVoltage(clamp(posEnv[v],-24.f,24.f),v);
			outputs[NEGATIVE_OUTPUT].setVoltage(clamp(negEnv[v],-24.f,24.f),v);
			if(clip){
				outputs[MAIN_OUTPUT].setVoltage(softClip(mainOut,10.f,0.5f),v);
			}else{
				outputs[MAIN_OUTPUT].setVoltage(mainOut,v);
			}
		}
	}
};


struct MoxieWidget : ModuleWidget {
	int theme = -1;

	MoxieWidget(Moxie* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Moxie.svg",
			getPalette(PAL_TANGERINE)
		));

		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(13.97, 38.00)), module, Moxie::BIAS_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35.56, 50.0)), module, Moxie::BITE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35.56, 34.0)), module, Moxie::DRIVE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35.56, 66.0)), module, Moxie::FADE_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(27.94, 89.0)), module, Moxie::FM_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(17.582, 12.00)), module, Moxie::LIN_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(8.89, 56.0)), module, Moxie::MODE_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(19.05, 56.0)), module, Moxie::ALIVE_SWITCH));
		addParam(createParamCentered<TL1105>(mm2px(Vec(22.86, 76.0)), module, Moxie::FORM_BUTTON));

		addChild(createLightCentered<SmallSimpleLight<RedGreenBlueLight>>(mm2px(Vec(22.86, 69.0)), module, Moxie::FORM_LIGHT));

		addInput(createInputCentered<QPort>(mm2px(Vec(13.97, 68.5)), module, Moxie::MAIN_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 89.0)), module, Moxie::VOCT_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(17.78, 89.0)), module, Moxie::FM_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(38.10, 89.0)), module, Moxie::SYNC_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.62, 114.5)), module, Moxie::POSITIVE_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(17.78, 114.5)), module, Moxie::NEGATIVE_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(38.10, 114.5)), module, Moxie::MAIN_OUTPUT));
	}
	void appendContextMenu(Menu* menu) override {
		Moxie* module = getModule<Moxie>();

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Main output options"));
		menu->addChild(createBoolPtrMenuItem("Soft clip to ±10v","",&module->clip));
		menu->addChild(createBoolPtrMenuItem("Remove DC offset","",&module->dcRemove));
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
		Moxie* module = getModule<Moxie>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
			"res/panels/Moxie.svg",
			getPalette(theme)
			));
		}
	}
};


Model* modelMoxie = createModel<Moxie, MoxieWidget>("Moxie");