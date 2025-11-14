#include "plugin.hpp"
#include "tinyexpr.h"
#include "lookups.hpp"
#include <cstdlib>


struct Surgeon : Module {
	enum ParamId {
		X_PARAM,
		Y_PARAM,
		Z_PARAM,
		R_PARAM,
		FREEZE_BUTTON,
		NUM_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		PITCH_INPUT,
		X_INPUT,
		Y_INPUT,
		Z_INPUT,
		FREEZE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	enum ExpressionId{
		EXPR_J,
		EXPR_K,
		EXPR_L,
		EXPR_WARP,
		EXPR_ATTACK,
		EXPR_DECAY,
		EXPR_PITCH,
		EXPR_AMP,
		EXPR_PHASE,
		EXPRESSIONS_LEN
	};

	//MODULE MAIN STUFF
		float trigInput = 0;
		bool triggable = true;
		QInfoText* infodisplay;
		double baseTrigTime[65]={300};
		double partialsInfo[3][65]={0};
		double randoms[65]={0};
		bool frozen = false;
		bool error = false;

	//TINY EXPRESSION STUFF
		te_expr *expressions[EXPRESSIONS_LEN];
		QDataEntry* fields[EXPRESSIONS_LEN];
		bool fieldsLoaded = false;
		std::string texts[EXPRESSIONS_LEN] = {
			"0",
			"0",
			"0",
			"0",
			"0.0039",
			"0.5",
			"i*f",
			"1/i*e",
			"0"
		};
	//Param/CV Vars
		double x;
		double y;
		double z;
		double n = 32; //number of partials
	//Per Partial User Vars
		double j;
		double k;
		double l;
	//Internally Defined Vars
		double p; //previous value
		double f; //fundamental frequency
		double t; //time since last trig, Extension that lets you warp time
		double e; //a per partial AD envelope based on defined stuff
		double r; //"random" per partial sample and hold noise
		double i = 1; //the index of current partial
		double out;

	te_variable vars[13] = {
		{"x", &x}, {"y", &y}, {"z", &z},
		{"j", &j}, {"k", &k}, {"l", &l}, 
		{"f", &f},
		{"t", &t},
		{"r", &r},
		{"e", &e},
		{"n", &n},
		{"i", &i},
		{"out", &out}
		};

	//SLOW TICK
	int slowTickRate = 16;
	int slowTickTimer = 0;


	double env(double a,double d){
		double pos = t;
		if(d>0){
		return (pos/a)*(pos>=0)*(pos<=a)/*Attack Slope*/+(1-(pos-a)/d)*(pos>a)*(pos<=a+d)/*Decay Slope*/;
		}
		return 0;
	}
	
	Surgeon() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(X_PARAM, -1.f, 1.f, 0.f, "X");
		configParam(Y_PARAM, -1.f, 1.f, 0.f, "Y");
		configParam(Z_PARAM, -1.f, 1.f, 0.f, "Z");
		configButton(FREEZE_BUTTON, "Freeze");
		configParam(R_PARAM, 1000, 2000, 1234, "R Seed")->snapEnabled=true;
		configParam(NUM_PARAM, 1.f, 64.f, 1.f, "Partials");
		configInput(TRIG_INPUT, "Trig");
		configInput(PITCH_INPUT, "Pitch");
		configOutput(OUT_OUTPUT, "Audio");
	}
	
