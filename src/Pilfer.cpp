#include "plugin.hpp"
#include <cmath>

struct Pilfer : Module {
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
		ACCEL_PARAM,
		FRICT_PARAM,
		TOLERANCE_PARAM,
		BOUNCE_PARAM,
		ACC_CV_PARAM,
		FRI_CV_PARAM,
		TOL_CV_PARAM,
		BOU_CV_PARAM,
		DRI_CV_PARAM,
		SHAKE_CV_PARAM,
		DRIVE_PARAM,
		HFLF_BUTTON,
		LINEXP_BUTTON,
		RECTPOL_BUTTON,
		OVERSAMPLE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ACC_CV_INPUT,
		FRI_CV_INPUT,
		TOL_CV_INPUT,
		BOU_CV_INPUT,
		DRI_CV_INPUT,
		SHAKE_CV_INPUT,
		AUDIO_X_INPUT,
		AUDIO_Y_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_X_OUTPUT,
		AUDIO_Y_OUTPUT,
		VEL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		VELOCITY_LIGHT,
		BOUNCING_LIGHT,
		D1_LIGHT,
		D2_LIGHT,
		LIGHTS_LEN
	};

	Pilfer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ACCEL_PARAM, 1.f, 19.5f, 10.f, "Acceleration","v/sec",2.f);
		configParam(FRICT_PARAM, 0.f, 1.f, 0.5f, "Friction");
		configParam(TOLERANCE_PARAM, 0.f, 5.f, 0.f, "Trigger Distance");
		configParam(BOUNCE_PARAM, 0.f, 0.98f, 0.f, "Bounce","%",0.f,100.0f);
		configParam(DRIVE_PARAM, 0.f, 5.f, 0.f, "Drive");
		configParam(OVERSAMPLE_PARAM, 1, 16, 4, "Oversampling", "x");

		configSwitch(HFLF_BUTTON, 0.f,1.f,1.f,"Range",{"Low","High"});
		configSwitch(LINEXP_BUTTON, 0.f,1.f,1.f,"Friction",{"Linear","Exponential"});
		configSwitch(RECTPOL_BUTTON, 0.f,1.f,1.f,"Coordinates",{"Rect","Polar"});
		configInput(ACC_CV_INPUT, "Acceleration CV");
		configInput(FRI_CV_INPUT, "Friction CV");
		configInput(TOL_CV_INPUT, "Distance CV");
		configInput(BOU_CV_INPUT, "Bounce CV");
		configInput(AUDIO_X_INPUT, "Target X/R");
		configOutput(AUDIO_X_OUTPUT, "X Position");
		configInput(AUDIO_Y_INPUT, "Target Y/θ");
		configOutput(AUDIO_Y_OUTPUT, "Y Position");
		configOutput(VEL_OUTPUT, "Velocity");
		configBypass(AUDIO_X_INPUT,AUDIO_X_OUTPUT);
		configBypass(AUDIO_Y_INPUT,AUDIO_Y_OUTPUT);
		configInput(SHAKE_CV_INPUT, "Shake");
		configInput(DRI_CV_INPUT, "Drive CV");

		configParam(ACC_CV_PARAM, -1.f, 1.f, 0.f, "Acceleration CV");
		configParam(FRI_CV_PARAM, -1.f, 1.f, 0.f, "Friction CV");
		configParam(TOL_CV_PARAM, -1.f, 1.f, 0.f, "Distance CV");
		configParam(BOU_CV_PARAM, -1.f, 1.f, 0.f, "Bounce CV");
		configParam(DRI_CV_PARAM, -1.f, 1.f, 0.f, "Drive CV");
		configParam(SHAKE_CV_PARAM, -1.f, 1.f, 0.f, "Shake Amount");
	}
	
	float pos_x[16] = {};
	float pos_y[16] = {};
	float vel_x[16] = {};
	float vel_y[16] = {};
	int mode = 0;
	float paramsum=0;
	std::vector<float> baseparams = {0,0,0,0,0};
	std::vector<float> cvparams = {0,0,0,0,0};
	float acceleration;
	float friction;
	float bounce;
	float tolerance;
	float drive;

	void process(const ProcessArgs& args) override {

		float delta = args.sampleTime;
		float hflf = params[HFLF_BUTTON].getValue();
		float rectpol = params[RECTPOL_BUTTON].getValue();

		int channels = std::max(std::max(inputs[AUDIO_X_INPUT].getChannels(),
		inputs[AUDIO_Y_INPUT].getChannels()),
		std::max(inputs[SHAKE_CV_INPUT].getChannels(),1));

		int oversampling = params[OVERSAMPLE_PARAM].getValue();

		float outvel;

		for(int i = 0;i<channels;i++)
		{
			if(filterParams(i)){
				acceleration = baseparams[0] * cvparams[0];
				friction = clamp(baseparams[1] + cvparams[1] + 1, 1.f,2.f);
				bounce = clamp(baseparams[2] + cvparams[2], 0.f,1.f); 
				tolerance = clamp(baseparams[3] + cvparams[3], 0.f,10.f); 
				drive = clamp(baseparams[4] + cvparams[4], 0.f,10.f);
				friction = pow(friction,args.sampleRate/48000/oversampling);

				if(hflf<1){
				acceleration/=100;
				tolerance/=60;
				}
			}

			float shake = inputs[SHAKE_CV_INPUT].getVoltage(i)*params[SHAKE_CV_PARAM].getValue();
			
			float x;
			float y;
			if(rectpol<1||mode==1){
				x = inputs[AUDIO_X_INPUT].getVoltage(i)*(1+drive);
				y = inputs[AUDIO_Y_INPUT].getVoltage(i)*(1+drive);
			}else{
				float r = inputs[AUDIO_X_INPUT].getVoltage(i)*(1+drive);
				float t = inputs[AUDIO_Y_INPUT].getVoltage(i)*(1+drive);
				x = cosf((t/5+0.25)*3.1415)*r;
				y = sinf((t/5+0.25)*3.1415)*r;
			}

			switch (mode)
			{
			case 1:
				for(int r = 0;r < oversampling;r++){
					pilfer1d(i,x,delta/oversampling,drive,tolerance,acceleration/oversampling,friction,shake,bounce);
				}
				outputs[AUDIO_X_OUTPUT].setVoltage(tanh(pos_x[i]/12/(1+drive))*12, i);
				outputs[AUDIO_Y_OUTPUT].setVoltage(0,i);
				break;
			
			case 2:
				for(int r = 0;r < oversampling;r++){
					pilfer2d(i,x,y,delta/oversampling,drive,tolerance,acceleration/oversampling,friction,shake,bounce);
				}
				outputs[AUDIO_X_OUTPUT].setVoltage(tanh(pos_x[i]/12/(1+drive))*12, i);
				outputs[AUDIO_Y_OUTPUT].setVoltage(tanh(pos_y[i]/12/(1+drive))*12, i);
				break;

			default:
					outputs[AUDIO_X_OUTPUT].setVoltage(0,i);
					outputs[AUDIO_Y_OUTPUT].setVoltage(0,i);
				break;
			}

			outvel = magnitude(vel_x[i],vel_y[i]);
			outputs[VEL_OUTPUT].setVoltage(outvel,i);
		}

		outputs[AUDIO_X_OUTPUT].setChannels(channels);
		outputs[AUDIO_Y_OUTPUT].setChannels(channels);
		outputs[VEL_OUTPUT].setChannels(channels);

		lights[BOUNCING_LIGHT].setBrightness(lights[BOUNCING_LIGHT].getBrightness()/1.0005f);
		lights[VELOCITY_LIGHT].setBrightness(outvel);
	}

	bool filterParams(int idx){
		bool updateparams;
		//MAIN
		float nextsum = params[ACCEL_PARAM].getValue() +
		params[FRICT_PARAM].getValue() +
		params[BOUNCE_PARAM].getValue() +
		params[TOLERANCE_PARAM].getValue() +
		params[DRIVE_PARAM].getValue() +
		params[HFLF_BUTTON].getValue();

		if(abs(nextsum-paramsum)>=0.0001){
			baseparams[0] = dsp::exp2_taylor5(params[ACCEL_PARAM].getValue());
			baseparams[1] = pow(params[FRICT_PARAM].getValue(),4);
			baseparams[2] = params[BOUNCE_PARAM].getValue();
			baseparams[3] = params[TOLERANCE_PARAM].getValue();
			baseparams[4] = params[DRIVE_PARAM].getValue();

			updateparams = true;
		}

		//CVS
		if(inputs[ACC_CV_INPUT].isConnected()){
			cvparams[0] = dsp::exp2_taylor5(inputs[ACC_CV_INPUT].getVoltage(idx) * params[ACC_CV_PARAM].getValue());

			updateparams = true;
		}else{
			cvparams[0] = 1;
		}

		if(inputs[FRI_CV_INPUT].isConnected()){
			cvparams[1] = inputs[FRI_CV_INPUT].getVoltage(idx) * params[FRI_CV_PARAM].getValue() / 10;

			updateparams = true;
		}else{
			cvparams[1] = 0;
		}

		if(inputs[BOU_CV_INPUT].isConnected()){
			cvparams[2] = inputs[BOU_CV_INPUT].getVoltage(idx) * params[BOU_CV_PARAM].getValue() / 10;

			updateparams = true;
		}else{
			cvparams[2] = 0;
		}

		if(inputs[TOL_CV_INPUT].isConnected()){
			cvparams[3] = inputs[TOL_CV_INPUT].getVoltage(idx) * params[TOL_CV_PARAM].getValue() / 2;

			updateparams = true;
		}else{
			cvparams[3] = 0;
		}

		if(inputs[DRI_CV_INPUT].isConnected()){
			cvparams[4] = inputs[DRI_CV_INPUT].getVoltage(idx) * params[DRI_CV_PARAM].getValue() / 2;

			updateparams = true;
		}else{
			cvparams[4] = 0;
		}

		return updateparams;
	}
	
	template <typename T> int sign(T val) {
    	return (T(0) < val) - (val < T(0));
	}

	void pilfer1d(int channel, float audio,float delta, float drive, float tolerance, float acceleration, float friction, float shake, float bounce){

			//Will I move?
			float diff = audio-pos_x[channel];
			float move = sign(diff)*(abs(diff)>tolerance)*acceleration*delta;
			move += shake*acceleration*delta*0.25;
			//Update Velocity
			vel_x[channel] += move;

			//Move
			pos_x[channel] += vel_x[channel];

			//Bounce
			if(abs(pos_x[channel])>=10&&sign(vel_x[channel])==sign(pos_x[channel])){
				lights[BOUNCING_LIGHT].setBrightness(bounce);
				vel_x[channel] = -vel_x[channel]*bounce;
			}
			//Friction
			if(params[LINEXP_BUTTON].getValue()<1){
				vel_x[channel]-=sign(vel_x[channel])*std::min(abs(vel_x[channel]),(friction-1)*acceleration*delta);
			}else{
				vel_x[channel]/=friction;
			}
	}

	void pilfer2d(int channel, float aud_x, float aud_y, float delta, float drive, float tolerance, float acceleration, float friction, float shake, float bounce){

		//Will I move?
		float dir_x = aud_x-pos_x[channel];
		float dir_y = aud_y-pos_y[channel];
		float diff = magnitude(dir_x,dir_y);
		if(diff!=0){
			dir_x/=diff;
			dir_y/=diff;
		}else{
			dir_x = 0;
			dir_y = 0;
		}
		float move = (diff>tolerance)*acceleration*delta;
		move += shake*acceleration*delta*0.25; 

		//Update velocity
		vel_x[channel] += dir_x*move;
		vel_y[channel] += dir_y*move;

		//Move
		pos_x[channel] += vel_x[channel];
		pos_y[channel] += vel_y[channel];

		//Bounce - Need to figure out algo so empty for now
		if(magnitude(pos_x[channel],pos_y[channel])>=10){
			lights[BOUNCING_LIGHT].setBrightness(bounce);
			vel_x[channel] = -vel_x[channel]*bounce;
			vel_y[channel] = -vel_y[channel]*bounce;
		}

		//Friction
		if(params[LINEXP_BUTTON].getValue()<1){
			vel_x[channel]-=sign(vel_x[channel])*std::min(abs(vel_x[channel]),(friction-1)*acceleration*delta);
			vel_y[channel]-=sign(vel_y[channel])*std::min(abs(vel_y[channel]),(friction-1)*acceleration*delta);
		}else{
			vel_x[channel]/=friction;
			vel_y[channel]/=friction;
		}

	}
	
	float magnitude(float x, float y){
		float out = sqrt(x*x+y*y);
		if(std::isnan(out)){
			return 0;
		}
		return out;
	}

	void onPortChange(const PortChangeEvent& e) override{

		if(inputs[AUDIO_Y_INPUT].isConnected()){
			mode = 2;
			lights[D1_LIGHT].setBrightness(0.f);
			lights[D2_LIGHT].setBrightness(1.f);
			return;
		}
		if(inputs[AUDIO_X_INPUT].isConnected()||inputs[SHAKE_CV_INPUT].isConnected()){
			mode = 1;
			lights[D1_LIGHT].setBrightness(1.f);
			lights[D2_LIGHT].setBrightness(0.f);
			return;
		}
		mode = 0;
		lights[D1_LIGHT].setBrightness(0.f);
		lights[D2_LIGHT].setBrightness(0.f);
	}
};


