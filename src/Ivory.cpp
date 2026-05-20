//Module Slug Ivory - find and replace this to make a new one

#include "plugin.hpp"
#include "mydsp.hpp"
#include "mymath.hpp"
#include "xywidget.hpp"

using namespace simd;

struct Ivory : Module {
	int theme = -1;
	
	enum ParamId {
		X0_PARAM,
		Y0_PARAM=4,
		DELAY_PARAM=8,
		WIDTH_PARAM,
		HEIGHT_PARAM,
		OUTX0_PARAM,
		OUTX1_PARAM,
		OUTY0_PARAM,
		OUTY1_PARAM,
		FEED_PARAM,
		DIFFUSE_PARAM,
		LOWPASS_PARAM,
		TONE_PARAM,
		CLOSE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIOL_OUTPUT,
		AUDIOR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Ivory() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(0,-1,1,0,"Point 1 X");
		configParam(1,-1,1,0,"Point 2 X");
		configParam(2,-1,1,0,"Point 3 X");
		configParam(3,-1,1,0,"Point 4 X");
		configParam(11,-1,1,0,"Point L X");
		configParam(12,-1,1,0,"Point R X");

		configParam(4,-1,1,0,"Point 1 Y");
		configParam(5,-1,1,0,"Point 2 Y");
		configParam(6,-1,1,0,"Point 3 Y");
		configParam(7,-1,1,0,"Point 4 Y");
		configParam(13,-1,1,0,"Point L Y");
		configParam(14,-1,1,0,"Point R Y");

		configParam(DELAY_PARAM, 0.00291545189,0.5,0,"Delay"," seconds");
		configParam(WIDTH_PARAM,0.02,1,0.9,"Width");
		configParam(HEIGHT_PARAM,0.02,1,0.9,"Height");
		configParam(FEED_PARAM,0,1,0,"Feedback");
		configParam(DIFFUSE_PARAM,0,10,0,"Diffuse");
		configParam(LOWPASS_PARAM,-9,0,0,"Brightness");
		configParam(TONE_PARAM,-9,0,-5,"Tone");
		configParam(CLOSE_PARAM,0,1,1,"Closeness");

		configBypass(AUDIO_INPUT,AUDIOL_OUTPUT);
		configBypass(AUDIO_INPUT,AUDIOR_OUTPUT);
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

	DelayLine<44100*60> delayLine;
	DelayLine<44100*60> feedbackLine;
	float delayCache[6][4][30];
	float ampCache[6][4][30] = {{{1}}};
	PointList* pointList;
	int delayDensity = 30;
	dsp::RCFilter hp;
	dsp::RCFilter lp;
	float diff;

	void onAdd(const AddEvent& e) override{
		pointList = new PointList(
			{
				new XYPoint(Vec(-0.6,-0.5),"A",SHP_CIRCLE,nvgRGBf(211.f/255.f, 42.f/255.f, 70.f/255.f)),
				//new XYPoint(Vec(-0.2,-0.5),"B",SHP_CIRCLE,nvgRGBf(198.f/255.f, 180.f/255.f, 0.f/255.f)),
				//new XYPoint(Vec(0.2,-0.5),"C",SHP_CIRCLE,nvgRGBf(128.f/255.f, 177.f/255.f, 53.f/255.f)),
				//new XYPoint(Vec(0.6,-0.5),"D",SHP_CIRCLE,nvgRGBf(68.f/255.f, 140.f/255.f, 234.f/255.f)),
				new XYPoint(Vec(0.2,-0.5),"L",SHP_SQUARE),
				new XYPoint(Vec(0.6,-0.5),"R",SHP_SQUARE)
			},
			{
				getParamQuantity(0),
				//getParamQuantity(1),
				//getParamQuantity(2),
				//getParamQuantity(3),
				getParamQuantity(11),
				getParamQuantity(12)
			},
			{
				getParamQuantity(4),
				//getParamQuantity(5),
				//getParamQuantity(6),
				//getParamQuantity(7),
				getParamQuantity(13),
				getParamQuantity(14)
			}
		);
		pointList->readFromParams();
	}