	void onAdd(const AddEvent& e) override{
		processStrings();
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
			for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
				std::string key = "expr"+std::to_string(iter);
				json_object_set_new(rootJ, key.c_str(), json_string(texts[iter].c_str()));
			}
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
			std::string key = "expr"+std::to_string(iter);
			json_t* exprJson = json_object_get(rootJ, key.c_str());
			if (exprJson){
				texts[iter] = json_string_value(exprJson);
				if(fieldsLoaded){
					fields[iter]->setText(texts[iter]);
				}
			}
		}
	}

	void process(const ProcessArgs& args) override {

		frozen = params[FREEZE_BUTTON].getValue()+inputs[FREEZE_INPUT].getVoltage()>0;
	
		bool trig = false;
		if(inputs[TRIG_INPUT].getVoltage()>0.1 && inputs[TRIG_INPUT].getVoltage()>trigInput && triggable){
			trig = true;
			triggable = false;
		}

		if(inputs[TRIG_INPUT].getVoltage()<=0.1){
			triggable = true;
		}

		if(!frozen){

			if(trig){
				int rng = params[R_PARAM].getValue();
				if(rng>1000){
					srand(rng);
				}
				baseTrigTime[0] = 0;
				
				for(int iter=0;iter<=n;iter++){
					if(texts[EXPR_PHASE] != "0"){
						baseTrigTime[iter] = 0;
					}
					randoms[iter]=rand()%100;
					randoms[iter]/=100;
				}
				
				slowTickTimer = 0;
				processStringFields();

			}


			baseTrigTime[0]+=args.sampleTime;

			trigInput = inputs[TRIG_INPUT].getVoltage();

			if(slowTickTimer==0){
					slowTick(args);
			}
			slowTickTimer++;
			if(slowTickTimer>=slowTickRate){
				slowTickTimer=0;
			}

		}
		float audioOut=0;
		for (int iint = 1; iint <= n; iint++){

			baseTrigTime[iint] += args.sampleTime*partialsInfo[0][iint];
			if(baseTrigTime[iint]>1){
				baseTrigTime[iint] --;
			}
			float adjustedSineTime = (baseTrigTime[iint]+partialsInfo[1][iint]);
			audioOut += lu_sin(adjustedSineTime) * partialsInfo[2][iint];
		}
		if(!std::isnan(audioOut)){
			outputs[OUT_OUTPUT].setVoltage(audioOut);
			out = audioOut;
		}
	}

	void slowTick(const ProcessArgs args){

		x = params[X_PARAM].getValue()*inputs[X_INPUT].getNormalVoltage(1.0f);
		y = params[Y_PARAM].getValue()*inputs[Y_INPUT].getNormalVoltage(1.0f);
		z = params[Z_PARAM].getValue()*inputs[Z_INPUT].getNormalVoltage(1.0f);

		n = params[NUM_PARAM].getValue();
		slowTickRate = n/4;

		f = dsp::FREQ_C4 * dsp::exp2_taylor5(inputs[PITCH_INPUT].getVoltage());

		if(infodisplay){
			infodisplay->color = nvgRGBAf(1,1,1,0.82);
			if(error){
				infodisplay->text = "ERROR";
			}else if(fields[EXPR_J]->hovered){
				infodisplay->text = "Variable J";
			}else if(fields[EXPR_K]->hovered){
				infodisplay->text = "Variable K";
			}else if(fields[EXPR_L]->hovered){
				infodisplay->text = "Variable L";
			}else if(fields[EXPR_WARP]->hovered){
				infodisplay->text = "Time Warp";
			}else if(fields[EXPR_ATTACK]->hovered){
				infodisplay->text = "Env Attack";
			}else if(fields[EXPR_DECAY]->hovered){
				infodisplay->text = "Env Decay";
			}else if(fields[EXPR_PITCH]->hovered){
				infodisplay->text = "Pitch";
			}else if(fields[EXPR_PHASE]->hovered){
				infodisplay->text = "Phase";
			}else if(fields[EXPR_AMP]->hovered){
				infodisplay->text = "Amplitude";
			}else{
				infodisplay->text = "SURGEON";
				infodisplay->color = nvgRGBAf(1,1,1,0.5);
			}
		}
		// RECALCULATE CONTROLS

		if(frozen){return;}

		double timeWarp;
		double attack;
		double decay;

		for(i=1;i<=n;i++){

			int iint = static_cast<int>(i);

			r = randoms[iint];

			j=te_eval(expressions[EXPR_J]);
			k=te_eval(expressions[EXPR_K]);
			l=te_eval(expressions[EXPR_L]);

			timeWarp=te_eval(expressions[EXPR_WARP]);

			t=timeWarp+baseTrigTime[0];

			attack=te_eval(expressions[EXPR_ATTACK]);
			decay=te_eval(expressions[EXPR_DECAY]);

			e = env(attack,decay);
			
			partialsInfo[0][iint]=te_eval(expressions[EXPR_PITCH]);
			partialsInfo[1][iint]=te_eval(expressions[EXPR_PHASE]);
			partialsInfo[2][iint]=te_eval(expressions[EXPR_AMP])*(partialsInfo[0][iint]<args.sampleRate/2);

		}
	}

	void processStringFields(){
		try{
			for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
				texts[iter]=fields[iter]->getText();
				expressions[iter]=te_compile(texts[iter].c_str(),vars,13,0);
			}
		}catch(...){
			
		}
	}

	void processStrings(){
		try{
			for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
				expressions[iter]=te_compile(texts[iter].c_str(),vars,13,0);
			}

		}catch(...){

		}
	}
};

struct SurgeonWidget : ModuleWidget {

	SurgeonWidget(Surgeon* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/qd-003/Surgeon.svg")));

		addParam(createParam<QBigNumber>(mm2px(Vec(32,64.5)), module, Surgeon::NUM_PARAM));
		addParam(createParamCentered<VCVButton>(mm2px(Vec(16.51,70.5)), module, Surgeon::FREEZE_BUTTON));

		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(10.16, 86.00)), module, Surgeon::X_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(25.40, 86.00)), module, Surgeon::Y_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(40.64, 86.00)), module, Surgeon::Z_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(31.115, 101.00)), module, Surgeon::R_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(19.685, 114.50)), module, Surgeon::TRIG_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(8.255, 114.50)), module, Surgeon::PITCH_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(8.255, 101.00)), module, Surgeon::X_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(19.685, 101.00)), module, Surgeon::Y_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(42.545, 101.00)), module, Surgeon::Z_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 70.50)), module, Surgeon::FREEZE_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(42.545, 114.50)), module, Surgeon::OUT_OUTPUT));
		if(module){
			for(int i=0;i<Surgeon::EXPRESSIONS_LEN;i++){
				module->fields[i] = createWidget<QDataEntry>(mm2px(Vec(1.5, 12+i*6)));
				module->fields[i]-> setText(module->texts[i]);
				if(i>0){
					module->fields[i]->prevField = module->fields[i-1];
					module->fields[i-1]->nextField = module->fields[i];
				}
				addChild(module->fields[i]);
			}
			module->fieldsLoaded=true;
			QInfoText* info = createWidget<QInfoText>(mm2px(Vec(3.1,7.86)));
			module->infodisplay = info;
			addChild(info);
		}

	}
	void appendContextMenu(Menu* menu) override {
		Surgeon* module = getModule<Surgeon>();

		menu -> addChild(new MenuSeparator);

		menu -> addChild(createMenuLabel(""));
	}
};


Model* modelSurgeon = createModel<Surgeon, SurgeonWidget>("Surgeon");