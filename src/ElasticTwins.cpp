#include "plugin.hpp"
#include <stack>
#include <queue>
#include "mymath.hpp"

struct ElasticTwins : Module {
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

	ElasticTwins() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(AUDIO_INPUT, "Audio");
		configInput(REV_INPUT, "Select CV");
		configInput(REC_INPUT, "Record CV");
		configInput(CLR_INPUT, "Clear CV");
		configInput(SWAP_INPUT, "Swap CV");
		configOutput(BUF1_OUTPUT, "Audio");
		configButton(REV_BUTTON, "Select Current Buffer");
		configButton(REC_BUTTON, "Record To Buffer");
		configButton(CLR_BUTTON, "Clear Buffers");
		configButton(SWAP_BUTTON, "Swap Buffer Data");
		configSwitch(MODE1_SWITCH, 0, 1, 0, "Buffer 1 Mode", {"Reverse","Forward"});
		configSwitch(MODE2_SWITCH, 0, 1, 0, "Buffer 2 Mode", {"Reverse","Forward"});
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 1.f, "Feedback");
		configParam(TRANS_PARAM, .0001f, 1.f, 0.12f, "Smooth");

		//CLOCK STUFF
		configParam(CLK1_PARAM, -4, 12, 0, "Clock 1", "hz", 2.f);
		configParam(CLK2_PARAM, -4, 12, 0, "Clock 2", "hz", 2.f);
		configSwitch(CLK_CLR_SWITCH,-1,1,0,"Clear Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_REV_SWITCH,-1,1,0,"Select Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_REC_SWITCH,-1,1,0,"Record Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_SWAP_SWITCH,-1,1,1,"Swap Clock",{"Clock 2","None","Clock 1"});
		configSwitch(CLK_SYNC_SWITCH,0,1,9, "Sync");
	}

	staque<float> buf1;
	float out1=0;
	float slope1=0;
	staque<float> buf2;
	float out2=0;
	float slope2=0;
	bool reverse = false;

	float lastClear;
	float lastSwap;

	float clock1progress;
	float clock2progress;

	void process(const ProcessArgs& args) override {
		//UPDATE CLOCKS
		float clock1Rate = dsp::exp2_taylor5(params[CLK1_PARAM].getValue())*args.sampleTime;
		float clock2Rate = dsp::exp2_taylor5(params[CLK2_PARAM].getValue())*args.sampleTime;
		clock1progress += clock1Rate;
		clock2progress += clock2Rate;
		if(clock1progress>=1){
			clock1progress--;
			if(params[CLK_SYNC_SWITCH].getValue()>0.1){
				clock2progress=0;
			}
		}
		if(clock2progress>=1){
			clock2progress--;
		}
		bool clock1Gate = clock1progress<=0.5f;
		bool clock2Gate = clock2progress<=0.5f;

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
		buf1.mode = params[MODE1_SWITCH].getValue();
		buf2.mode = params[MODE2_SWITCH].getValue();

		//GATES
		bool recording = !(inputs[REC_INPUT].getNormalVoltage(recGate)>0.1) != !(params[REC_BUTTON].getValue()>0.1);
		bool reverse = !(inputs[REV_INPUT].getNormalVoltage(revGate)>0.1) != !(params[REV_BUTTON].getValue()>0.1);

		//FEEDBACK AND FILTER
		float fb = params[FEEDBACK_PARAM].getValue();
		float slew = 1/params[TRANS_PARAM].getValue()-1;

		//TRIGS
		float clear = inputs[CLR_INPUT].getNormalVoltage(clrGate)+params[CLR_BUTTON].getValue();
		float swap = inputs[SWAP_INPUT].getNormalVoltage(swapGate)+params[SWAP_BUTTON].getValue();

		lights[REC_LIGHT].setBrightness(static_cast<float>(recording));
		lights[REV_LIGHT].setBrightness(static_cast<float>(reverse));

		if(!inputs[AUDIO_INPUT].isConnected()){//Do nothing if disconnected
			return;
		}

		float audio = inputs[AUDIO_INPUT].getVoltageSum()/inputs[AUDIO_INPUT].getChannels();

		if(clear>lastClear){//Clear on trig
			buf1.clear();
			buf2.clear();
			lights[CLR_LIGHT].setBrightness(1.f);
		}
		lights[CLR_LIGHT].setBrightness(lights[CLR_LIGHT].getBrightness()/1.0005f);
		lastClear = clear;

		if(swap>lastSwap){//Swap on trig
			buf1.swap(&buf2);
			lights[SWAP_LIGHT].setBrightness(1.f);
		}
		lights[SWAP_LIGHT].setBrightness(lights[SWAP_LIGHT].getBrightness()/1.0005f);
		lastSwap = swap;

		//DO THE PROCESS

		if(!recording&&!reverse&&!buf2.empty()){//POP FROM 2
			buf1.push(buf2.top()*fb);
			buf2.pop();
		}

		if(recording&&!reverse&&!buf2.empty()){//POP FROM 2
			buf1.push(buf2.top()*fb+audio);
			buf2.pop();
		}

		if(!recording&&reverse&&!buf1.empty()){//POP FROM 1
			buf2.push(buf1.top()*fb);
			buf1.pop();
		}

		if(recording&&reverse&&!buf1.empty()){//POP FROM 1
			buf2.push(buf1.top()*fb+audio);
			buf1.pop();
		}

		if(recording&&!reverse&&buf2.empty()){
			buf1.push(audio);
		}

		if(recording&&reverse&&buf1.empty()){
			buf2.push(audio);
		}

		//Outputs each output shaped by a declick filter
		float cv = 0;
		if(!buf1.empty()&&reverse){
			cv += buf1.top();
		}
		if(!buf2.empty()&&!reverse){
			cv +=buf2.top();
		}
		slope1 = abs(cv-out1);
		if(slope1>slew){
			out1 = cv*0.1+out1*0.9;
		}else{
			out1 = cv;
		}

		outputs[BUF1_OUTPUT].setVoltage(out1);

	}
};



