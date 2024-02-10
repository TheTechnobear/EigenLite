#pragma once

#include <lib_alpha2/alpha2_active.h>
#include <lib_pico/pico_active.h>
#include <picross/pic_usb.h>
#include <sys/fcntl.h>

#include <memory>
#include <set>
#include <thread>
#include <vector>

#include "ef_harp.h"
#include "eigenlite_impl.h"

namespace EigenApi {

struct devcheck : virtual pic::tracked_t {
    devcheck(const std::string& dev) : dev_(dev) {}
    void found(const std::string& device) { found_ |= (dev_ == device); }
    const std::string& dev_;
    bool found_ = false;
};

struct devfinder : virtual pic::tracked_t {
    devfinder(std::vector<std::string>& devlist) : devlist_(devlist) {}

    void found(const std::string& device) { devlist_.push_back(device); }

    std::vector<std::string>& devlist_;
};

class EF_Harp {
public:
    EF_Harp(EigenLite& efd);
    virtual ~EF_Harp();

    const char* name();
    virtual bool create(const std::string& usbdev);
    virtual bool destroy();
    virtual bool start();
    virtual bool stop();
    virtual bool poll(long long t);

    bool stopping() { return stopping_; }

    virtual void fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, float p, float r, float y);
    virtual void fireBreathEvent(unsigned long long t, float val);
    virtual void fireStripEvent(unsigned long long t, unsigned strip, float val, bool a);
    virtual void firePedalEvent(unsigned long long t, unsigned pedal, float val);
    virtual void fireDeadEvent(unsigned reason);

    virtual void restartKeyboard() = 0;
    virtual void setLED(unsigned course, unsigned keynum, unsigned colour) = 0;

    pic::usbdevice_t* usbDevice() { return pDevice_; }

    bool loadFirmware(pic::usbdevice_t* pDevice, std::string ihxFile);
    static void logmsg(const char* msg);

protected:
private:
    // for controlling firmware loading
    static void pokeFirmware(pic::usbdevice_t* pDevice, int address, int byteCount, void* data);
    static void firmwareCpucs(pic::usbdevice_t* pDevice, const char* mode);
    static void resetFirmware(pic::usbdevice_t* pDevice);
    static void runFirmware(pic::usbdevice_t* pDevice);
    static void sendFirmware(pic::usbdevice_t* pDevice, int recType, int address, int byteCount, void* data);

    // for IHX processing
    static unsigned hexToInt(char* buf, int len);
    static char hexToChar(char* buf);
    bool processIHXLine(pic::usbdevice_t* pDevice, void* fd);

    static constexpr float STRIP_HYSTERISIS = 0.01f;
    static constexpr float PEDAL_HYSTERISIS = 0.01f;
    static constexpr float BREATH_HYSTERISIS = 0.01f;

    class IHXException {
    public:
        IHXException(const std::string& reason) : reason_(reason) {}
        const std::string& reason() { return reason_; }

    private:
        std::string reason_;
    };

    pic::usbdevice_t* pDevice_;
    unsigned lastBreath_;
    unsigned lastStrip_[2];
    unsigned lastPedal_[4];
    bool stopping_;

protected:
    EigenLite& efd_;
};

class EF_Pico : public EF_Harp {
public:
    EF_Pico(EigenLite& efd);
    virtual ~EF_Pico();

    bool create(const std::string& usbdev) override;
    bool destroy() override;
    bool start() override;
    bool stop() override;
    bool poll(long long t) override;

    void restartKeyboard() override;

    void setLED(unsigned course, unsigned keynum, unsigned colour) override;
    void fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, float p, float r, float y) override;

    static std::vector<std::string> availableDevices();

private:
    bool checkFirmware(const std::string& usbdev);
    bool loadPicoFirmware(const std::string& usbdev);
    pico::active_t* pLoop_;
    unsigned lastMode_[4];

    class Delegate : public pico::active_t::delegate_t {
    public:
        Delegate(EF_Pico& p) : parent_(p), s_count_(100), s_threshold_(65), s_state_(0), s_last_(0) { ; }
        void kbd_dead(unsigned reason);
        void kbd_raw(bool resync, const pico::active_t::rawkbd_t&);
        void kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y);
        void kbd_strip(unsigned long long t, unsigned s);
        void kbd_breath(unsigned long long t, unsigned b);
        void kbd_mode(unsigned long long t, unsigned key, unsigned m);

        static constexpr int STRIP_THRESH = 50;
        static constexpr int STRIP_MIN = 110;
        static constexpr int STRIP_MAX = 3050;
        static constexpr float STRIP_RANGE = float(STRIP_MAX) - float(STRIP_MIN);
        static constexpr int STRIP_MID_RANGE = STRIP_RANGE / 2.0f;

        static constexpr float SENSOR_RANGE = 4096.f;
        static constexpr float MID_SENSOR_RANGE = 2047.f;
        static constexpr float PRESSURE_RANGE = 3192.f;
        // static constexpr float PRESSURE_RANGE = 4096.f;
        static constexpr float ROLL_YAW_RANGE = 1024.f;
        static constexpr float BREATH_GAIN = 1.4f;

        inline float clip(float v) {
            v = std::max(v, -1.f);
            v = std::min(v, 1.f);
            return v;
        }

        inline float aclip(float v) {
            v = std::max(v, 0.f);
            v = std::min(v, 1.f);
            return v;
        }


        inline float pToFloat(int v) { return aclip(float(v) / PRESSURE_RANGE); }
        inline float rToFloat(int v) { return clip(float(v) / ROLL_YAW_RANGE); }
        inline float yToFloat(int v) { return clip(float(v) / ROLL_YAW_RANGE); }
        inline float stripToFloat(int v) { return 1.0f - aclip(float(v - STRIP_MIN) / STRIP_RANGE); }
        inline float breathToFloat(int v) {
            return clip(((float(v) - MID_SENSOR_RANGE) / MID_SENSOR_RANGE) * BREATH_GAIN);
        }
        inline float pedalToFloat(int v) { return float(v) / SENSOR_RANGE; }

        EF_Pico& parent_;

        int breathWarmUp_ = 0;
        float breathZero_ = 0.f;
        unsigned s_count_, s_threshold_, s_state_, s_last_;
    } delegate_;
};

