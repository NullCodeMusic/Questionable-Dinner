//Module Slug PlasticWrap - find and replace this to make a new one

#include "plugin.hpp"
#include "mymath.hpp"

inline float clipFunction(float v, float k){
	float vr = abs(v);
	float vs = sign(v);
	float a = std::max(vr-k,0.f)*vs/(1-k);
	float b = std::min(vr,k)*vs;
	return tanh(a)*(1-k)+b;
}
inline float clipA(float v, float k){
	float vr = abs(v);
	float vs = sign(v);
	return std::max(vr-k,0.f)*vs/(1-k);
}

struct PlasticWrap : Module {
	int theme = -1;
	
	enum ParamId {
		THRESH_SWITCH,
		MIX_PARAM,
		WRAP_PARAM,
		KNEE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		L_IN,
		R_IN,
		INPUTS_LEN
	};
	enum OutputId {
		L_OUT,
		R_OUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=44
	};
	
	PlasticWrap() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configSwitch(THRESH_SWITCH,0,1,0,"Threshold",{"5v","10v"});
		configParam(MIX_PARAM,0,1,1,"Mix","%",0,100);
		configParam(KNEE_PARAM,0,0.9999,0.5,"Hardness","%",0,100);
		configParam(WRAP_PARAM,0,1,0,"Plastic Wrap","%",0,100);
		configBypass(L_IN,L_OUT);
		configBypass(R_IN,R_OUT);
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

	dsp::RCFilter wrapFilter;
	float env[2] = {0};
	float thresh;

    void process(const ProcessArgs& args) override {
		float wrap = params[WRAP_PARAM].getValue();
		float knee = params[KNEE_PARAM].getValue();
		thresh = params[THRESH_SWITCH].getValue()*5.f+5.f;
		float mix = params[MIX_PARAM].getValue();

		float vabs = inputs[R_IN].isConnected() ? (abs(inputs[L_IN].getVoltageSum())+abs(inputs[R_IN].getVoltageSum()))/2 : (abs(inputs[L_IN].getVoltageSum()));
		
		env[0] = std::max(vabs,exp2Decay(env[0],0.4,args.sampleRate));

		wrapFilter.setCutoffFreq(150.f*args.sampleTime);
		wrapFilter.process(vabs);
		thresh = clamp(thresh*(1-wrap)+wrapFilter.lowpass()*wrap,0.1f,10.f);

		float l = inputs[L_IN].getVoltageSum();
		float r = inputs[R_IN].isConnected() ? inputs[R_IN].getVoltageSum() : inputs[L_IN].getVoltageSum();

		l = clipFunction(l/thresh, knee)*thresh*mix+l*(1-mix);
		r = clipFunction(r/thresh, knee)*thresh*mix+l*(1-mix);

		env[1] = std::max((abs(l)+abs(r))/2,exp2Decay(env[1],0.4,args.sampleRate));

		outputs[L_OUT].setVoltage(l);
		outputs[R_OUT].setVoltage(r);
	}
};


struct PlasticWrapWidget : ModuleWidget {
	int theme = -1;

	PlasticWrapWidget(PlasticWrap* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Plastic.svg",
			getPalette(PAL_LIGHT)
		));
        //Add widgets here: 
        addParam(createParamCentered<CKSS>(mm2px(Vec(5.08, 38)), module, PlasticWrap::THRESH_SWITCH));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(5.08, 51.5)), module, PlasticWrap::MIX_PARAM));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(12.700, 71)), module, PlasticWrap::KNEE_PARAM));
		addParam(createParamCentered<QKnob6mm>(mm2px(Vec(19.050, 89)), module, PlasticWrap::WRAP_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(6.985, 99.500)), module, PlasticWrap::L_IN));
		addInput(createInputCentered<QPort>(mm2px(Vec(18.415, 99.500)), module, PlasticWrap::R_IN));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(6.985, 114.500)), module, PlasticWrap::L_OUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(18.415, 114.500)), module, PlasticWrap::R_OUT));

		for(int i = 0;i < 11;i++){
			addChild(createLightCentered<SmallSimpleLight<GreenRedLight>>(mm2px(Vec(12.295, 11.500+4.5*i)), module, i*2));
			addChild(createLightCentered<SmallSimpleLight<GreenRedLight>>(mm2px(Vec(20.725, 11.500+4.5*i)), module, i*2+22));
		}
	}

	void appendContextMenu(Menu* menu) override {
		PlasticWrap* module = getModule<PlasticWrap>();
		
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
		PlasticWrap* module = getModule<PlasticWrap>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Plastic.svg",
				getPalette(theme)
			));
		}
		float knee = module->params[PlasticWrap::KNEE_PARAM].getValue();
		float thresh = 1/(module->thresh/10);
		float mix = module->params[PlasticWrap::MIX_PARAM].getValue();
		for(int i = 0; i <11;i++){
			float fi = clamp((1.f-float(i)/10.f)*thresh);
			module->lights[i*2+23].setBrightness(clipA(fi,knee)*mix);
			module->lights[i*2+22].setBrightness(1.f-clipA(fi,knee)*mix);
			fi = (1.f-float(i)/10.f)*10.f;
			if(module->env[0]>0.01 || module->env[1]>0.01){
				module->lights[i*2+1].setBrightness((module->env[0] >= fi)&&(fi >= module->env[1]));
				module->lights[i*2].setBrightness(module->env[1] >= fi);
			}
		}
	}
};


Model* modelPlasticWrap = createModel<PlasticWrap, PlasticWrapWidget>("PlasticWrap");