//Module Slug Boxes - find and replace this to make a new one

#include "plugin.hpp"
#include "mydsp.hpp"
#include "mymath.hpp"
#include "xywidget.hpp"

using namespace simd;

struct Boxes : Module {
	int theme = -1;
	
	enum ParamId {
		XI_PARAM,
		YI_PARAM,
		DELAY_PARAM,
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
		ALLPASS_BUTTON,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_INPUT,
		RECIEVE_INPUT,
		DELAY_CV_INPUT,
		WIDTH_CV_INPUT,
		HEIGHT_CV_INPUT,
		FEED_CV_INPUT,
		DIFF_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIOL_OUTPUT,
		AUDIOR_OUTPUT,
		SEND_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Boxes() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(XI_PARAM,-1,1,0,"In X");
		configParam(YI_PARAM,-1,1,0.5,"In Y");
		configParam(OUTX0_PARAM,-1,1,-0.5,"Out 0 X");
		configParam(OUTY0_PARAM,-1,1,-0.2,"Out 0 Y");
		configParam(OUTX1_PARAM,-1,1,0.5,"Out 1 X");
		configParam(OUTY1_PARAM,-1,1,-0.2,"Out 1 Y");

		configParam(DELAY_PARAM, 0,12,0,"Delay"," seconds",2,0.003);
		configParam(WIDTH_PARAM,0.02,1,0.9,"Width");
		configParam(HEIGHT_PARAM,0.02,1,0.9,"Height");
		configParam(FEED_PARAM,0,1,0,"Feedback");
		configParam(DIFFUSE_PARAM,0,10,0,"Diffuse");
		configParam(LOWPASS_PARAM,-9,0,0,"Lowpass");
		configParam(TONE_PARAM,-9,0,-5,"Highpass");
		configParam(CLOSE_PARAM,0,1,1,"Pull");

		configInput(DELAY_CV_INPUT, "Delay CV");
		configInput(WIDTH_CV_INPUT, "Width CV");
		configInput(HEIGHT_CV_INPUT, "Height CV");
		configInput(FEED_CV_INPUT, "Feed CV");
		configInput(DIFF_CV_INPUT, "Diff CV");

		configButton(ALLPASS_BUTTON,"Allpass Filtering");

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

	static constexpr int delayDensity = 50;
	static constexpr int rngSeed = 0;
	bool initialized = false;

	DelayLineDeque<44100*180>* delayLine = new DelayLineDeque<44100*180>;
	DelayLineDeque<44100*180>* feedbackLine = new DelayLineDeque<44100*180>;
	float delayCache[6][4][100];
	float ampCache[6][4][100] = {{{1}}};
	PointList* pointList;
	dsp::RCFilter hp;
	dsp::RCFilter lp;
	float diff;
	float w;
	float h;
	PRNGCache<400, 1000> prng;

