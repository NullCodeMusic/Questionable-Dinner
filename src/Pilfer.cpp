#include "plugin.hpp"
#include <cmath>

struct Pilfer : Module {
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
		LIGHTS_LEN
	};

	Pilfer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ACCEL_PARAM, 1.f, 5.f, 3.f, "Acceleration");
		configParam(FRICT_PARAM, 0.f, 1.f, 0.5f, "Friction");
		configParam(TOLERANCE_PARAM, 0.f, 5.f, 0.f, "Trigger Distance");
		configParam(BOUNCE_PARAM, 0.f, 0.98f, 0.f, "Bounce");
		configParam(DRIVE_PARAM, 0.f, 5.f, 0.f, "Drive");

		configSwitch(HFLF_BUTTON, 0.f,1.f,1.f,"Range",{"Low","High"});
		configSwitch(LINEXP_BUTTON, 0.f,1.f,1.f,"Friction Mode",{"B","A"});
		configInput(ACC_CV_INPUT, "Acceleration CV");
		configInput(FRI_CV_INPUT, "Friction CV");
		configInput(TOL_CV_INPUT, "Distance CV");
		configInput(BOU_CV_INPUT, "Bounce CV");
		configInput(AUDIO_X_INPUT, "Target X In");
		configOutput(AUDIO_X_OUTPUT, "X Position Out");
		configInput(AUDIO_Y_INPUT, "Target Y In");
		configOutput(AUDIO_Y_OUTPUT, "Y Position Out");
		configBypass(AUDIO_X_INPUT,AUDIO_X_OUTPUT);
		configBypass(AUDIO_Y_INPUT,AUDIO_Y_OUTPUT);
		configInput(SHAKE_CV_INPUT, "Shake In");
		configInput(DRI_CV_INPUT, "Drive CV");

		configParam(ACC_CV_PARAM, -1.f, 1.f, 0.f, "Acceleration CV");
		configParam(FRI_CV_PARAM, -1.f, 1.f, 0.f, "Friction CV");
		configParam(TOL_CV_PARAM, -1.f, 1.f, 0.f, "Distance CV");
		configParam(BOU_CV_PARAM, -1.f, 1.f, 0.f, "Bounce CV");
		configParam(DRI_CV_PARAM, -1.f, 1.f, 0.f, "Drive CV");
		configParam(SHAKE_CV_PARAM, -1.f, 1.f, 0.f, "Shake Amount");
	}
	
	float pos_x = 0;
	float pos_y = 0;
	float vel_x = 0;
	float vel_y = 0;
	int mode = 0;

	void process(const ProcessArgs& args) override {

		float delta = args.sampleTime;

		float acceleration = pow(10,params[ACCEL_PARAM].getValue()) * 
			pow(2,inputs[ACC_CV_INPUT].getVoltage() * params[ACC_CV_PARAM].getValue());
		float friction = clamp(	
			pow(params[FRICT_PARAM].getValue(),4) + //Main
			inputs[FRI_CV_INPUT].getVoltage() * params[FRI_CV_PARAM].getValue() / 10, //CV
			0.f,1.f); //Min/Max
		float bounce = clamp(	
			params[BOUNCE_PARAM].getValue() + //Main
			inputs[BOU_CV_INPUT].getVoltage() * params[BOU_CV_PARAM].getValue() / 10, //CV
			0.f,1.f); //Min/Max
		float tolerance = clamp(	
			params[TOLERANCE_PARAM].getValue() + //Main
			inputs[TOL_CV_INPUT].getVoltage() * params[TOL_CV_PARAM].getValue() / 2, //CV
			0.f,10.f); //Min/Max
		float drive = clamp(	
			params[DRIVE_PARAM].getValue() + //Main
			inputs[DRI_CV_INPUT].getVoltage() * params[DRI_CV_PARAM].getValue() / 2, //CV
			0.f,10.f); //Min/Max
		float hflf = params[HFLF_BUTTON].getValue();
		float shake = inputs[SHAKE_CV_INPUT].getVoltage()*params[SHAKE_CV_PARAM].getValue();

		
		if(hflf<1){
			acceleration/=100;
			tolerance/=60;
		}

		switch (mode)
		{
		case 1:
				pilfer1d(delta,drive,tolerance,acceleration,friction,shake,bounce);
				outputs[AUDIO_Y_OUTPUT].setVoltage(inputs[AUDIO_X_INPUT].getVoltage());
			break;
		
		case 2:
				pilfer2d(delta,drive,tolerance,acceleration,friction,shake,bounce);
			break;
		
		default:
				outputs[AUDIO_X_OUTPUT].setVoltage(0);
				outputs[AUDIO_Y_OUTPUT].setVoltage(0);
			break;
		}

		outputs[VEL_OUTPUT].setVoltage(magnitude(vel_x,vel_y));
		lights[VELOCITY_LIGHT].setBrightness(magnitude(vel_x,vel_y));
	}

	template <typename T> int sign(T val) {
    	return (T(0) < val) - (val < T(0));
	}

	void pilfer1d(float delta, float drive, float tolerance, float acceleration, float friction, float shake, float bounce){
		float audio = inputs[AUDIO_X_INPUT].getVoltage()*(1+drive);

			//Will I move?
			float diff = audio-pos_x;
			float move = sign(diff)*(abs(diff)>tolerance)*acceleration*delta;
			move += shake*acceleration*delta;
			//Update Velocity
			vel_x += move;

			//Move
			pos_x += vel_x;

			//Bounce
			if(abs(pos_x)>=10&&sign(vel_x)==sign(pos_x)){
				lights[BOUNCING_LIGHT].setBrightness(1);
				vel_x = -vel_x*bounce;
			}
			//Friction
			if(params[LINEXP_BUTTON].getValue()<1){
				vel_x-=sign(vel_x)*std::min(abs(vel_x),friction*acceleration*delta);
			}else{
				vel_x/=1+friction;
			}

			//Output
			float out = 0;
				out = pos_x/(1+drive);
				out = tanh(out/12)*12;

			outputs[AUDIO_X_OUTPUT].setVoltage(out);

			//Decay Lights
			lights[BOUNCING_LIGHT].setBrightness(lights[BOUNCING_LIGHT].getBrightness()/1.0005f);
	}

	void pilfer2d(float delta, float drive, float tolerance, float acceleration, float friction, float shake, float bounce){
		float aud_x = inputs[AUDIO_X_INPUT].getVoltage()*(1+drive);
		float aud_y = inputs[AUDIO_Y_INPUT].getVoltage()*(1+drive);

		//Will I move?
		std::vector<float> diff = dir_dist(pos_x,pos_y,aud_x,aud_y);
		float move = (diff[2]>tolerance)*acceleration*delta;
		move += shake*acceleration*delta;
		float dir_x = diff[0];
		float dir_y = diff[1];

		//Update velocity
		vel_x += dir_x*move;

		vel_y += dir_y*move;

		//Move
		pos_x += vel_x;
		pos_y += vel_y;

		//Bounce - Need to figure out algo so empty for now
		if(magnitude(pos_x,pos_y)>=10){
			lights[BOUNCING_LIGHT].setBrightness(1);
			vel_x = -vel_x*bounce;
			vel_y = -vel_y*bounce;
		}

		//Friction
		if(params[LINEXP_BUTTON].getValue()<1){
			vel_x-=sign(vel_x)*std::min(abs(vel_x),friction*acceleration*delta);
			vel_y-=sign(vel_y)*std::min(abs(vel_y),friction*acceleration*delta);
		}else{
			vel_x/=1+friction;
			vel_y/=1+friction;
		}

		//Output
		
		outputs[AUDIO_X_OUTPUT].setVoltage(tanh(pos_x/12/(1+drive))*12);
		outputs[AUDIO_Y_OUTPUT].setVoltage(tanh(pos_y/12/(1+drive))*12);

		//Decay Lights
		lights[BOUNCING_LIGHT].setBrightness(lights[BOUNCING_LIGHT].getBrightness()/1.0005f);
	}

	std::vector<float> dir_dist(float x1, float y1, float x2, float y2){
		float x = x2 - x1;
		float y = y2 - y1;
		float mag = magnitude(x,y);
		if(mag == 0){
			return {0,0,0};
		}
		return {x/mag , y/mag, mag};
	}
	
	float magnitude(float x, float y){
		float out = sqrt(pow(x,2)+pow(y,2));
		if(std::isnan(out)){
			return 0;
		}
		return out;
	}

	void onPortChange(const PortChangeEvent& e) override{
		if(inputs[AUDIO_Y_INPUT].isConnected()){
			mode = 2;
			return;
		}
		if(inputs[AUDIO_X_INPUT].isConnected()||inputs[SHAKE_CV_INPUT].isConnected()){
			mode = 1;
			return;
		}
		mode = 0;
	}
};


