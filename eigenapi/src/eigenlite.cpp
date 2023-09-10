#include <eigenapi.h>
#include "eigenlite_impl.h"
#include "ef_harp.h"
#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <string.h>

#define VERSION_STRING "1.0.0"

#define VERSION_DESC "EigenLite v" VERSION_STRING " for Alpha/Tau/Pico - Author: TheTechnobear"

namespace EigenApi {
    void EigenLite::logmsg(const char *msg) {
        pic::logmsg() << msg;
    }

    // public interface
    EigenLite::EigenLite() : EigenLite(nullptr) {
    }

    EigenLite::EigenLite(IFW_Reader* fwReader) 
        : fwReader_(fwReader), pollTime_(100) {
        if(fwReader_==nullptr) {
            internalReader_ = new FWR_Embedded();
            fwReader_ = internalReader_;
        }
    }

    EigenLite::~EigenLite() {
        destroy();
        delete internalReader_;
        internalReader_ = fwReader_ = nullptr;
    }


    const char* EigenLite::versionString() {
        return VERSION_STRING;
    }


    void EigenLite::addCallback(EigenApi::Callback *api) {
        // do not allow callback to be added twice
        for (auto cb: callbacks_) {
            if (cb == api) {
                return;
            }
        }
        callbacks_.push_back(api);
    }

    void EigenLite::removeCallback(EigenApi::Callback *api) {
        std::vector<Callback *>::iterator iter;
        for (iter = callbacks_.begin(); iter != callbacks_.end(); iter++) {
            if (*iter == api) {
                callbacks_.erase(iter);
                return;
            }
        }
    }

    void EigenLite::clearCallbacks() {
        std::vector<Callback *>::iterator iter;
        while (!callbacks_.empty()) {
            callbacks_.pop_back();
        }
    }

    volatile bool discoverProcessRun = true;

    void *discoverProcess(void *pthis) {
        auto pThis = static_cast<EigenLite *>(pthis);
        while (discoverProcessRun) {
            if(pThis->checkUsbDev()) {
                // 10seconds
                pic_microsleep(10 * 100000); 
            } else {
                // failed as poll() in progress
                // try again quickly, spinlock
                // 1 mS
                pic_microsleep(1000);
            } 
        }
        return nullptr;
    }



    bool EigenLite::checkUsbDev() {
        if(usbDevCheckSpinLock.test_and_set()) {
            if (!usbDevChange_) {

                // any change in basestation setup?
                auto baseUSBDevList = EF_BaseStation::availableDevices();
                if(availableBaseStations_.size() == baseUSBDevList.size()) {
                    int i=0;
                    for(auto& usbdev : baseUSBDevList) {
                        if(availableBaseStations_[i] != usbdev) {
                            usbDevChange_ |= true;
                            availableBaseStations_ = baseUSBDevList;
                            break;
                        }
                        i++;
                    }
                } else {
                    usbDevChange_ |= true;
                    availableBaseStations_ = baseUSBDevList;
                }

                // any change in pico setup?
                auto picoUSBDevList = EF_Pico::availableDevices();
                if(availablePicos_.size() == picoUSBDevList.size()) {
                    int i=0;
                    for(auto& usbdev : picoUSBDevList) {
                        if(availablePicos_[i] != usbdev) {
                            usbDevChange_ |= true;
                            availablePicos_ = picoUSBDevList;
                            break;
                        }
                        i++;
                    }
                } else {
                    usbDevChange_ |= true;
                    availablePicos_ = picoUSBDevList;
                }
            }
            usbDevCheckSpinLock.clear();
            return true;
        }
        return false;
    }

    void EigenLite::setDeviceFilter(bool baseStation, unsigned devenum) {
        filterBaseStationOrPico_ = baseStation;
        filterDeviceEnum_ = devenum;
    }


    bool EigenLite::create() {
        logmsg(VERSION_DESC);
        logmsg("start EigenLite");
        pic_init_time();
        discoverProcessRun = true;
        discoverThread_ = std::thread(discoverProcess, this);
        pic_set_foreground(true);
        lastPollTime_ = 0;
        usbDevChange_ = false;
        return true;
    }

    bool EigenLite::destroy() {
        discoverProcessRun = false;
        if (discoverThread_.joinable()) {
            try {
                discoverThread_.join();
            } catch (std::system_error &) {
                logmsg("warn error whilst joining to discover thread");
            }

        }
        for (auto device: devices_) {
            device->destroy();
        }
        devices_.clear();
        return true;
    }

    void EigenLite::deviceDead(const char *dev, unsigned reason) {
        deadDevices_.insert(dev);
    }


