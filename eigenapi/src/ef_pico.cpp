#include <eigenapi.h>
#include <lib_pico/pico_usb.h>
#include <picross/pic_config.h>
#include <picross/pic_log.h>
#include <picross/pic_time.h>
#include <picross/pic_usb.h>

#include "ef_harp.h"
#include "eigenlite_impl.h"

#define DEFAULT_DEBOUNCE 25000

// these are all hardcode in eigend, rather than in pico_usb :(

#define PICO_PRE_LOAD 0x0001

#define PRODUCT_ID_PICO BCTPICO_USBPRODUCT

#define PICO_MAINKEYS 18

namespace EigenApi {

// public interface
EF_Pico::EF_Pico(EigenLite &efd) : EF_Harp(efd), pLoop_(NULL), delegate_(*this) {
}

EF_Pico::~EF_Pico() {
}

bool EF_Pico::create(const std::string &usbdev) {
    logmsg("create eigenharp pico");

    if (!checkFirmware(usbdev)) {
        return false;
    }

    if (!EF_Harp::create(usbdev)) return false;

    try {
        logmsg("close device to allow active_t to open");
        usbDevice()->detach();
        usbDevice()->close();
        logmsg("create pico loop");
        pLoop_ = new pico::active_t(usbDevice()->name(), &delegate_);
        pLoop_->load_calibration_from_device();
        logmsg("created pico loop");

    } catch (pic::error &e) {
        // error is logged by default, so dont need to repeat, but useful if we want line number etc for debugging
        // logmsg(e.what());
        return false;
    }

    return true;
}

bool EF_Pico::destroy() {
    logmsg("destroy pico....");
    EF_Pico::stop();
    {
        delete pLoop_;
        pLoop_ = NULL;
    }
    logmsg("destroyed pico");
    efd_.fireDisconnectEvent(usbDevice()->name());
    return EF_Harp::destroy();
}

bool EF_Pico::start() {
    if (!EF_Harp::start()) return false;

    if (pLoop_ == NULL) return false;
    pLoop_->load_calibration_from_device();
    pLoop_->start();
    logmsg("started loop");
    // todo - need device name
    efd_.fireConnectEvent(usbDevice()->name(), Callback::DeviceType::PICO);
    return true;
}

bool EF_Pico::stop() {
    if (pLoop_ == NULL) return false;
    //    pLoop_->stop(); //??
    //    logmsg("stopped loop");

    for (unsigned k = 0; k < PICO_MAINKEYS + 4; k++) {
        pLoop_->set_led(k, 0);
    }
    // pLoop_->msg_flush();

    return EF_Harp::stop();
}

void EF_Pico::restartKeyboard() {
    if (pLoop_ != NULL) {
        logmsg("restarting pico keyboard....");
    }
}

void EF_Pico::fireKeyEvent(unsigned long long t, unsigned course, unsigned key, bool a, float p, float r, float y) {
    if (course > 0) {
        if (lastMode_[key] == p) return;
        lastMode_[key] = p;
    }

    EF_Harp::fireKeyEvent(t, course, key, a, p, r, y);
}

bool EF_Pico::poll(long long t) {
    if (!EF_Harp::poll(t)) return false;
    pLoop_->poll(t);
    // pLoop_->msg_flush();
    return true;
}

void EF_Pico::setLED(unsigned course, unsigned key, unsigned colour) {
    if (pLoop_ == NULL) return;
    unsigned keynum = course * PICO_MAINKEYS + key;
    pLoop_->set_led(keynum, colour);
}

bool EF_Pico::loadPicoFirmware(const std::string &usbdev) {
    std::string ihxFile;
    if (usbdev.size() == 0) {
        pic::logmsg() << "no pico connected/powered on?";
        return false;
    } else {
        ihxFile = PICO_FIRMWARE;
    }

    pic::usbdevice_t *pDevice;
    try {
        pDevice = new pic::usbdevice_t(usbdev.c_str(), 0);
        //		pDevice->set_power_delegate(0);
    } catch (std::exception &e) {
        char buf[1024];
        snprintf(buf, 1024, "unable to open device: %s ", e.what());
        logmsg(buf);
        return false;
    }
    return loadFirmware(pDevice, ihxFile);
}

bool EF_Pico::checkFirmware(const std::string &usbdevice) {
    devcheck f(usbdevice);
    pic::usbenumerator_t::enumerate(BCTPICO_USBVENDOR, PICO_PRE_LOAD, pic::f_string_t::method(&f, &devcheck::found));

    // we need to load firmware
    if (f.found_) {
        logmsg("pico loading firmware...");
        if (loadPicoFirmware(usbdevice)) {
            logmsg("pico firmware loaded");

            f.found_ = false;
            pic::usbenumerator_t::enumerate(BCTPICO_USBVENDOR, PRODUCT_ID_PICO, pic::f_string_t::method(&f, &devcheck::found));
            for (int i = 0; i < 10 && f.found_ == false; i++) {
                logmsg("attempting to find pico...");
                pic_microsleep(1000000);
                // can take a few seconds for pico to reregister itself
                pic::usbenumerator_t::enumerate(BCTPICO_USBVENDOR, PRODUCT_ID_PICO, pic::f_string_t::method(&f, &devcheck::found));
            }

            if (f.found_) {
                char buf[1024];
                snprintf(buf, 1024, "pico loaded dev: %s ", usbdevice.c_str());
                logmsg(buf);
            } else {
                logmsg("error: pico post firmware not found");
                return false;
            }
        } else {
            logmsg("error loading pico");
            return false;
        }
    }
    return true;
}

std::vector<std::string> EF_Pico::availableDevices() {
    std::vector<std::string> devList;
    devfinder f(devList);
    pic::usbenumerator_t::enumerate(BCTPICO_USBVENDOR, PICO_PRE_LOAD, pic::f_string_t::method(&f, &devfinder::found));
    pic::usbenumerator_t::enumerate(BCTPICO_USBVENDOR, PRODUCT_ID_PICO, pic::f_string_t::method(&f, &devfinder::found));
    return devList;
}

// PicoDelegate
void EF_Pico::Delegate::kbd_dead(unsigned reason) {
    parent_.fireDeadEvent(reason);
    if (!parent_.stopping()) parent_.restartKeyboard();
}

void EF_Pico::Delegate::kbd_key(unsigned long long t, unsigned key, bool a, unsigned p, int r, int y) {
    // pic::logmsg() << "kbd_key fire" << key << " p " << p << " r " << r << " y " << y;
    float fp = pToFloat(p);
    float fr = rToFloat(r);
    float fy = yToFloat(y);
    parent_.fireKeyEvent(t, 0, key, a, fp, fr, fy);
}

void EF_Pico::Delegate::kbd_raw(bool resync, const pico::active_t::rawkbd_t &) {
}

void EF_Pico::Delegate::kbd_strip(unsigned long long t, unsigned s) {
    // pic::logmsg() << "kbd_strip s " << s;
    if (--s_count_ != 0) {
        return;
    }

    s_count_ = 20;

    switch (s_state_) {
        case 0:
            // idle state, wait for input above threshold
            if (s < s_threshold_) {
                s_count_ = 100;
                break;
            }

            // pic::logmsg() << "strip starting: " << s;
            s_state_ = 1;
            s_count_ = 100;
            break;

        case 1:
            // possibly starting, is it still above thresh?, if not return to idl
            if (s < s_threshold_) {
                s_state_ = 0;
            } else {
                // pic::logmsg() << "strip origin: " << s;
                s_state_ = 2;
                // origin_ = s;
                s_last_ = s;
            }
            break;

        case 2:
            // if(std::abs(long(s-last_))<200 && s>threshold_)
            // {
            //     int o = origin_;
            //     o -= s;
            //     float f = (float)o/(float)range_;
            //     float abs = (float)(s-min_)/(float)(range_/2.0f)-1.0f;
            //     if(abs>1.0f)
            //         abs=1.0f;
            //     if(abs<-1.0f)
            //         abs=-1.0f;
            //     //pic::logmsg() << "strip " << s << " -> " << f << "  range=" << range_ << "  abs=" << abs;
            //     output_.add_value(1,piw::makefloat_bounded_nb(1,-1,0,f,t));
            //     output_.add_value(2,piw::makefloat_bounded_nb(1,-1,0,abs,t));
            //     break;
            // }

            // if (std::abs(long(s) - s_last_) < 200 && s > s_threshold_) {
            //     float fv = stripToFloat(s);
            //     parent_.fireStripEvent(t, 1, fv, s_state_ != 3);
            // }
            // s_state_ = 3;
            // s_count_ = 80;

            // active, until we go below threshold
            if (s < s_threshold_ || std::abs(long(s) - s_last_) > 200) {
                // possibly ending...
                s_state_ = 3;
                s_count_ = 80;
            } else {
                // active still ...
                float fv = stripToFloat(s);
                parent_.fireStripEvent(t, 1, fv, s_state_ != 3);
            }

            break;

        case 3:
            // possibly ending...
            if (s < s_threshold_) {
                // pic::logmsg() << "strip ending";
                // float fv = stripToFloat(s_last_);
                parent_.fireStripEvent(t, 1, 0.f, s_state_ != 3);
                s_state_ = 0;
            } else {
                // ok, we can resume
                s_state_ = 2;
            }
            break;
    }

    s_last_ = s;
}

void EF_Pico::Delegate::kbd_breath(unsigned long long t, unsigned v) {
    float fv = breathToFloat(v);
    if (breathWarmUp_ < 1000) {
        breathWarmUp_++;
        if (breathWarmUp_ == 1000) {
            breathZero_ = fv;
        }
    } else {
        parent_.fireBreathEvent(t, fv - breathZero_);
    }
}

void EF_Pico::Delegate::kbd_mode(unsigned long long t, unsigned key, unsigned m) {
    float mode_value = m > 0 ? 1.0f : 0.0f;
    parent_.fireKeyEvent(t, 1, key - PICO_MAINKEYS, m, mode_value, 0, 0);
}

}  // namespace EigenApi
