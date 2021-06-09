#include <eigenapi.h>

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <thread>

class PrinterCallback: public  EigenApi::Callback
{
public:
    PrinterCallback(EigenApi::Eigenharp& eh) : eh_(eh), led_(false)
    {
        for(int i;i<3;i++) { max_[i]=0; min_[i]=4096;}
    }
    
    virtual void device(const char* dev, DeviceType dt, int rows, int cols, int ribbons, int pedals)
    {
        std::cout << "device " << dev << " (" << dt << ") " << rows << " x " << cols << " strips " << ribbons << " pedals " << pedals << std::endl;
    }
    
    virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
    {
        std::cout  << "key " << dev << " @ " << t << " - " << course << ":" << key << ' ' << a << ' ' << p << ' ' << r << ' ' << y << std::endl;
//        if(p >max_[0]) max_[0]=p;
//        if(r >max_[1]) max_[1]=r;
//        if(y >max_[2]) max_[2]=y;
//        if(p <min_[0]) min_[0]=p;
//        if(r <min_[1]) min_[1]=r;
//        if(y <min_[2]) min_[2]=y;
        
        
        bool led = course > 0;
        if (led_ != led) {
            led_ = led;
            eh_.setLED(dev,course, key,led_);
//            std::cout  << min_[0] << " to " << max_[0] << ", " << min_[1] << " to " << max_[1] << ", " << min_[2] << " to " << max_[2] << std::endl;
        }
    }
    virtual void breath(const char* dev, unsigned long long t, unsigned val)
    {
        std::cout  << "breath " << dev << " @ " << t << " - "  << val << std::endl;
    }
    
    virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val, bool a)
    {
        std::cout  << "strip " << dev << " @ " << t << " - " << strip << " = " << val << " " << a << std::endl;
    }
    
    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
    {
        std::cout  << "pedal " << dev << " @ " << t << " - " << pedal << " = " << val << std::endl;
    }
    
    EigenApi::Eigenharp& eh_;
    bool led_;
    int max_[3], min_[3];
    
};

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    std::cerr  << "int handler called";
    if(keepRunning==0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}

void* process(void* arg) {
    EigenApi::Eigenharp *pE = static_cast<EigenApi::Eigenharp*>(arg);
    while(keepRunning) {
        pE->process();
    }
    return nullptr;
}

void makeThreadRealtime(std::thread& thread) {

    int policy;
    struct sched_param param;

    pthread_getschedparam(thread.native_handle(), &policy, &param);
    param.sched_priority = 95;
    pthread_setschedparam(thread.native_handle(), SCHED_FIFO, &param);

}

int main(int ac, char **av)
{
    signal(SIGINT, intHandler);
    EigenApi::Eigenharp myD("./");
    myD.setPollTime(100);
    myD.addCallback(new PrinterCallback(myD));
    if(!myD.start())
    {
		std::cout  << "unable to start EigenLite";
		return -1;
    }

    std::thread t=std::thread(process, &myD);
//    makeThreadRealtime(t);
    t.join();

    myD.stop();
    return 0;
}
