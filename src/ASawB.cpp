#include "plugin.hpp"

struct ASawB : Module {
	enum ParamId {
		BIAS_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LA_INPUT,
		LB_INPUT,
		SA_INPUT,
		SB_INPUT,
		MA_INPUT,
		MB_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LLESS_OUTPUT,
		LGREATER_OUTPUT,
		LEQUAL_OUTPUT,
		LUNEQUAL_OUTPUT,
		SOVER_OUTPUT,
		SUNDER_OUTPUT,
		MOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	ASawB() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BIAS_PARAM, 0.f, 5.f, 0.f, "Bias");
		configInput(LA_INPUT, "Logic A");
		configInput(LB_INPUT, "Logic B");
		configInput(SA_INPUT, "Split A");
		configInput(SB_INPUT, "Split B");
		configInput(MA_INPUT, "Minus A");
		configInput(MB_INPUT, "Minus B");
		configOutput(LLESS_OUTPUT, "A is less than B (-Bias)");
		configOutput(LGREATER_OUTPUT, "A is greater than B (+Bias)");
		configOutput(LEQUAL_OUTPUT, "A is equal to B (±Bias)");
		configOutput(LUNEQUAL_OUTPUT, "A is not equal to B (±Bias)");
		configOutput(SOVER_OUTPUT, "A split by B over");
		configOutput(SUNDER_OUTPUT, "A split by B under");
		configOutput(MOUT_OUTPUT, "A minus B");
	}

	void process(const ProcessArgs& args) override {

		float bias = params[BIAS_PARAM].getValue();

		//LOGIC

		//In
		float a = inputs[LA_INPUT].getNormalVoltage(0.f);
		float b = inputs[LB_INPUT].getNormalVoltage(0.f);

		//Out
		outputs[LLESS_OUTPUT].setVoltage((a+bias<b)*10.f);
		outputs[LGREATER_OUTPUT].setVoltage((a-bias>b)*10.f);

		outputs[LEQUAL_OUTPUT].setVoltage((abs(a-b)<=bias)*10.f);
		outputs[LUNEQUAL_OUTPUT].setVoltage((abs(a-b)>bias)*10.f);

		//SPLIT

		a = inputs[SA_INPUT].getNormalVoltage(a);
		b = inputs[SB_INPUT].getNormalVoltage(b);

		outputs[SOVER_OUTPUT].setVoltage(std::max(bias,a-b)-bias);
		outputs[SUNDER_OUTPUT].setVoltage(abs(std::min(-bias,a-b)+bias));

		//SUBTRACT
		a = inputs[MA_INPUT].getNormalVoltage(a);
		b = inputs[MB_INPUT].getNormalVoltage(b);

		outputs[MOUT_OUTPUT].setVoltage(a-b);

	}
};


struct ASawBWidget : ModuleWidget {
	ASawBWidget(ASawB* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ASawB.svg")));

		/*
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		*/

		addParam(createParamCentered<WinterKnob>(mm2px(Vec(18.197, 11.595)), module, ASawB::BIAS_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 25.5)), module, ASawB::LA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.78, 25.5)), module, ASawB::LB_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 68)), module, ASawB::SA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.78, 68)), module, ASawB::SB_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 98)), module, ASawB::MA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.78, 98)), module, ASawB::MB_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 38)), module, ASawB::LLESS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 38)), module, ASawB::LGREATER_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 50.5)), module, ASawB::LEQUAL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 50.5)), module, ASawB::LUNEQUAL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 80.5)), module, ASawB::SOVER_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 80.5)), module, ASawB::SUNDER_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.78, 111)), module, ASawB::MOUT_OUTPUT));
	}
};


Model* modelASawB = createModel<ASawB, ASawBWidget>("ASawB");