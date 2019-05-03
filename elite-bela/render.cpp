#include <Bela.h>

#include <string>
#include <algorithm>
#include <eigenapi.h>
#include <math.h>

EigenApi::Eigenharp*	gApi=NULL;
class BelaCallback;
BelaCallback* 	gCallback=NULL;


AuxiliaryTask gProcessTask;



// really simple scale implmentation
static constexpr unsigned MAX_SCALE = 7;
static constexpr float SCALES [MAX_SCALE][13]= {
	{12.0f,0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,  7.0f, 8.0f, 9.0f, 10.0f, 11.0f},// chromatic
	{7.0f, 0.0f, 2.0f, 4.0f, 5.0f, 7.0f, 9.0f, 11.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // major
	{7.0f, 0.0f, 2.0f, 3.0f, 5.0f, 7.0f, 8.0f, 11.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // harmonic minor
	{7.0f, 0.0f, 2.0f, 3.0f, 5.0f, 7.0f, 8.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // natural minor
	{5.0f, 0.0f, 2.0f, 4.0f, 7.0f, 9.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // pentatonic major
	{5.0f, 0.0f, 3.0f, 5.0f, 7.0f, 10.0f,0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // pentatonic minor
	{7.0f, 0.0f, 2.0f, 3.0f, 6.0f, 7.0f, 8.0f, 11.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}   // hungarian minor
};
// scales converted from EigenD /plg_simple/scale_manager_plg.py

class BelaCallback : public EigenApi::Callback {
public:
	BelaCallback()   {
	}

	//========================== callback interface	======================
	void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals) override {
		rt_printf("device %s : %d - %d, %d : %d %d",dev, (int) dt, rows,cols,ribbons,pedals);
		dev_=dev;
		rows_=rows;
		cols_=cols;
		ribbons_=ribbons;
		pedals=pedals_;
		type_=dt;


		switch (dt) {
			case EigenApi::Callback::PICO:
				break;
			case EigenApi::Callback::TAU:
			case EigenApi::Callback::ALPHA:
				rt_printf("currently this is designed for the pico, some changes may be needed for TAU or ALPHA");
				break;
		}
		displayScale();
	}

	void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y) override {
		 //rt_printf("key %s , %d : %d,  %d  , %d %d %d \n", dev, course, key, a, p , r , y );

		if(course) {
			button(key,a);
		} else {
			switch(mode_) {
				case 0: {
					playNote(key,a,p,r,y);
					break;
				}
				case 1: {
					if(a && key<MAX_OCTAVE && key!=octave_) {
						gApi->setLED(dev_.c_str(),0,octave_,2);
						octave_=key;
						gApi->setLED(dev_.c_str(),0,octave_,1);
					}
					break;
				}
				case 2: {
					if(a && key<MAX_SCALE && key!=scaleIdx_) {
						gApi->setLED(dev_.c_str(),0,scaleIdx_,2);
						scaleIdx_=key;
						gApi->setLED(dev_.c_str(),0,scaleIdx_,1);
					}
					break;
				}
				case 3: {
					if(a && key<12 && key!=tonic_) {
						gApi->setLED(dev_.c_str(),0,tonic_,2);
						tonic_=key;
						gApi->setLED(dev_.c_str(),0,tonic_,1);
					}
					break;
				}
				case 4: {
					break;
				}
			}//switch
		}// main key
	}
	
	void breath(const char* dev, unsigned long long t, unsigned val) override {
		// rt_printf("breath %s , %d ", dev, val);
		breath_ = unipolar(val);
	}

	void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val) override {
		// rt_printf("strip %s , %d %d ", dev, strip, val);
		ribbon_ = unipolar(val);
	}

	void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val) override {
		// rt_printf("pedal %s , %d %d ", dev, pedal, val);
	};

	//=====================end of callback interface	======================
	void playNote( unsigned key, bool a, unsigned p, int r, int y) {
		float note = scaleNote(key);
		float mx = bipolar(r);
		float my = bipolar(y);
		float mz = unipolar(p);
		if(active_) {
			// currently have an active key
			if(touchId_==key) {
				// its this key, so update
				active_=a;  // might be off!
				touchId_=key;
				note_=note;
				x_=mx;
				y_=my;
				z_=mz;
			}

		} else {
			// no active key, so take this one
			if(a) {
     			// rt_printf("playNote  %f: %f %f %f\n",note,mx,my,mz);
				active_ = true;
				touchId_ = key;
				note_ = note;
				x_ = mx;
				y_ = my;
				z_ = mz;
			}
		}
	}
	

	void button(unsigned key, bool a) {
		// rt_printf("button %d %d ", key, a);
		int mode = key + 1;
		if(a) {
			if(mode_==0) { mode_=mode; enterMode(mode);}
		} else {
			if(mode_==mode) {mode_=0; enterMode(0);} 
		}
	}
	
	void enterMode(unsigned mode) {
		switch(mode) {
			case 0: {
				// playing mode
				displayScale();
				break;
			}
			case 1: {
				// select octave
				for(unsigned i=0;i<(rows_*cols_);i++) {
					gApi->setLED(dev_.c_str(),0, i,(i<MAX_OCTAVE ? 2  : 0)  ); 
				}
				gApi->setLED(dev_.c_str(),0,octave_,1);
				break;
			}
			case 2: {
				// select scale
				for(unsigned i=0;i<(rows_*cols_);i++) {
					gApi->setLED(dev_.c_str(),0, i,(i<MAX_SCALE ? 2 : 0) ); 
				}
				gApi->setLED(dev_.c_str(),0,scaleIdx_,1);
				break;
			}
			case 3: {
				// select tonic
				for(unsigned i=0;i<(rows_*cols_);i++) {
					gApi->setLED(dev_.c_str(),0, i,(i<12 ? 2 : 0)); 
				}
				gApi->setLED(dev_.c_str(),0,tonic_,1);
				break;
			}
			case 4: {
				// undecided mode :) 
				for(unsigned i=0;i<(rows_*cols_);i++) {
					gApi->setLED(dev_.c_str(),0, i,(i + 1) % 2 ); 
				}
				break;
			}
		}
	}
	
	void displayScale() {
		unsigned scalelen=SCALES[scaleIdx_][0];
		for(unsigned i=0;i<(rows_*cols_);i++) {
			// turn off all lights
			gApi->setLED(dev_.c_str(),0, i,(i%scalelen) == 0); 
		}
	}
	
	float scaleNote(unsigned key) {
		unsigned scalelen=SCALES[scaleIdx_][0];
		int octave = key / scalelen;
		int noteidx = (key % scalelen) + 1;
		float note = SCALES[scaleIdx_][ noteidx];
		return note + float(octave) * 12.0f;  
	}

    void render(BelaContext *context) {
		float a0=transpose(note_ + pitchbend(x_),octave_,tonic_);
		float a1=active_;
		float a2=scaleY(y_,1.0f);
		float a3=pressure(z_,1.0f);
		float a4=ribbon_;
		float a5=breath_;
		float a6= 0.0f;
		float a7= 0.0f;
		
		for(unsigned int n = 0; n < context->analogFrames; n++) {
			analogWriteOnce(context, n, 0,a0);
			analogWriteOnce(context, n, 1,a1);
			analogWriteOnce(context, n, 2,a2);
			analogWriteOnce(context, n, 3,a3);
			analogWriteOnce(context, n, 4,a4);
			analogWriteOnce(context, n, 5,a5);
			analogWriteOnce(context, n, 6,a6);
			analogWriteOnce(context, n, 7,a7);
		}    	
    }
    
    
private: 

	static constexpr float PRESSURE_CURVE=0.5f;

	float audioAmp(float z, float mult ) {
		return powf(z, PRESSURE_CURVE) * mult;
	}

	float pressure(float z, float mult ) {
		return ( (powf(z, PRESSURE_CURVE) * mult * ( 1.0f-ZERO_OFFSET) ) ) + ZERO_OFFSET;	
	}

	static constexpr float PB_CURVE=2.0f;
	static constexpr float PB_RANGE=2.0f;
	float pitchbend(float x) {
		float sign = x < 0 ? -1.0f : 1.0f;
		float pbx = powf(x * sign, PB_CURVE);
		return pbx*sign*PB_RANGE;
		
	}
	
	float scaleY(float y, float mult) {
		return ( (y * mult * 0.5f)  * ( 1.0f-ZERO_OFFSET) )  + 0.5f ;	
	}

	float scaleX(float x, float mult) {
		return ( (x * mult * 0.5f)  * ( 1.0f-ZERO_OFFSET) )  + 0.5f ;	
	}
	
	float transpose (float pitch, int octave, int semi) {
		return (pitch + semi + (( START_OCTAVE + octave) * 12.0f )) *  semiMult_ ;
	}



#ifdef SALT
	static constexpr float 	OUT_VOLT_RANGE=10.0f;
	static constexpr float 	ZERO_OFFSET=0.5f;
	static constexpr int   	START_OCTAVE=5;
#else 
	static constexpr float 	OUT_VOLT_RANGE=5.0f;
	static constexpr float 	ZERO_OFFSET=0.0f;
	static constexpr int 	START_OCTAVE=0.0f;
#endif 

	static constexpr unsigned MAX_OCTAVE = OUT_VOLT_RANGE;
	static constexpr float semiMult_ = (1.0f / (OUT_VOLT_RANGE * 12.0f)); // 1.0 = 10v = 10 octaves 

	std::string dev_;
	float rows_;
	float cols_;
	float ribbons_;
	float pedals_;
	DeviceType type_;


	float touchId_=0.0f;   
    float note_=0.0f;
    float x_=0.0f,y_=0.0f,z_=0.0f;
    bool active_ = false;

    float breath_=0.0f;
    float ribbon_=0.0f;
    
    unsigned octave_ = 1;
    unsigned tonic_ = 0; //C 
    unsigned scaleIdx_ = 0;
    unsigned mode_ = 0;

	inline float clamp(float v, float mn, float mx) { return (std::max(std::min(v, mx), mn)); }

	float unipolar(int val) { return std::min(float(val) / 4096.0f, 1.0f); }

	float bipolar(int val) { return clamp(float(val) / 4096.0f, -1.0f, 1.0f); }

};

void eliteProcess(void* pvApi) {
	EigenApi::Eigenharp *pApi = (EigenApi::Eigenharp*) pvApi;
	pApi->poll(0);
}


bool setup(BelaContext *context, void *userData) {
	gApi= new EigenApi::Eigenharp("./");
	gCallback=new BelaCallback();
	gApi->addCallback(gCallback);

	gApi->create();

	gApi->start();
	
    // Initialise auxiliary tasks

	if((gProcessTask = Bela_createAuxiliaryTask(&eliteProcess, BELA_AUDIO_PRIORITY - 1, "eliteProcess", gApi)) == 0)
		return false;

	return true;
}

// render is called 2750 per second (44000/16)
const int decimation = 5;  // = 550/seconds
long renderFrame = 0;
void render(BelaContext *context, void *userData)
{
	Bela_scheduleAuxiliaryTask(gProcessTask);
	
	renderFrame++;
	// silence audio buffer
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, 0.0f);
		}
	}
	
	gCallback->render(context);
	
	
	if(decimation <= 1 || ((renderFrame % decimation) == 0) ) {	
	}

}

void cleanup(BelaContext *context, void *userData)
{
	gApi->clearCallbacks();
	delete gCallback;
	
	gApi->stop();
	gApi->destroy();
	delete gApi;
}