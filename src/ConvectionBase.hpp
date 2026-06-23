#pragma once

#include "plugin.hpp"
struct Segment {
	enum SegmentType{
		ST_UP,
		ST_DOWN,
		ST_UPDOWN,
		ST_DOWNUP,
		ST_CONSTANT
	};

	float phase = 0;
	bool active = false;
	dsp::PulseGenerator eoc;
	dsp::PulseGenerator trig;
	float direct = 0;
	float single = 0;
	float slope = 0;
	int type = 0;
	int position = 0;

	Segment(){

	}

	inline void trigger(float skew, int type){
		phase = 0;
		slope = 0;
		direct = 0;
		trig.trigger();
		active = true;
	}

	inline void reset(){
		phase = 0;
		slope = 0;
		direct = 0;
		active = false;
	}

	inline void process(float delta, float length, float scale, float density, float skew, int type, bool dist){
		eoc.process(delta);
		trig.process(delta);
		//return if inactive
		if(!active){
			slope = 0;
			return;
		}
		//advance phase
		phase += delta/length*scale*1000;
		if(phase >=scale){
			phase = scale;
			active = false;
			eoc.trigger();
		}
		//apply envelope curve
		float skewR = (skew+1)/2;
		float last = direct;
		float power = dsp::exp2_taylor5(skew*2);
		switch(type){
			case ST_UP:
				direct = pow(phase,power);
				single = direct;
				break;
			case ST_DOWN:
				direct = pow(1-phase,power)-1;
				single = direct+1;
				break;
			case ST_UPDOWN:
				direct = (phase < skewR) ? pow(phase/skewR,1/power) : pow((1-phase)/(1-skewR), power);
				single = direct;
				break;
			case ST_DOWNUP:
				direct = (phase < skewR) ? pow(phase/skewR,1/power) : pow((1-phase)/(1-skewR), power);
				direct = -direct;
				single = 1+direct;
				break;
			case ST_CONSTANT:
				direct = phase < scale ? skew : 0;
				single = direct;
				break;
		}
		//slope
		slope = direct - last;
		//generate trigs
		if(int(direct*density)!=position&&dist){
			position = direct*density;
			trig.trigger();
		}else if(int(phase*density)!=position&&!dist&&phase<scale){
			position = phase*density;
			trig.trigger();
		}
	}
};