	inline void cacheEmitter(float x, float y, float a, float b, int id){
		float amj2 = a-y;
		amj2*=amj2;
		float apj2 = a+y;
		apj2*=apj2;
		float bmi2 = b-x;
		bmi2*=bmi2;
		float bpi2 = b+x;
		bpi2*=bpi2;

		for(int i = 0; i<=delayDensity; i++){
			float gridPos = float(i)/float(delayDensity)*2.f-1.f;
			float delay;
			if(gridPos>=-a&&gridPos<=a){
				delay = gridPos-x;
				delay = delay*delay+apj2;
				delayCache[id][0][i]=sqrt(delay);
				ampCache[id][0][i]=1/(1+delay*delay);
				delay = gridPos-x;
				delay = delay*delay+amj2;
				delayCache[id][1][i]=sqrt(delay);
				ampCache[id][1][i]=1/(1+delay*delay);
			}else{
				delayCache[id][0][i]=0;
				ampCache[id][0][i]=0;
				delayCache[id][1][i]=0;
				ampCache[id][1][i]=0;
			}
			if(gridPos>=-b&&gridPos<=b){
				delay = gridPos-y;
				delay = delay*delay+bpi2;
				delayCache[id][2][i]=sqrt(delay);
				ampCache[id][2][i]=1/(1+delay*delay);
				delay = gridPos-y;
				delay = delay*delay+bmi2;
				delayCache[id][3][i]=sqrt(delay);
				ampCache[id][3][i]=1/(1+delay*delay);
			}else{
				delayCache[id][2][i]=0;
				ampCache[id][2][i]=0;
				delayCache[id][3][i]=0;
				ampCache[id][3][i]=0;
			}
		}
	}

	inline float getDelayBounceOutput(DelayLine<44100*60>* dl, int startID, int endID, float delayMult, float w, float h){
		float outx = 0;
		float outy = 0;

		for(int i = 0; i<=delayDensity; i++){
			outx+=dl->readRaw((delayCache[startID][0][i]+delayCache[endID][0][i])*delayMult)*ampCache[startID][0][i]*ampCache[endID][0][i];
			outx+=dl->readRaw((delayCache[startID][1][i]+delayCache[endID][1][i])*delayMult)*ampCache[startID][1][i]*ampCache[endID][1][i];
			outy+=dl->readRaw((delayCache[startID][2][i]+delayCache[endID][2][i])*delayMult)*ampCache[startID][2][i]*ampCache[endID][2][i];
			outy+=dl->readRaw((delayCache[startID][3][i]+delayCache[endID][3][i])*delayMult)*ampCache[startID][3][i]*ampCache[endID][3][i];
		}
		return (outx/w+outy/h)/delayDensity;
	}

	inline float getDelayRoomOutput(DelayLine<44100*60>* dl, int id, float delayMult, float w, float h){
		float outx = 0;
		float outy = 0;

		srand(2310);
		for(int i = 0; i<=delayDensity; i++){
			outx+=dl->readRaw((delayCache[id][0][i])+diff*randf())*ampCache[id][0][i];
			outx+=dl->readRaw((delayCache[id][1][i])+diff*randf())*ampCache[id][1][i];
			outy+=dl->readRaw((delayCache[id][2][i])+diff*randf())*ampCache[id][2][i];
			outy+=dl->readRaw((delayCache[id][3][i])+diff*randf())*ampCache[id][3][i];
		}
		return (outx/w+outy/h)/delayDensity;
	}

	inline float getDelayDirect(DelayLine<44100*60>* dl,float x1,float y1,float x2, float y2, float delayMult){
		float x = x2-x1;
		float y = y2-y1;
		float delay = sqrt(x*x+y*y);
		return dl->readRaw(delay*delayMult)/(1+delay*delay);
	}

	inline float getDistance(float x1,float y1,float x2, float y2){
		float x = x2-x1;
		float y = y2-y1;
		float delay = sqrt(x*x+y*y);
		return delay;
	}

	inline float randf(){
		return float(rand()%1000)/1000;
	}