    bool EigenLite::poll() {
        if(usbDevCheckSpinLock.test_and_set()) {
            if (usbDevChange_) {

                bool newPico = false;
                bool newBase = false;
                std::string picoUSBDev, baseUSBDev;


                if(filterDeviceEnum_ == 0) {
                    // attempt to connect to all devices
                    if(availablePicos_.size()>0) {
                        newPico = true;
                        picoUSBDev = availablePicos_[0];
                    }

                    if(availableBaseStations_.size()>0) {
                        newBase = true;
                        baseUSBDev = availableBaseStations_[0];
                    }
                } else {
                    // todo 
                }


                // check not already connected.
                for (auto dev: devices_) {
                    if (picoUSBDev == dev->usbDevice()->name()) {
                        newPico = false;
                    }
                    if (baseUSBDev == dev->usbDevice()->name()) {
                        newBase = false;
                    }
                }

                EF_Harp *pDevice = nullptr;
                if (newPico) {
                    char logbuf[100];
                    snprintf(logbuf, 100, "new pico %s", picoUSBDev.c_str());
                    logmsg(logbuf);

                    pDevice = new EF_Pico(*this);
                    if (pDevice->create()) {
                        char logbuf[100];
                        snprintf(logbuf, 100, "created pico %s", pDevice->usbDevice()->name());
                        logmsg(logbuf);
                        devices_.push_back(pDevice);
                        pDevice->start();
                    }
                }
                if (newBase) {
                    char logbuf[100];
                    snprintf(logbuf, 100, "new base %s", baseUSBDev.c_str());
                    logmsg(logbuf);

                    pDevice = new EF_BaseStation(*this);
                    if (pDevice->create()) {
                        devices_.push_back(pDevice);
                        pDevice->start();
                    }
                }

                usbDevChange_ = false;
                usbDevCheckSpinLock.clear();

                if (newBase || newPico) return true;
            }
        } // test n' set, else just wait till next time! 

        while (deadDevices_.size() > 0) {
            std::string usbname = *deadDevices_.begin();

            std::vector<EF_Harp *>::iterator iter;
            bool found = false;
            for (iter = devices_.begin(); !found && iter != devices_.end(); iter++) {
                EF_Harp *pDevice = *iter;
                if (pDevice->name() == usbname) {
                    char logbuf[100];
                    snprintf(logbuf, 100, "destroy device %s", usbname.c_str());
                    logmsg(logbuf);
                    pDevice->destroy();
                    found = true;
                    devices_.erase(iter);
                }
            }
            deadDevices_.erase(usbname);
        }

        long long t = pic_microtime();
        long long diff = t - lastPollTime_;

        if (diff > pollTime_) {
            lastPollTime_ = t;
            bool ret = true;
            for (auto pDevice: devices_) {
                ret &= pDevice->poll(0);
            }
            return ret;
        }
        return false;

    }

    void EigenLite::setPollTime(unsigned pollTime) {
        pollTime_ = pollTime;
    }

    void EigenLite::fireNewDeviceEvent(Callback::DeviceType dt, const char* name) {
        for (auto cb: callbacks_) {
            cb->newDevice(dt, name);
        }
    }


    void EigenLite::fireConnectEvent(const char *dev, Callback::DeviceType dt, const char* name) {
        for (auto cb: callbacks_) {
            cb->connected(dev, dt, name);
        }
    }

    void EigenLite::fireDisconnectEvent(const char *dev) {
        for (auto cb: callbacks_) {
            cb->disconnected(dev);
        }
    }

    void
    EigenLite::fireKeyEvent(const char *dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p,
                            int r, int y) {
        for (auto cb: callbacks_) {
            cb->key(dev, t, course, key, a, p, r, y);
        }
    }

    void EigenLite::fireBreathEvent(const char *dev, unsigned long long t, unsigned val) {
        for (auto cb: callbacks_) {
            cb->breath(dev, t, val);
        }
    }

    void EigenLite::fireStripEvent(const char *dev, unsigned long long t, unsigned strip, unsigned val, bool a) {
        for (auto cb: callbacks_) {
            cb->strip(dev, t, strip, val, a);
        }
    }

    void EigenLite::firePedalEvent(const char *dev, unsigned long long t, unsigned pedal, unsigned val) {
        for (auto cb: callbacks_) {
            cb->pedal(dev, t, pedal, val);
        }
    }

    void EigenLite::fireDeadEvent(const char *dev, unsigned reason) {
        deviceDead(dev, reason);
        for (auto cb: callbacks_) {
            cb->dead(dev, reason);
        }
    }

    void EigenLite::setLED(const char *dev, unsigned course, unsigned int key, unsigned int colour) {
        for (auto pDevice: devices_) {
            if (dev == NULL || dev == pDevice->name() || strcmp(dev, pDevice->name()) == 0) {
                pDevice->setLED(course, key, colour);
            }
        }
    }
} // namespace EigenApi