class EF_BaseStation : public EF_Harp {
public:
    EF_BaseStation(EigenLite& efd);
    virtual ~EF_BaseStation();

    bool create(const std::string& usbdev) override;
    bool destroy() override;
    bool start() override;
    bool stop() override;
    bool poll(long long t) override;
    void restartKeyboard() override;

    void setLED(unsigned course, unsigned keynum, unsigned colour) override;

    static std::vector<std::string> availableDevices();

    unsigned short* curMap() { return curmap_; }
    unsigned short* skpMap() { return skpmap_; }

protected:
    alpha2::active_t* loop() { return pLoop_; }

private:
    bool checkFirmware(const std::string& usbdevice);
    bool loadBaseStation(const std::string& usbdev, unsigned short bstype);
    std::shared_ptr<alpha2::active_t::delegate_t> delegate_;
    alpha2::active_t* pLoop_;
    unsigned short curmap_[9], skpmap_[9];
    bool isAlpha_;
};

class EF_BaseDelegate : public alpha2::active_t::delegate_t {
public:
    EF_BaseDelegate(EF_BaseStation& p) : parent_(p) {}
    // alpha2::active_t::delegate_t
    void kbd_dead(unsigned reason) override;
    void kbd_raw(unsigned long long t, unsigned key, unsigned c1, unsigned c2, unsigned c3, unsigned c4) override;
    void pedal_down(unsigned long long t, unsigned pedal, unsigned p) override;
    void midi_data(unsigned long long t, const unsigned char* data, unsigned len) override;
    void kbd_mic(unsigned char s, unsigned long long t, const float* samples) override;

protected:
    static constexpr float SENSOR_RANGE = 4096.f;
    static constexpr float MID_SENSOR_RANGE = 2047.f;
    static constexpr float PRESSURE_RANGE = 4096.f;
    static constexpr float ROLL_YAW_RANGE = 1024.f;

    inline float clip(float v) {
        v = std::max(v, -1.f);
        v = std::min(v, 1.f);
        return v;
    }

    inline float aclip(float v) {
        v = std::max(v, 0.f);
        v = std::min(v, 1.f);
        return v;
    }

    inline float pToFloat(int v) { return aclip(float(v) / PRESSURE_RANGE); }
    inline float rToFloat(int v) { return clip((MID_SENSOR_RANGE - float(v)) / ROLL_YAW_RANGE); }
    inline float yToFloat(int v) { return clip((MID_SENSOR_RANGE - float(v)) / ROLL_YAW_RANGE); }
    inline float stripToFloat(int v) { return 1.0f - aclip(float(v) / SENSOR_RANGE); }
    inline float breathToFloat(int v) { return clip((float(v) - MID_SENSOR_RANGE) / MID_SENSOR_RANGE); }
    inline float pedalToFloat(int v) { return aclip(float(v) / SENSOR_RANGE); }

    EF_BaseStation& parent_;
};

class EF_Alpha : public EF_BaseDelegate {
public:
    EF_Alpha(EF_BaseStation& p) : EF_BaseDelegate(p) {}

    // alpha2::active_t::delegate_t
    void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y) override;
    void kbd_keydown(unsigned long long t, const unsigned short* bitmap) override;

private:
    void fireAlphaKeyEvent(unsigned long long t, unsigned key, bool a, float p, float r, float y);
};

class EF_Tau : public EF_BaseDelegate {
public:
    EF_Tau(EF_BaseStation& p) : EF_BaseDelegate(p) {}

    void kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y) override;
    void kbd_keydown(unsigned long long t, const unsigned short* bitmap) override;

private:
    void fireTauKeyEvent(unsigned long long t, unsigned key, bool a, float p, float r, float y);
};

}  // namespace EigenApi