struct PilferWidget : ModuleWidget {
	PilferWidget(Pilfer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/qd-002/Pilfer.svg")));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.156, 10.421)), module, Pilfer::ACCEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.156, 20)), module, Pilfer::DRIVE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.709, 10.628)), module, Pilfer::FRICT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.871, 10.357)), module, Pilfer::TOLERANCE_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(49.871, 20)), module, Pilfer::HFLF_BUTTON));
		addParam(createParamCentered<CKSS>(mm2px(Vec(49.871, 30)), module, Pilfer::LINEXP_BUTTON));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.73, 10.683)), module, Pilfer::BOUNCE_PARAM));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.456, 35.284)), module, Pilfer::SHAKE_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.456, 46.284)), module, Pilfer::ACC_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(23.009, 46.491)), module, Pilfer::FRI_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.171, 46.22)), module, Pilfer::TOL_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.03, 60.546)), module, Pilfer::DRI_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(36.03, 46.546)), module, Pilfer::BOU_CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.412, 80.276)), module, Pilfer::ACC_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.965, 80.483)), module, Pilfer::FRI_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.127, 80.212)), module, Pilfer::TOL_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.985, 80.538)), module, Pilfer::BOU_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.73, 20.683)), module, Pilfer::DRI_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.709, 20.628)), module, Pilfer::SHAKE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.789, 108.847)), module, Pilfer::AUDIO_X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.789, 118.847)), module, Pilfer::AUDIO_Y_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.291, 109.069)), module, Pilfer::AUDIO_X_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.291, 119.069)), module, Pilfer::AUDIO_Y_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40, 115.069)), module, Pilfer::VEL_OUTPUT));

		addChild(createLightCentered<MediumSimpleLight<WhiteLight>>(mm2px(Vec(53.866, 97.021)), module, Pilfer::VELOCITY_LIGHT));
		addChild(createLightCentered<MediumSimpleLight<WhiteLight>>(mm2px(Vec(54.19, 105.769)), module, Pilfer::BOUNCING_LIGHT));
	}
	void appendContextMenu(Menu* menu) override {
		Pilfer* module = getModule<Pilfer>();

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuItem("Mode",std::to_string(module->mode)+"d"));
	}
};


Model* modelPilfer = createModel<Pilfer, PilferWidget>("Pilfer");