    void process(const ProcessArgs& args) override {
		float delayMult = params[DELAY_PARAM].getValue()*args.sampleRate;
		float a = params[WIDTH_PARAM].getValue();
		float b = params[HEIGHT_PARAM].getValue();
		diff = params[DIFFUSE_PARAM].getValue()*args.sampleRate/1000;
		float lpf = dsp::exp2_taylor5(params[LOWPASS_PARAM].getValue())*20000*args.sampleTime;
		float hpf = dsp::exp2_taylor5(params[TONE_PARAM].getValue())*20000*args.sampleTime;
		float closeness = params[CLOSE_PARAM].getValue();
		pointList->screen->setBounds(a,b);

		float feedback = softClip(feedbackLine.readLerp(delayMult*a)*a+feedbackLine.readLerp(delayMult*b)*b,5);

		delayLine.write(inputs[AUDIO_INPUT].getVoltage()+feedback*params[FEED_PARAM].getValue());
		feedbackLine.process();
		//Math for single point

		cacheEmitter(params[0].getValue(),params[4].getValue(),a,b,0);
		//cacheEmitter(params[1].getValue(),params[5].getValue(),a,b,1);
		//cacheEmitter(params[2].getValue(),params[6].getValue(),a,b,2);
		//cacheEmitter(params[3].getValue(),params[7].getValue(),a,b,3);
		cacheEmitter(params[11].getValue(),params[13].getValue(),a,b,4);
		cacheEmitter(params[12].getValue(),params[14].getValue(),a,b,5);

		float lDelay = getDistance(params[0].getValue(),params[4].getValue(),params[11].getValue(),params[13].getValue());
		float rDelay = getDistance(params[0].getValue(),params[4].getValue(),params[12].getValue(),params[14].getValue());
		float comp = std::min(lDelay,rDelay)*closeness;
		lDelay -= comp;
		rDelay -= comp;

		float lRoom = getDelayBounceOutput(&delayLine,0,4,delayMult,a,b);
		float lDirect = delayLine.readRaw(lDelay*delayMult)/(1+lDelay*lDelay);
		float rRoom = getDelayBounceOutput(&delayLine,0,5,delayMult,a,b);
		float rDirect = delayLine.readRaw(rDelay*delayMult)/(1+rDelay*rDelay);

		outputs[AUDIOL_OUTPUT].setVoltage(lRoom+lDirect);
		outputs[AUDIOR_OUTPUT].setVoltage(rRoom+rDirect);

		hp.setCutoffFreq(hpf);
		hp.process(getDelayRoomOutput(&delayLine,0,delayMult,a,b));

		lp.setCutoffFreq(lpf);
		lp.process(hp.highpass());

		feedbackLine.write(clamp(lp.lowpass(),-10.f,10.f));
		delayLine.process();
		pointList->writeToParams();
	}
};


struct IvoryWidget : ModuleWidget {
	int theme = -1;
	XYScreen* screen;

	IvoryWidget(Ivory* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Ivory.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
        //addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 10.0)), module, Ivory::DELAY_PARAM));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 85.0)), module, Ivory::AUDIO_INPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.62, 95.0)), module, Ivory::AUDIOL_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.62, 105.0)), module, Ivory::AUDIOR_OUTPUT));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(20, 95.0)), module, Ivory::WIDTH_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(20, 110.0)), module, Ivory::CLOSE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35, 95.0)), module, Ivory::HEIGHT_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(35, 110.0)), module, Ivory::TONE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(50, 95.0)), module, Ivory::DELAY_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(50, 110.0)), module, Ivory::LOWPASS_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(65, 95.0)), module, Ivory::FEED_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(65, 110.0)), module, Ivory::DIFFUSE_PARAM));
		screen = createWidget<XYScreen>(mm2px(Vec(3.810,9.000)));
		screen->setSize(mm2px(Vec(68.580, 68.580)));
		addChild(screen);
		if(module){
			module->pointList->addPointsToScreen(screen);
		}
	}

	void appendContextMenu(Menu* menu) override {
		Ivory* module = getModule<Ivory>();
		
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
		Ivory* module = getModule<Ivory>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Ivory.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelIvory = createModel<Ivory, IvoryWidget>("Ivory");