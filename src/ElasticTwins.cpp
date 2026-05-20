#include "plugin.hpp"
#include <stack>
#include <queue>
#include "mymath.hpp"

struct ElasticTwins : Module {

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

	//The Module
	enum ParamId {
		REV_BUTTON,
		REC_BUTTON,
		CLR_BUTTON,
		MODE1_SWITCH,
		MODE2_SWITCH,
		FEEDBACK_PARAM,
		TRANS_PARAM,
		BUFFER_PARAM,
		SWAP_BUTTON,
		CLK1_PARAM,
		CLK2_PARAM,
		CLK_REV_SWITCH,
		CLK_REC_SWITCH,
		CLK_CLR_SWITCH,
		CLK_SWAP_SWITCH,
		CLK_SYNC_SWITCH,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		REV_INPUT,
		REC_INPUT,
		CLR_INPUT,
		SWAP_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		BUF1_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		REC_LIGHT,
		REV_LIGHT,
		CLR_LIGHT,
		SWAP_LIGHT,
		CLK1_LIGHT,
		CLK2_LIGHT,
		LIGHTS_LEN
	};
	enum TransParamId {
		REC_TPARAM,
		REV_TPARAM,
		CLR_TPARAM,
		SWAP_TPARAM
	};

	ElasticTwins() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(AUDIO_INPUT, "Audio");
		configInput(REV_INPUT, "Select Override");
		configInput(REC_INPUT, "Record Override");
		configInput(CLR_INPUT, "Clear Override");
		configInput(SWAP_INPUT, "Swap Override");
		configOutput(BUF1_OUTPUT, "Audio");
		configButton(REV_BUTTON, "Select Current Buffer");
		configButton(REC_BUTTON, "Record To Buffer")->defaultValue = 1.f;
		configButton(CLR_BUTTON, "Clear Buffers");
		configButton(SWAP_BUTTON, "Swap Buffer Data");
		configSwitch(MODE1_SWITCH, 0, 1, 0, "Buffer 1 Mode", {"Reverse","Forward"});
		configSwitch(MODE2_SWITCH, 0, 1, 0, "Buffer 2 Mode", {"Reverse","Forward"});
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "Feedback");
		configParam(TRANS_PARAM, 0.00001f, 20.f, 3.f, "Smooth", "ms");

		//CLOCK STUFF
		configParam(CLK1_PARAM, -4, 12, 0, "Clock 1", "hz", 2.f);
		configParam(CLK2_PARAM, -4, 12, 0, "Clock 2", "hz", 2.f);
		configSwitch(CLK_CLR_SWITCH,-1,1,0,"Clear Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_REV_SWITCH,-1,1,0,"Select Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_REC_SWITCH,-1,1,0,"Record Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_SWAP_SWITCH,-1,1,1,"Swap Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_SYNC_SWITCH,0,1,9, "Sync");

		//BYPASS
		configBypass(AUDIO_INPUT,BUF1_OUTPUT);
	}

	//BUFFER VARS
	staque<float> buf1[16];
	staque<float> buf2[16];

	//TRIG AND PARAMS
	float lastClear[16];
	float lastSwap;

	//TIMERS
	float clock1progress;
	float clock2progress;
	float transitionProgress[16];
	bool queuedParams[4][16] = {};
	bool currentParams[4][16] = {};

	void process(const ProcessArgs& args) override {

	/*
	
		In this section, grab all param inputs and queue any param changes
	
	*/
	int voices = inputs[AUDIO_INPUT].getChannels();
	outputs[BUF1_OUTPUT].setChannels(voices);

		//UPDATE CLOCKS
		float clock1Rate = dsp::exp2_taylor5(params[CLK1_PARAM].getValue())*args.sampleTime;
		float clock2Rate = dsp::exp2_taylor5(params[CLK2_PARAM].getValue())*args.sampleTime;
		clock1progress += clock1Rate;
		clock2progress += clock2Rate;
		clock1progress-=floor(clock1progress);
		if(clock1progress>=1){
			if(params[CLK_SYNC_SWITCH].getValue()>0.1){
				clock2progress=0;
			}
		}
		clock2progress-=floor(clock2progress);
		bool clock1Gate = clock1progress<=0.5f;
		bool clock2Gate = clock2progress<=0.5f;

	for(int i = 0; i < voices; i++){

		//SEND VALUES FROM CLOCK
		float clrGate;
		switch (static_cast<int>(params[CLK_CLR_SWITCH].getValue()))
		{
		case -1:
			clrGate = clock2Gate;
			break;
		case 1:
			clrGate = clock1Gate;
			break;
		default:
			clrGate = 0.f;
			break;
		}

		float recGate;
		switch (static_cast<int>(params[CLK_REC_SWITCH].getValue()))
		{
		case -1:
			recGate = clock2Gate;
			break;
		case 1:
			recGate = clock1Gate;
			break;
		default:
			recGate = 0.f;
			break;
		}

		float swapGate;
		switch (static_cast<int>(params[CLK_SWAP_SWITCH].getValue()))
		{
		case -1:
			swapGate = clock2Gate;
			break;
		case 1:
			swapGate = clock1Gate;
			break;
		default:
			swapGate = 0.f;
			break;
		}

		float revGate;
		switch (static_cast<int>(params[CLK_REV_SWITCH].getValue()))
		{
		case -1:
			revGate = clock2Gate;
			break;
		case 1:
			revGate = clock1Gate;
			break;
		default:
			revGate = 0.f;
			break;
		}

		//BUFFER MODE SWITCH
		buf1[i].mode = params[MODE1_SWITCH].getValue();
		buf2[i].mode = params[MODE2_SWITCH].getValue();

		//FEEDBACK
		float fb = params[FEEDBACK_PARAM].getValue();

		//GATES
		queuedParams[REC_TPARAM][i] = !(inputs[REC_INPUT].getNormalVoltage(recGate,i)>0.1) != !(params[REC_BUTTON].getValue()>0.1);
		queuedParams[REV_TPARAM][i] = !(inputs[REV_INPUT].getNormalVoltage(revGate,i)>0.1) != !(params[REV_BUTTON].getValue()>0.1);

		//TRIGS
		if(inputs[CLR_INPUT].getNormalVoltage(clrGate,i)+params[CLR_BUTTON].getValue()>lastClear[i]){
			queuedParams[CLR_TPARAM][i] = true;
		}
		lastClear[i] = inputs[CLR_INPUT].getNormalVoltage(clrGate,i)+params[CLR_BUTTON].getValue();
		if(inputs[SWAP_INPUT].getNormalVoltage(swapGate,i)+params[SWAP_BUTTON].getValue()>lastSwap){
			queuedParams[SWAP_TPARAM][i] = true;
		}
		lastSwap = inputs[SWAP_INPUT].getNormalVoltage(swapGate,i)+params[SWAP_BUTTON].getValue();

	/*
	
		In this section, apply all transition curves
	
	*/

		bool transition = false;
		transition = transition || (queuedParams[REC_TPARAM][i] != currentParams[REC_TPARAM][i]);
		transition = transition || (queuedParams[REV_TPARAM][i] != currentParams[REV_TPARAM][i]);
		transition = transition || queuedParams[CLR_TPARAM][i];
		transition = transition || queuedParams[SWAP_TPARAM][i];

		if(transition){
			transitionProgress[i]+=args.sampleTime/params[TRANS_PARAM].getValue()*1000;
		}else{
			transitionProgress[i]-=args.sampleTime/params[TRANS_PARAM].getValue()*1000;
		}

		if(transitionProgress[i] >= 1){
			transitionProgress[i] = 1;
			currentParams[0][i] = queuedParams[0][i];
			currentParams[1][i] = queuedParams[1][i];
			currentParams[2][i] = queuedParams[2][i];
			currentParams[3][i] = queuedParams[3][i];
		}

		if(transitionProgress[i] <= 0){
			transitionProgress[i] = 0;
		}

	/*
	
		In this section, process audio and trigs
	
	*/

		lights[REC_LIGHT].setBrightness(static_cast<float>(currentParams[REC_TPARAM][1]));
		lights[REV_LIGHT].setBrightness(static_cast<float>(currentParams[REV_TPARAM][1]));

		if(!inputs[AUDIO_INPUT].isConnected()){//Do nothing if disconnected
			return;
		}

		float audio = inputs[AUDIO_INPUT].getVoltage(i);

		if(currentParams[CLR_TPARAM][i]){//Clear on trig
			buf1[i].clear();
			buf2[i].clear();
			lights[CLR_LIGHT].setBrightness(1.f);
			currentParams[CLR_TPARAM][i] = false;
			queuedParams[CLR_TPARAM][i] = false;
		}
		lights[CLR_LIGHT].setBrightness(lights[CLR_LIGHT].getBrightness()/1.0005f);

		if(currentParams[SWAP_TPARAM][i]){//Swap on trig
			buf1[i].swap(&buf2[i]);
			lights[SWAP_LIGHT].setBrightness(1.f);
			currentParams[SWAP_TPARAM][i] = false;
			queuedParams[SWAP_TPARAM][i] = false;
		}
		lights[SWAP_LIGHT].setBrightness(lights[SWAP_LIGHT].getBrightness()/1.0005f);

		//DO THE PROCESS

		if(!currentParams[REC_TPARAM][i]&&!currentParams[REV_TPARAM][i]&&!buf2[i].empty()){//POP FROM 2
			buf1[i].push((buf2[i].top()*fb)*(1-transitionProgress[i]));
			buf2[i].pop();
		}

		if(currentParams[REC_TPARAM][i]&&!currentParams[REV_TPARAM][i]&&!buf2[i].empty()){//POP FROM 2
			buf1[i].push((buf2[i].top()*fb+audio)*(1-transitionProgress[i]));
			buf2[i].pop();
		}

		if(!currentParams[REC_TPARAM][i]&&currentParams[REV_TPARAM][i]&&!buf1[i].empty()){//POP FROM 1
			buf2[i].push((buf1[i].top()*fb)*(1-transitionProgress[i]));
			buf1[i].pop();
		}

		if(currentParams[REC_TPARAM][i]&&currentParams[REV_TPARAM][i]&&!buf1[i].empty()){//POP FROM 1
			buf2[i].push((buf1[i].top()*fb+audio)*(1-transitionProgress[i]));
			buf1[i].pop();
		}

		if(currentParams[REC_TPARAM][i]&&!currentParams[REV_TPARAM][i]&&buf2[i].empty()){
			buf1[i].push(audio*(1-transitionProgress[i]));
		}

		if(currentParams[REC_TPARAM][i]&&currentParams[REV_TPARAM][i]&&buf1[i].empty()){
			buf2[i].push(audio*(1-transitionProgress[i]));
		}

		//Outputs each output shaped by transition window
		float cv = 0;
		if(!buf1[i].empty()&&currentParams[REV_TPARAM][i]){
			cv += buf1[i].top();
		}
		if(!buf2[i].empty()&&!currentParams[REV_TPARAM][i]){
			cv +=buf2[i].top();
		}

		outputs[BUF1_OUTPUT].setVoltage(cv*(1-transitionProgress[i]),i);
	}

	}
};

