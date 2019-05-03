#include <Bela.h>

#include <string>
#include <algorithm>
#include <eigenapi.h>
#include <math.h>

EigenApi::Eigenharp*	gApi=NULL;
class BelaCallback;
BelaCallback* 	gCallback=NULL;


AuxiliaryTask gProcessTask;


class BelaCallback : public EigenApi::Callback {
public:
	BelaCallback()  {
		;
	}

	void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals) override {
		rt_printf("device %s : %d - %d, %d : %d %d",dev, (int) dt, rows,cols,ribbons,pedals);
		dev_=dev;
		rows_=rows;
		cols_=cols;
		ribbons_=ribbons;
		pedals=pedals_;
		type_=dt;


//		switch (dt) {
//			case EigenApi::Callback::PICO:
//				break;
//			case EigenApi::Callback::TAU:
//				break;
//			case EigenApi::Callback::ALPHA:
//				break;
//		}
	}

	void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y) override {
		 //rt_printf("key %s , %d : %d,  %d  , %d %d %d \n", dev, course, key, a, p , r , y );

		float note = course * 128 + key;
		if(course) {
			button(key,a);
		} else {
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
					active_ = true;
					touchId_ = key;
					note_ = note;
					x_ = mx;
					y_ = my;
					z_ = mz;
				}
			}
		}
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


	void button(unsigned key, bool a) {
		// rt_printf("button %d %d ", key, a);
		gApi->setLED(dev_.c_str(),key+18,a);
	}

    void render(BelaContext *context) {
		float a0=transpose(note_,0,0);
		float a1=active_;
		float a2=scaleY(y_,1.0f);
		float a3=pressure(z_,1.0f);
		float a4=breath_;
		float a5=ribbon_;
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
	

	float scaleY(float y, float mult) {
		return ( (y * mult)  * ( 1.0f-ZERO_OFFSET) )  + ZERO_OFFSET ;	
	}

	float scaleX(float x, float mult) {
		return ( (x * mult)  * ( 1.0f-ZERO_OFFSET) )  + ZERO_OFFSET ;	
	}
	
	float transpose (float pitch, int octave, int semi) {
		return pitch + (((( START_OCTAVE + octave) * 12 ) + semi) *  semiMult_ );
	}



#ifdef SALT
	static constexpr float 	OUT_VOLT_RANGE=10.0f;
	static constexpr float 	ZERO_OFFSET=0.5f;
	static constexpr int   	START_OCTAVE=5;
#else 
	static constexpr float 	OUT_VOLT_RANGE=5.0f;
	static constexpr float 	ZERO_OFFSET=0;
	static constexpr int 	START_OCTAVE=1.0f;
#endif 

	static constexpr float semiMult_ = (1.0f / (OUT_VOLT_RANGE * 12.0f)); // 1.0 = 10v = 10 octaves 

	std::string dev_;
	float rows_;
	float cols_;
	float ribbons_;
	float pedals_;
	DeviceType type_;


	float touchId_;   
    float note_;
    float x_,y_,z_;
    float active_;

    float breath_;
    float ribbon_;

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
	
	
	if(decimation <= 1 || ((renderFrame % decimation) ==0) ) {	
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