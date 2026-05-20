//Module Slug Notable - find and replace this to make a new one

#include "plugin.hpp"

struct VoiceManager {
	bool gates[16] = {false};
	float pitches[16] = {0};
	int ages[16] = {0};
	int map[16] = {0};
	int current;
	int voices=1;
	int getOldest(int tick){
		int max = 0;
		int out = 0;
		for(int i=0; i<voices; i++){
			if(ages[i]>max){
				max = ages[i];
				out = i;
			}
			ages[i]+=tick;
		}
		return out;
	}
	int getYoungest(int tick){
		int min = 0;
		int out = 0;
		for(int i=0; i<voices; i++){
			if(ages[i]<min&&gates[i]){
				min = ages[i];
				out = i;
			}
			ages[i]+=tick;
		}
		return out;
	}
	int getOldest(){
		int max = 0;
		int out = 0;
		for(int i=0; i<voices; i++){
			if(ages[i]>max){
				max = ages[i];
				out = i;
			}
		}
		return out;
	}
	int getYoungest(){
		int min = 100;
		int out = 0;
		for(int i=0; i<voices; i++){
			if(ages[i]<min&&gates[i]){
				min = ages[i];
				out = i;
			}
		}
		return out;
	}
	void sendNote(float pitch,int mapVoice){
		int oldest = getOldest(1);
		pitches[oldest] = pitch;
		gates[oldest] = true;
		ages[oldest] = 0;
		map[mapVoice] = oldest;
	}
	void sendStop(int mapVoice){
		int voice = map[mapVoice];
		gates[voice] = false;
		//ages[voice]++;
	}
	float getPitch(int voice){
		return pitches[voice];
	}
	float getGate(int voice){
		return gates[voice]*10.f;
	}
};

struct Notable : Module {
	int theme = -1;
	
	enum ParamId {
		MIN_PARAM,
		MAX_PARAM,
		MEMORY_PARAM,
		POLY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		GATE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VOCT_OUTPUT,
		GATE_OUTPUT,
		GARBAGE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	Notable() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MIN_PARAM, -5.f, 5.f, -1.5f, "Wrap Min");
		configParam(MAX_PARAM, -5.f, 5.f, 1.5f, "Wrap Max");
		configParam(POLY_PARAM, 1.f, 16.f, 1.f, "Output Polyphony Channels")->snapEnabled=true;
		configSwitch(MEMORY_PARAM,0,1,0,"Filter Type",{"Repeat","Remove"});
		configInput(VOCT_INPUT, "V/Oct");
		configInput(GATE_INPUT, "Gate");
		configOutput(VOCT_OUTPUT, "V/Oct");
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(GARBAGE_OUTPUT, "Note Removed");
		configBypass(0,0);
		configBypass(1,1);
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

	//Constants
	static constexpr float SEMITONE = 1.f/12.f;
	//Note Filter
	float latchVoltage[3] = {{0.f}};
	bool latchGate = false;
	bool zig = false;
	float lastGate[16] = {0};
	//Trigs
	bool rising;
	bool falling;
	//Voice Management
	VoiceManager vm;
	//Garbage out
	dsp::PulseGenerator garbage;

    void process(const ProcessArgs& args) override {
		float currentNote = inputs[VOCT_INPUT].getVoltage();
		float currentVoice = 0;
		float maxVoltage = params[MAX_PARAM].getValue();
		float minVoltage = params[MIN_PARAM].getValue();
		bool zig = params[MEMORY_PARAM].getValue() < 0.5;
		int voltageRange = abs(maxVoltage-minVoltage);
		int channels = inputs[GATE_INPUT].getChannels();
		int voices = params[POLY_PARAM].getValue();
		vm.voices = voices;
		//TODO Squash poly signal into mono
		rising = false;
		falling = false;
		for(int i=0; i<channels; i++){
			if(inputs[GATE_INPUT].getVoltage(i)>lastGate[i]){
				rising = true;
				currentNote = inputs[VOCT_INPUT].getVoltage(i);
				currentVoice = i;
			}

			if(inputs[GATE_INPUT].getVoltage(i)<lastGate[i]){
				vm.sendStop(i);
			};

			lastGate[i] = inputs[GATE_INPUT].getVoltage(i);
		}
		

		//Wrap Notes
		while(currentNote>maxVoltage){
			currentNote-=voltageRange;
		}
		while(currentNote<minVoltage&&minVoltage<maxVoltage){
			currentNote+=voltageRange;
		}

		//Filter Notes

		int size = 2;
		if(rising && abs(currentNote-latchVoltage[0]) > SEMITONE/2.f){//a Different note
			for(int i = 1; i < size; i++){//Shift Pitch Memory
				latchVoltage[i]=latchVoltage[i-1];
			}
			latchVoltage[0] = currentNote;
			vm.sendNote(latchVoltage[0],currentVoice);
		}else{

			if(rising && zig){

				float temp = latchVoltage[size-1];
				for(int i = 0; i < size; i++){//Shift Pitch Memory
					latchVoltage[i]=latchVoltage[i+1];
				}
				latchVoltage[0] = temp;

				vm.sendNote(latchVoltage[0],currentVoice);

			}

			if(rising && !zig){
				
				garbage.trigger();

			}

		}

		//Output notes from voice manager
		garbage.process(args.sampleTime);
		outputs[GATE_OUTPUT].setChannels(voices);
		outputs[VOCT_OUTPUT].setChannels(voices);
		for(int i = 0; i < voices; i++){
			outputs[GATE_OUTPUT].setVoltage(vm.getGate(i),i);
			outputs[VOCT_OUTPUT].setVoltage(vm.getPitch(i),i);
		}
		outputs[GARBAGE_OUTPUT].setVoltage(garbage.isHigh()*10.f);
	}
};


struct NotableWidget : ModuleWidget {
	int theme = -1;

	NotableWidget(Notable* module) {
		setModule(module);
		setPanel(createTintPanel(
			"res/panels/Notable.svg",
			getPalette(PAL_BUBBLEGUM)
		));
        //Add widgets here: 
        addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.620, 39.0)), module, Notable::MIN_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.620, 52.0)), module, Notable::MAX_PARAM));
		addParam(createParamCentered<QKnob8mm>(mm2px(Vec(7.620, 84.0+3)), module, Notable::POLY_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(7.620, 69.0-2.5)), module, Notable::MEMORY_PARAM));

		addInput(createInputCentered<QPort>(mm2px(Vec(7.620, 13.0)), module, Notable::VOCT_INPUT));
		addInput(createInputCentered<QPort>(mm2px(Vec(7.620, 26.0)), module, Notable::GATE_INPUT));

		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.620, 98.5+3)), module, Notable::VOCT_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(7.620, 111.5+3)), module, Notable::GATE_OUTPUT));
		addOutput(createOutputCentered<QPort>(mm2px(Vec(10.97, 76.5)), module, Notable::GARBAGE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Notable* module = getModule<Notable>();

		menu->addChild(new MenuSeparator);

		char const * p = reinterpret_cast<char const *>(module->latchVoltage);
		std::string s(p, p + sizeof(module->latchVoltage));
		menu->addChild(createMenuItem(s));

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
		Notable* module = getModule<Notable>();
		if(!module){
			return;
		}
		if(theme != module->theme){
			theme = module->theme;
			setPanel(createTintPanel(
				"res/panels/Notable.svg",
				getPalette(theme)
			));
		}
	}
};


Model* modelNotable = createModel<Notable, NotableWidget>("Notable");