struct PilferWidget : ModuleWidget {
	int theme = -1;
	std::string panelpaths[3] = {
		"res/qd-002/Pilfer.svg",
		"res/qd-002/PilferMinDark.svg",
		"res/qd-002/PilferMinLight.svg"
	};
	PilferWidget(Pilfer* module) {
		setModule(module);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/qd-002/Pilfer.svg")));

		addParam(createParam<QSegParam>(mm2px(Vec(48.25, 9.00)), module, Pilfer::OVERSAMPLE_PARAM));

		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(16.51, 29.50)), module, Pilfer::ACCEL_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(38.735, 50.50)), module, Pilfer::DRIVE_PARAM));
		addParam(createParamCentered<QKnob18mm>(mm2px(Vec(54.60, 29.50)), module, Pilfer::FRICT_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(18.46, 56.50)), module, Pilfer::TOLERANCE_PARAM));
		addParam(createParamCentered<QKnob10mm>(mm2px(Vec(52.66, 56.50)), module, Pilfer::BOUNCE_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(29.67, 18.00)), module, Pilfer::HFLF_BUTTON));
		addParam(createParamCentered<CKSS>(mm2px(Vec(41.45, 18.00)), module, Pilfer::LINEXP_BUTTON));
		addParam(createParamCentered<CKSS>(mm2px(Vec(35.56, 35.5)), module, Pilfer::RECTPOL_BUTTON));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 116.50)), module, Pilfer::SHAKE_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.16, 76.00)), module, Pilfer::ACC_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(60.96, 76.00)), module, Pilfer::FRI_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(22.86, 76.00)), module, Pilfer::TOL_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(35.56, 76.00)), module, Pilfer::DRI_CV_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.26, 76.00)), module, Pilfer::BOU_CV_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(10.16, 93.00)), module, Pilfer::ACC_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(60.96, 93.00)), module, Pilfer::FRI_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(22.86, 91.00)), module, Pilfer::TOL_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(48.26, 91.00)), module, Pilfer::BOU_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(35.56, 90.00)), module, Pilfer::DRI_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(35.56, 102.5)), module, Pilfer::SHAKE_CV_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(10.16, 111.00)), module, Pilfer::AUDIO_X_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(22.86, 111.00)), module, Pilfer::AUDIO_Y_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(48.26, 111.00)), module, Pilfer::AUDIO_X_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(60.96, 111.00)), module, Pilfer::AUDIO_Y_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(54.40, 101.80)), module, Pilfer::VEL_OUTPUT));

		addChild(createLightCentered<SmallSimpleLight<RedLight>>(mm2px(Vec(35.56, 15.75)), module, Pilfer::VELOCITY_LIGHT));
		addChild(createLightCentered<SmallSimpleLight<RedLight>>(mm2px(Vec(35.56, 20.25)), module, Pilfer::BOUNCING_LIGHT));
		addChild(createLightCentered<SmallSimpleLight<BlueLight>>(mm2px(Vec(22.83, 102.50)), module, Pilfer::D1_LIGHT));
		addChild(createLightCentered<SmallSimpleLight<YellowLight>>(mm2px(Vec(27.96, 105.00)), module, Pilfer::D2_LIGHT));

		
	}
	void appendContextMenu(Menu* menu) override {
		Pilfer* module = getModule<Pilfer>();

		menu->addChild(new MenuSeparator);
	
		menu->addChild(createIndexSubmenuItem(
			"Panel Theme", 
			{"Main","Min Dark","Min Light"},	
			[=](){
				return module->getTheme();
			},
			[=](int newTheme) {
				module->setTheme(newTheme);
			}
		));
	}
	void step() override {
		ModuleWidget::step();
		Pilfer* module = getModule<Pilfer>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createPanel(asset::plugin(pluginInstance,panelpaths[theme])));
		}
	}
};


Model* modelPilfer = createModel<Pilfer, PilferWidget>("Pilfer");