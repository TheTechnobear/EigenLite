#pragma once
// Required ihx firmware files
#define PICO_FIRMWARE "pico.ihx"
#define BASESTATION_FIRMWARE "bs_mm_fw_0103.ihx"
#define PSU_FIRMWARE "psu_mm_fw_0102.ihx"

namespace EigenApi
{
    class Callback
    {
    public:
    	enum DeviceType {
    		PICO,
    		TAU,
    		ALPHA
    	};
    	virtual ~Callback() = default;

        // information 
        virtual void newDevice(DeviceType dt, const char* name) {}

        // device events 
        // note: dev = usb id, name is a 'friendly' enumeration of usb devices 
        virtual void connected(const char* dev, DeviceType dt, const char* name) {};
        virtual void disconnected(const char* dev) {};

        virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y) {};
        virtual void breath(const char* dev, unsigned long long t, unsigned val) {};
        virtual void strip(const char* dev, unsigned long long t, unsigned strip, unsigned val, bool a) {};
        virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, unsigned val) {};
        virtual void dead(const char* dev, unsigned reason) {};
    };

    // Firmware reader Interface for providing other implementations than the default Posix
    class IFW_Reader
    {
    public:
        virtual ~IFW_Reader() = default;
        virtual bool open(const char* filename, int oFlags, void* *fd) = 0;
        virtual long read(void* fd, void *data, long byteCount) = 0;
        virtual void close(void* fd) = 0;
        virtual bool confirmResources() = 0;
    };

    // Default firmware reader implementation for OSX/Linux
    class FWR_Posix : public EigenApi::IFW_Reader
    {
    public:
        explicit FWR_Posix(const char* path);

        bool open(const char* filename, int oFlags, void* *fd) override;
        long read(void* fd, void *data, long byteCount) override;
        void close(void* fd) override;
        bool confirmResources() override;

    private:
        const char* path_;
    };

    class FWR_Embedded : public EigenApi::IFW_Reader
    {
    public:
        explicit FWR_Embedded();

        bool open(const char* deviceihx, int oFlags, void* *fd) override;
        long read(void* fd, void *data, long byteCount) override;
        void close(void* fd) override;
        bool confirmResources() override;

    private:
        long position_=0;
        unsigned maxLen_=0;
    };


    // important note: 
    // any pointers passed in, are owned by caller
    // they are assumed valid until explicitly removed, or api is destroyed
    class Eigenharp
    {
    public:
        explicit Eigenharp(); // use an embedded reader
        explicit Eigenharp(IFW_Reader *fwReader);
        virtual ~Eigenharp();

        const char* versionString();

        bool start();
        bool process();
        bool stop();

        void addCallback(Callback* api);
        void removeCallback(Callback* api);
        void clearCallbacks();
        
        void setLED(const char* dev, unsigned course, unsigned int key, unsigned int colour);

        void setPollTime(unsigned pollTime);

    private:
        void *impl;
    };
    
    class Logger
    {
    public:
        static void setLogFunc(void (*pLogFn)(const char*));
        static void logmsg(const char*);
    private:
        static void (*_logmsg)(const char*);
    };
}

