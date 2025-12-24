#pragma once

template <typename T> int sign(T val);

float magnitude(float x, float y);

enum staqueModes{
	SM_STACK,
	SM_QUEUE
};

float ratioFrom(float f);

float softClip(float val, float thresh, float hardness );

float softClip(float val, float thresh );

float softFold(float val, float thresh);

template <typename T>
struct staque
{
	static constexpr int MAX_DELAY = 2646000;

	std::deque<T> data = {};
	int mode = SM_STACK;

	void push(T val){
		data.push_front(val);
		if(data.size()>MAX_DELAY){
			data.pop_back();
		}
	}

	void pop(){
		if(mode==SM_STACK){
			data.pop_front();
		}else{
			data.pop_back();
		}
	}

	T top(){
		if(mode==SM_STACK){
			return data.front();
		}else{
			return data.back();
		}
	}

	bool empty(){
		return data.empty();
	}

	void clear(){
		data.clear();
	}

	void swap(staque *s){
		data.swap(s->data);
	}
};

float softFold05(float val);

float exp2Decay(float val, float time, float sr);

float mxyRound(float val, float shape);
float mxySpike(float val, float shape);
