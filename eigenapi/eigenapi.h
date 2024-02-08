#pragma once
// Required ihx firmware files
#define PICO_FIRMWARE "pico.ihx"
#define BASESTATION_FIRMWARE "bs_mm_fw_0103.ihx"
#define PSU_FIRMWARE "psu_mm_fw_0102.ihx"

namespace EigenApi {
class Callback {
   public:
    enum DeviceType {
        PICO,
        TAU,
        ALPHA
    };
    virtual ~Callback() = default;

    // information
    virtual void beginDeviceInfo() {}
    virtual void deviceInfo(bool isPico, unsigned devEnum, const char* dev) {}
    virtual void endDeviceInfo() {}

    // device events
    // note: dev = usb id, name is a 'friendly' enumeration of usb devices
    virtual void connected(const char* dev, DeviceType dt){};
    virtual void disconnected(const char* dev){};

    virtual void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, float p, float r, float y){};
    virtual void breath(const char* dev, unsigned long long t, float val){};
    virtual void strip(const char* dev, unsigned long long t, unsigned strip, float val, bool a){};
    virtual void pedal(const char* dev, unsigned long long t, unsigned pedal, float val){};
    virtual void dead(const char* dev, unsigned reason){};
};

class IFW_Reader;

// important note:
// any pointers passed in, are owned by caller
// they are assumed valid until explicitly removed, or api is destroyed
class Eigenharp {
   public:
    explicit Eigenharp();  // use an embedded reader
    explicit Eigenharp(IFW_Reader* fwReader);
    virtual ~Eigenharp();

    const char* versionString();

    bool start();
    bool process();
    bool stop();

    void addCallback(Callback* api);
    void removeCallback(Callback* api);
    void clearCallbacks();

    enum LedColour {
        LED_OFF,
        LED_GREEN,
        LED_RED,
        LED_ORANGE
    };

    void setLED(const char* dev, unsigned course, unsigned key, LedColour colour);

    void setPollTime(unsigned pollTime);

    // allBasePico 0= All, 1 = basestation only, 2 = pico only
    // devEnum 0= All, 1-N = device 
    void setDeviceFilter(unsigned allBasePico, unsigned devEnum);

   private:
    void* impl;
};

// Firmware reader
class IFW_Reader {
   public:
    virtual ~IFW_Reader() = default;
    virtual bool open(const char* filename, int oFlags, void** fd) = 0;
    virtual long read(void* fd, void* data, long byteCount) = 0;
    virtual void close(void* fd) = 0;
    virtual bool confirmResources() = 0;
};

// firmware reader implementation for OSX/Linux files
class FWR_Posix : public EigenApi::IFW_Reader {
   public:
    explicit FWR_Posix(const char* path);

    bool open(const char* filename, int oFlags, void** fd) override;
    long read(void* fd, void* data, long byteCount) override;
    void close(void* fd) override;
    bool confirmResources() override;

   private:
    const char* path_;
};

// use firmware embedeed into EigenLite
class FWR_Embedded : public EigenApi::IFW_Reader {
   public:
    explicit FWR_Embedded();

    bool open(const char* deviceihx, int oFlags, void** fd) override;
    long read(void* fd, void* data, long byteCount) override;
    void close(void* fd) override;
    bool confirmResources() override;

   private:
    long position_ = 0;
    unsigned maxLen_ = 0;
};

class Logger {
   public:
    static void setLogFunc(void (*pLogFn)(const char*));
    static void logmsg(const char*);

   private:
    static void (*_logmsg)(const char*);
};
}  // namespace EigenApi