struct ElasticTwinsWidget : ModuleWidget {
	int theme = -1;

	ElasticTwinsWidget(ElasticTwins* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/ElasticTwin.svg")));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.54, 19.50)), module, ElasticTwins::REC_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(15.685, 19.50)), module, ElasticTwins::CLR_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(24.955, 19.50)), module, ElasticTwins::SWAP_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(34.1, 19.50)), module, ElasticTwins::REV_INPUT));

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(mm2px(Vec(6.54, 41.5)), module, ElasticTwins::REC_BUTTON, ElasticTwins::REC_LIGHT));
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(15.685, 41.5)), module, ElasticTwins::CLR_BUTTON, ElasticTwins::CLR_LIGHT));
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(24.955, 41.5)), module, ElasticTwins::SWAP_BUTTON, ElasticTwins::SWAP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(34.1, 41.5)), module, ElasticTwins::REV_BUTTON, ElasticTwins::REV_LIGHT));

		addParam(createParamCentered<CKSSThree>(mm2px(Vec(6.54, 30.5)), module, ElasticTwins::CLK_REC_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(15.685, 30.5)), module, ElasticTwins::CLK_CLR_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(24.955, 30.5)), module, ElasticTwins::CLK_SWAP_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(34.1, 30.5)), module, ElasticTwins::CLK_REV_SWITCH));
		
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(6.54, 101.5)), module, ElasticTwins::FEEDBACK_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(34.1, 101.5)), module, ElasticTwins::TRANS_PARAM));

		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(12.54, 63.5)), module, ElasticTwins::CLK1_PARAM));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(28.1, 81.5)), module, ElasticTwins::CLK2_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(15.875, 114.5)), module, ElasticTwins::MODE1_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(24.765, 114.5)), module, ElasticTwins::MODE2_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(32.56, 60.00)), module, ElasticTwins::CLK_SYNC_SWITCH));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.54, 114.5)), module, ElasticTwins::AUDIO_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(34.1, 114.5)), module, ElasticTwins::BUF1_OUTPUT));
	}
	void appendContextMenu(Menu* menu) override {
		ElasticTwins* module = getModule<ElasticTwins>();

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
		ElasticTwins* module = getModule<ElasticTwins>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			if(theme>=0){
				setPanel(createTintPanel(
					"res/panels/ElasticTwinTintLayers.svg",
					getPalette(theme)
				));
			}else{
				setPanel(createPanel(asset::plugin(pluginInstance,"res/panels/ElasticTwin.svg")));
			}
		}
	}
};

Model* modelElasticTwins = createModel<ElasticTwins, ElasticTwinsWidget>("ElasticTwins");