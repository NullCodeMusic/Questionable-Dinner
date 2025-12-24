#include "plugin.hpp"
#include "tinyexpr.h"
#include "lookups.hpp"
#include "palettes.hpp"
#include <cstdlib>


struct Surgeon : Module {
	//Copy Pase Channel Themes Stuff
	int theme = -1;
	void setTheme(int newTheme){
		theme = newTheme;
	}
	int getTheme(){
		return theme;
	}

	//Module
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
		double partialsInfo[6][65]={0};
		double randoms[65]={0};
		bool frozen = false;
		bool error = false;
		int voices = 0;

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
			"1"
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

	te_variable vars[14] = {
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

	//QUALITY
	int slowTickRate = 64;
	int slowTickTimer = 0;
	int getQuality(){
		switch (slowTickRate)
		{
		case 2:
			return 4;
		
		case 8:
			return 3;

		case 24:
			return 2;

		case 64:
			return 1;

		case 128:
			return 0;
		
		default:
			return 1;
		}
	}
	void setQuality(int qual){
		switch (qual)
		{
		case 0:
			slowTickRate = 128;
			break;
		
		case 1:
			slowTickRate = 64;
			break;

		case 2:
			slowTickRate = 24;
			break;

		case 3:
			slowTickRate = 8;
			break;

		case 4:
			slowTickRate = 2;
			break;
		
		default:
			slowTickRate = 64;
			break;
		}
	}

	//POLYPHONY
	int channels = 1;


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
		configParam(R_PARAM, 1000, 2000, 1234, "R Seed", "",0.0f,1.0f,-1000.f)->snapEnabled=true;
		configParam(NUM_PARAM, 1.f, 64.f, 1.f, "Partials");
		configInput(TRIG_INPUT, "Trig");
		configInput(PITCH_INPUT, "Pitch");
		configOutput(OUT_OUTPUT, "Audio");
	}
	
