#pragma once
#include <math.h> 
#include <deque>

const float pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;

//Cytomic Allpass
struct CytomicAllpass
{
    float c_sr = 1;
    float out = 0;
    float ic1eq = 0;
    float ic2eq = 0;
    float q=0.5;

    void setCutoff(float c,float sr){
        c_sr = c/sr;
    }

    void setQ(float newQ){
        q = newQ;
    }

    void process(float v0){
        float g = tan(pi*c_sr);
        float k = 1/q;
        float a1 = 1/(1 + g*(g + k));
        float a2 = g*a1;
        float a3 = g*a2;
        float m0 = 1;
        float m1 = -2*k;
        float m2 = 0;

        float v3 = v0 - ic2eq;
        float v1 = a1*ic1eq + a2*v3;
        float v2 = ic2eq + a2*ic1eq + a3*v3;
        ic1eq = 2*v1 - ic1eq;
        ic2eq = 2*v2 - ic2eq;

        out = m0*v0 + m1*v1 + m2*v2;
    }

    float get(){
        return out;
    }
};

//Delay Line REDO AS A DEQUE TO get rid of modulo maybe
template <int MAXDELAY>
struct DelayLine
{
    float buffer[MAXDELAY+1] = {0};
    int writeHead=0;
    void write(float val){
        buffer[writeHead] = val;
    }
    void process(){
        writeHead ++;
        writeHead = (writeHead+MAXDELAY)%MAXDELAY;
    }
    float readLerp(float pos){
        pos=writeHead-pos;
        int ipos = pos;
        float fpos = pos - ipos;
        ipos = (ipos+MAXDELAY)%MAXDELAY;
        int i2pos = ((ipos-1)+MAXDELAY)%MAXDELAY;
        return buffer[i2pos]*fpos+buffer[ipos]*(1-fpos);
    }
    float readRaw(float pos){
        pos=writeHead-pos;
        int ipos = pos;
        ipos = (ipos+MAXDELAY)%MAXDELAY;
        return buffer[ipos];
    }
    float readRaw(int ipos){
        ipos=writeHead-ipos;
        ipos = (ipos+MAXDELAY)%MAXDELAY;
        return buffer[ipos];
    }
};

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