	void onAdd(const AddEvent& e) override{
		pointList = new PointList(
			{
				new XYPoint(Vec(-0.6,-0.5),"I",SHP_CIRCLE,nvgRGBf(242.f/255.f, 223.f/255.f, 0.f/255.f)),
				new XYPoint(Vec(0.2,-0.5),"A",SHP_SQUARE,nvgRGBf(255.f/255.f, 101.f/255.f, 38.f/255.f)),
				new XYPoint(Vec(0.6,-0.5),"B",SHP_SQUARE,nvgRGBf(174.f/255.f, 86.f/255.f, 255.f/255.f))
			},
			{
				getParamQuantity(XI_PARAM),
				getParamQuantity(OUTX0_PARAM),
				getParamQuantity(OUTX1_PARAM)
			},
			{
				getParamQuantity(YI_PARAM),
				getParamQuantity(OUTY0_PARAM),
				getParamQuantity(OUTY1_PARAM)
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

	inline float getDelayBounceOutput(DelayLineDeque<44100*180>* dl, int startID, int endID, float delayMult, float w, float h){
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

	inline float getDelayRoomOutput(DelayLineDeque<44100*180>* dl, int id, float delayMult, float w, float h){
		float outx = 0;
		float outy = 0;
		prng.reseedSoft(rngSeed);
		for(int i = 0; i<=delayDensity; i++){
			outx+=dl->readRaw((delayCache[id][0][i])+diff*prng.output[i])*ampCache[id][0][i];
			outx+=dl->readRaw((delayCache[id][1][i])+diff*prng.output[100+i])*ampCache[id][1][i];
			outy+=dl->readRaw((delayCache[id][2][i])+diff*prng.output[200+i])*ampCache[id][2][i];
			outy+=dl->readRaw((delayCache[id][3][i])+diff*prng.output[300+i])*ampCache[id][3][i];
		}
		return (outx/w+outy/h)/delayDensity;
	}

	inline float getDelayDirect(DelayLineDeque<44100*180>* dl,float x1,float y1,float x2, float y2, float delayMult){
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

    void process(const ProcessArgs& args) override {
		if(!initialized&&pointList){
			pointList->readFromParams();
			initialized = true;
		}

		float delayMult = clamp(0.003*dsp::exp2_taylor5(params[DELAY_PARAM].getValue()-inputs[DELAY_CV_INPUT].getVoltage())*args.sampleRate,0,13*44100);
		float a = clamp(params[WIDTH_PARAM].getValue()+inputs[WIDTH_CV_INPUT].getVoltage()/10.f,0.02);
		float b = clamp(params[HEIGHT_PARAM].getValue()+inputs[HEIGHT_CV_INPUT].getVoltage()/10.f,0.02);
		diff = (params[DIFFUSE_PARAM].getValue()+inputs[DIFF_CV_INPUT].getVoltage())*args.sampleRate/1000.f;
		float lpf = dsp::exp2_taylor5(params[LOWPASS_PARAM].getValue())*20000*args.sampleTime;
		float hpf = dsp::exp2_taylor5(params[TONE_PARAM].getValue())*20000*args.sampleTime;
		float closeness = params[CLOSE_PARAM].getValue();
		float fbmult = clamp(params[FEED_PARAM].getValue()+inputs[FEED_CV_INPUT].getVoltage()/10.f);

		w=a;
		h=b;

		float feedback = softClip(feedbackLine->readLerp(delayMult*a)*a+feedbackLine->readLerp(delayMult*b)*b,5);
		outputs[SEND_OUTPUT].setVoltage(feedback);
		feedback = inputs[RECIEVE_INPUT].getNormalVoltage(feedback);
		
		delayLine->write(inputs[AUDIO_INPUT].getVoltage()+feedback*fbmult);
		
		cacheEmitter(params[0].getValue(),params[4].getValue(),a,b,0);
		cacheEmitter(params[11].getValue(),params[13].getValue(),a,b,4);
		cacheEmitter(params[12].getValue(),params[14].getValue(),a,b,5);

		float lDelay = getDistance(params[XI_PARAM].getValue(),params[YI_PARAM].getValue(),params[OUTX0_PARAM].getValue(),params[OUTY0_PARAM].getValue());
		float rDelay = getDistance(params[XI_PARAM].getValue(),params[YI_PARAM].getValue(),params[OUTX1_PARAM].getValue(),params[OUTY1_PARAM].getValue());
		float comp = std::min(lDelay,rDelay)*closeness;
		lDelay -= comp;
		rDelay -= comp;

		float lRoom = getDelayBounceOutput(delayLine,0,4,delayMult,a,b);
		float lDirect = delayLine->readRaw(lDelay*delayMult)/(1+lDelay*lDelay);
		float rRoom = getDelayBounceOutput(delayLine,0,5,delayMult,a,b);
		float rDirect = delayLine->readRaw(rDelay*delayMult)/(1+rDelay*rDelay);

		outputs[AUDIOL_OUTPUT].setVoltage(lRoom+lDirect);
		outputs[AUDIOR_OUTPUT].setVoltage(rRoom+rDirect);

		hp.setCutoffFreq(hpf);
		hp.process(getDelayRoomOutput(delayLine,0,delayMult,a,b));

		lp.setCutoffFreq(lpf);
		lp.process(hp.highpass());

		feedbackLine->write(clamp(lp.lowpass(),-10.f,10.f));
		if(initialized){
			pointList->writeToParams();
		}
	}
};


struct BoxesWidget : ModuleWidget {
	int theme = -1;
	XYScreen* screen;

	BoxesWidget(Boxes* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Ivory.svg",
			getPalette(PAL_BUBBLEGUM)
		));
        //Add widgets here: 
        //addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.62, 10.0)), module, Boxes::DELAY_PARAM));
		addInput(createInputCentered<QPort>(mm2px(Vec(68.580, 88.500)), module, Boxes::AUDIO_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(55.245, 114.5)), module, Boxes::RECIEVE_INPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(68.580, 101.500)), module, Boxes::AUDIOL_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(68.580, 114.500)), module, Boxes::AUDIOR_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(55.245, 101.5)), module, Boxes::SEND_OUTPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.050, 101.500)), module, Boxes::WIDTH_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.050, 114.500)), module, Boxes::HEIGHT_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(19.050, 88.500)), module, Boxes::DELAY_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 101.500)), module, Boxes::WIDTH_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 114.500)), module, Boxes::HEIGHT_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.62, 88.500)), module, Boxes::DELAY_CV_INPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(55.245, 88.500)), module, Boxes::CLOSE_PARAM));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(30.480, 101.500)), module, Boxes::TONE_PARAM));
		addInput(createInputCentered<QPort>(mm2px(Vec(30.480, 88.500)), module, Boxes::FEED_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(30.480, 114.500)), module, Boxes::DIFF_CV_INPUT));

		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(41.910, 101.500)), module, Boxes::LOWPASS_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(41.910, 88.500)), module, Boxes::FEED_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(41.910, 114.500)), module, Boxes::DIFFUSE_PARAM));
		screen = createWidget<XYScreen>(mm2px(Vec(3.810,9.000)));
		screen->setSize(mm2px(Vec(68.580, 68.580)));
		addChild(screen);
		if(module){
			module->pointList->addPointsToScreen(screen);
		}
	}

	void appendContextMenu(Menu* menu) override {
		Boxes* module = getModule<Boxes>();
		
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
		Boxes* module = getModule<Boxes>();
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
		screen->setBoundsClamp(module->w,module->h);
	}
};


Model* modelBoxes = createModel<Boxes, BoxesWidget>("Boxes");