struct ElasticTwinsWidget : ModuleWidget {
	ElasticTwinsWidget(ElasticTwins* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/test.svg")));

		addInput(createInputCentered<QPort>(mm2px(Vec(19.527, 18.667)), module, ElasticTwins::REC_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(19.527, 8.667)), module, ElasticTwins::SWAP_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(19.527, 28.667)), module, ElasticTwins::REV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(19.527, 38.667)), module, ElasticTwins::CLR_INPUT));

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<RedLight>>>(mm2px(Vec(29.527, 18.667)), module, ElasticTwins::REC_BUTTON, ElasticTwins::REC_LIGHT));
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(29.527, 8.667)), module, ElasticTwins::SWAP_BUTTON, ElasticTwins::SWAP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(29.527, 28.667)), module, ElasticTwins::REV_BUTTON, ElasticTwins::REV_LIGHT));
		addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(29.527, 38.667)), module, ElasticTwins::CLR_BUTTON, ElasticTwins::CLR_LIGHT));

		addParam(createParamCentered<CKSSThree>(mm2px(Vec(9.527, 18.667)), module, ElasticTwins::CLK_REC_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(9.527, 8.667)), module, ElasticTwins::CLK_SWAP_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(9.527, 28.667)), module, ElasticTwins::CLK_REV_SWITCH));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(9.527, 38.667)), module, ElasticTwins::CLK_CLR_SWITCH));
		
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.527, 60.667)), module, ElasticTwins::FEEDBACK_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.527, 50.667)), module, ElasticTwins::TRANS_PARAM));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.527, 70.667)), module, ElasticTwins::CLK1_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.527, 80.667)), module, ElasticTwins::CLK2_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(9.527, 98.667)), module, ElasticTwins::MODE1_SWITCH));
		addParam(createParamCentered<CKSS>(mm2px(Vec(29.527, 98.667)), module, ElasticTwins::MODE2_SWITCH));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.336, 98.181)), module, ElasticTwins::AUDIO_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.553, 116.914)), module, ElasticTwins::BUF1_OUTPUT));
	}
};


Model* modelElasticTwins = createModel<ElasticTwins, ElasticTwinsWidget>("ElasticTwins");