	void onAdd(const AddEvent& e) override{
		processStrings();
		setQuality(getQuality());
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
			for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
				std::string key = "expr"+std::to_string(iter);
				json_object_set_new(rootJ, key.c_str(), json_string(texts[iter].c_str()));
			}
			json_object_set_new(rootJ, "theme", json_integer(theme));
			json_object_set_new(rootJ, "quality", json_integer(slowTickRate));
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
		json_t* modeJ = json_object_get(rootJ, "theme");
		if (modeJ){
			setTheme(json_integer_value(modeJ));
		}
		json_t* qualJ = json_object_get(rootJ, "quality");
		if (qualJ){
			slowTickRate = json_integer_value(qualJ);
		}
	}

	void onReset(const ResetEvent &e) override {
		Module::onReset(e);
		texts[EXPR_J] = "0";
		texts[EXPR_K] = "0";
		texts[EXPR_L] = "0";
		texts[EXPR_WARP] = "0";
		texts[EXPR_ATTACK] = "0.0039";
		texts[EXPR_DECAY] = "0.5";
		texts[EXPR_PITCH] = "i*f";
		texts[EXPR_AMP] = "1/i*e";
		texts[EXPR_PHASE] = "1";
		if(fieldsLoaded){
			for(int iter=0;iter<EXPRESSIONS_LEN;iter++){
			fields[iter]->setText(texts[iter]);
			}
		}
	}

	void process(const ProcessArgs& args) override {

		if(!fieldsLoaded){
			return;
		}

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

				//slowTickRate = n/4+1;
				
				slowTickTimer = -1;
				processStringFields();

			}


			baseTrigTime[0]+=args.sampleTime;

			trigInput = inputs[TRIG_INPUT].getVoltage();

			slowTickTimer++;
			if(slowTickTimer>=slowTickRate){
				slowTickTimer=0;
			}
			if(slowTickTimer==0){
					slowTick(args,trig);
			}

		}
		float audioOut=0;
		float lerp = float(slowTickTimer)/float(slowTickRate);
		for (int iint = 1; iint <= n; iint++){

			float lerpPitch = partialsInfo[3][iint]*lerp+partialsInfo[0][iint]*(1-lerp);
			float lerpPhase = partialsInfo[4][iint]*lerp+partialsInfo[1][iint]*(1-lerp);
			float lerpAmp = partialsInfo[5][iint]*lerp+partialsInfo[2][iint]*(1-lerp);

			baseTrigTime[iint] += args.sampleTime*lerpPitch;

			if(baseTrigTime[iint]>1){//Switch to minus int cast if other stuff works
				baseTrigTime[iint] --;
			}
			float adjustedSineTime = (baseTrigTime[iint]+lerpPhase);
			audioOut += lu_sin(adjustedSineTime) * lerpAmp;
		}
		if(!std::isnan(audioOut)){
			outputs[OUT_OUTPUT].setVoltage(audioOut);
			out = audioOut;
		}
	}

	void slowTick(const ProcessArgs args, bool trig){

		x = params[X_PARAM].getValue()*inputs[X_INPUT].getNormalVoltage(1.0f);
		y = params[Y_PARAM].getValue()*inputs[Y_INPUT].getNormalVoltage(1.0f);
		z = params[Z_PARAM].getValue()*inputs[Z_INPUT].getNormalVoltage(1.0f);

		n = params[NUM_PARAM].getValue();

		f = dsp::FREQ_C4 * dsp::exp2_taylor5(inputs[PITCH_INPUT].getVoltage());

		// RECALCULATE CONTROLS

		if(frozen){return;}

		double timeWarp;
		double attack;
		double decay;


		if(trig){
		
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

		}else{
			std::swap(partialsInfo[0],partialsInfo[3]);
			std::swap(partialsInfo[1],partialsInfo[4]);
			std::swap(partialsInfo[2],partialsInfo[5]);
		}

		for(i=1;i<=n;i++){

			int iint = static_cast<int>(i);

			r = randoms[iint];

			j=te_eval(expressions[EXPR_J]);
			k=te_eval(expressions[EXPR_K]);
			l=te_eval(expressions[EXPR_L]);

			timeWarp=te_eval(expressions[EXPR_WARP]);

			t=timeWarp+baseTrigTime[0]+args.sampleTime*slowTickRate;

			attack=te_eval(expressions[EXPR_ATTACK]);
			decay=te_eval(expressions[EXPR_DECAY]);

			e = env(attack,decay);
			
			partialsInfo[3][iint]=te_eval(expressions[EXPR_PITCH]);
			partialsInfo[4][iint]=te_eval(expressions[EXPR_PHASE]);
			partialsInfo[5][iint]=te_eval(expressions[EXPR_AMP])*(partialsInfo[0][iint]<args.sampleRate/2);

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
	int theme = -1;
	QTintPanel* tintPanel;
	SurgeonWidget(Surgeon* module) {
		setModule(module);

		tintPanel = createTintPanel(
			"res/panels/SurgeonTintLayers.svg",
			getPalette(PAL_PEACHBERRY)
		);
		setPanel(tintPanel);

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

		menu->addChild(createIndexSubmenuItem(
			"Quality", 
			{"Low", "Standard", "High", "Very High", "Ultra"},	
			[=](){
				return module->getQuality();
			},
			[=](int qual) {
				module->setQuality(qual);
			}
		));
	}

	void step() override {
		ModuleWidget::step();
		Surgeon* module = getModule<Surgeon>();
		if(!module){
			return;
		}

		module->infodisplay->color = nvgRGBAf(1,1,1,0.82);
		if(module->fields[Surgeon::EXPR_J]->hovered){
			module->infodisplay->text = "Variable J";
		}else if(module->fields[Surgeon::EXPR_K]->hovered){
			module->infodisplay->text = "Variable K";
		}else if(module->fields[Surgeon::EXPR_L]->hovered){
			module->infodisplay->text = "Variable L";
		}else if(module->fields[Surgeon::EXPR_WARP]->hovered){
			module->infodisplay->text = "Time Warp";
		}else if(module->fields[Surgeon::EXPR_ATTACK]->hovered){
			module->infodisplay->text = "Env Attack";
		}else if(module->fields[Surgeon::EXPR_DECAY]->hovered){
			module->infodisplay->text = "Env Decay";
		}else if(module->fields[Surgeon::EXPR_PITCH]->hovered){
			module->infodisplay->text = "Pitch";
		}else if(module->fields[Surgeon::EXPR_PHASE]->hovered){
			module->infodisplay->text = "Phase";
		}else if(module->fields[Surgeon::EXPR_AMP]->hovered){
			module->infodisplay->text = "Amplitude";
		}else{
			module->infodisplay->text = "SURGEON";
			module->infodisplay->color = nvgRGBAf(1,1,1,0.5);
		}

		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			if(theme>=0){
				setPanel(createTintPanel(
					"res/panels/SurgeonTintLayers.svg",
					getPalette(theme)
				));
			}else{
				setPanel(createPanel(asset::plugin(pluginInstance,"res/panels/Surgeon.svg")));
			}
		}
	}
};


Model* modelSurgeon = createModel<Surgeon, SurgeonWidget>("Surgeon");