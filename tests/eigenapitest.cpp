#include <eigenapi.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>
#include <thread>

class PrinterCallback : public EigenApi::Callback {
   public:
    PrinterCallback(EigenApi::Eigenharp& eh) : eh_(eh), led_(false) {
    }

    void beginDeviceInfo() override {
        std::cout << "=====================================================" << std::endl;
        std::cout << "DEVICE UPDATE - BEGIN" << std::endl;
    }

    void deviceInfo(bool isPico, unsigned devEnum, const char* dev) override {
        std::cout << "DEVICE ";
        if (isPico)
            std::cout << "pico";
        else
            std::cout << "base";
        std::cout << "-" << devEnum << "  ( " << dev << " )" << std::endl;
    }

    void endDeviceInfo() override {
        std::cout << "DEVICE UPDATE - END" << std::endl;
        std::cout << "=====================================================" << std::endl;
    }

    void connected(const char* dev, DeviceType dt) override {
        std::cout << "dev id " << dev << " ( " << dt << " )" << std::endl;
        switch (dt) {
            case PICO:
                maxLeds_ = 16;
                break;
            case TAU:
                maxLeds_ = 84;
                break;
            case ALPHA:
                maxLeds_ = 120;
                break;
            default:
                maxLeds_ = 0;
        }
    }

    void disconnected(const char* dev) override {
        std::cout << "dev id " << dev << std::endl;
    }

    void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y) override {
        std::cout << "key " << dev << " @ " << t << " - " << course << ":" << key << ' ' << a << ' ' << p << ' ' << r << ' ' << y << std::endl;
        if (course) {
            // mode key
            eh_.setLED(dev, course, key, a ? EigenApi::Eigenharp::LED_GREEN : EigenApi::Eigenharp::LED_OFF);
        } else {
            if (!a) {
                led_ = !led_;

                for (int i = 0; i < maxLeds_; i++) {
                    if (led_)
                        eh_.setLED(dev, 0, i, EigenApi::Eigenharp::LedColour(i % 3));
                    else
                        eh_.setLED(dev, 0, i, EigenApi::Eigenharp::LED_OFF);
                }
            }
        }
    }
    void breath(const char* dev, unsigned long long t, unsigned val) override {
        std::cout << "breath " << dev << " @ " << t << " - " << val << std::endl;
    }

    void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val, bool a) override {
        std::cout << "strip " << dev << " @ " << t << " - " << strip << " = " << val << " " << a << std::endl;
    }

    void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val) override {
        std::cout << "pedal " << dev << " @ " << t << " - " << pedal << " = " << val << std::endl;
    }

    EigenApi::Eigenharp& eh_;
    bool led_ = false;
    int maxLeds_ = 0;
};

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    std::cerr << "int handler called";
    if (keepRunning == 0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}

void* process(void* arg) {
    EigenApi::Eigenharp* pE = static_cast<EigenApi::Eigenharp*>(arg);
    while (keepRunning) {
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

#define USE_EMBEDDED_FWR 1
int main(int ac, char** av) {
    signal(SIGINT, intHandler);

#if USE_EMBEDDED_FWR
    EigenApi::FWR_Embedded fwr;
#else
    // EigenApi::FWR_Posix fwr("./resources/firmware/ihx/");
    std::cout << "Looking for the ihx files at: \"" << fwr.getPath() << "\"..." << std::endl;
    bool ihxFilesFound = fwr.confirmResources();
    if (!ihxFilesFound) {
        fwr.setPath("./");
        std::cout << "Looking for the ihx files at: \"" << fwr.getPath() << "\"..." << std::endl;
        ihxFilesFound = fwr.confirmResources();
    }
    if (!ihxFilesFound) {
        std::cout << "Could not find the ihx firmware files." << std::endl;
        return -1;
    }
#endif

    EigenApi::Eigenharp myD(&fwr);
    myD.setPollTime(100);
    myD.addCallback(new PrinterCallback(myD));

    // no filter , dev num =0 (default)
    // myD.setDeviceFilter(false, 0);

    // myD.setDeviceFilter(true, 1); // first pico
    // myD.setDeviceFilter(true, 2); // second pico
    // myD.setDeviceFilter(false, 1); // first base
    // myD.setDeviceFilter(false, 2); // second base

    if (!myD.start()) {
        std::cout << "unable to start EigenLite";
        return -1;
    }

    std::thread t = std::thread(process, &myD);
    t.join();

    myD.stop();
    return 0;
}
