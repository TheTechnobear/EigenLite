#include <eigenapi.h>
#include <picross/pic_log.h>
#include <picross/pic_time.h>
#include <string.h>

#include "ef_harp.h"
#include "eigenlite_impl.h"

#define VERSION_STRING "1.0.0"

#define VERSION_DESC "EigenLite v" VERSION_STRING " for Alpha/Tau/Pico - Author: TheTechnobear"

namespace EigenApi {
void EigenLite::logmsg(const char* msg) {
    pic::logmsg() << msg;
}

// public interface
EigenLite::EigenLite() : EigenLite(nullptr) {
}

EigenLite::EigenLite(IFW_Reader* fwReader) : pollTime_(100), fwReader_(fwReader) {
    if (fwReader_ == nullptr) {
        logmsg("create embedded fw reader");
        internalReader_ = new FWR_Embedded();
        fwReader_ = internalReader_;
    } else {
        internalReader_ = nullptr;
    }
}

EigenLite::~EigenLite() {
    destroy();
    if (internalReader_ != nullptr) {
        logmsg("destroy embedded fw reader");
        delete internalReader_;
    }
    internalReader_ = fwReader_ = nullptr;
    logmsg("EigenLite destroyed");
}

const char* EigenLite::versionString() {
    return VERSION_STRING;
}

void EigenLite::addCallback(EigenApi::Callback* api) {
    if (callbacksIterating_ && callbacksWriteIdx_ == callbacksReadIdx_) {
        callbacksWriteIdx_ = 1 - callbacksReadIdx_;
        callbacks_[callbacksWriteIdx_] = callbacks_[callbacksReadIdx_];
    }
    auto& wb = callbacks_[callbacksWriteIdx_];
    for (auto cb : wb) {
        if (cb == api) { return; }
    }
    wb.push_back(api);
    if (!callbacksIterating_) { callbacksReadIdx_ = callbacksWriteIdx_; }
}

void EigenLite::removeCallback(EigenApi::Callback* api) {
    if (callbacksIterating_ && callbacksWriteIdx_ == callbacksReadIdx_) {
        callbacksWriteIdx_ = 1 - callbacksReadIdx_;
        callbacks_[callbacksWriteIdx_] = callbacks_[callbacksReadIdx_];
    }
    auto& wb = callbacks_[callbacksWriteIdx_];
    for (auto it = wb.begin(); it != wb.end(); ++it) {
        if (*it == api) {
            wb.erase(it);
            if (!callbacksIterating_) { callbacksReadIdx_ = callbacksWriteIdx_; }
            return;
        }
    }
}

void EigenLite::clearCallbacks() {
    callbacks_[0].clear();
    callbacks_[1].clear();
    callbacksReadIdx_ = 0;
    callbacksWriteIdx_ = 0;
}

volatile bool discoverProcessRun = true;

void* discoverProcess(void* pthis) {
    auto pThis = static_cast<EigenLite*>(pthis);
    while (discoverProcessRun) {
        if (pThis->checkUsbDev()) {
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
    if (!usbDevCheckSpinLock.test_and_set()) {
        // any change in basestation setup?
        auto baseUSBDevList = EF_BaseStation::availableDevices();
        if (availableBaseStations_.size() == baseUSBDevList.size()) {
            int i = 0;
            for (auto& usbdev : baseUSBDevList) {
                if (availableBaseStations_[i] != usbdev) {
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
        if (availablePicos_.size() == picoUSBDevList.size()) {
            int i = 0;
            for (auto& usbdev : picoUSBDevList) {
                if (availablePicos_[i] != usbdev) {
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

        usbDevCheckSpinLock.clear();
        return true;
    }
    return false;
}

void EigenLite::setDeviceFilter(unsigned allBasePico, unsigned devenum) {
    filterAllBasePico_ = allBasePico;
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
        } catch (std::system_error&) { logmsg("warn error whilst joining to discover thread"); }
    }
    for (auto device : devices_) { device->destroy(); }
    devices_.clear();
    return true;
}

void EigenLite::deviceDead(const char* dev, unsigned reason) {
    deadDevices_.insert(dev);
}

bool EigenLite::connectNewBaseStation() {
    bool newDevice = true;
    std::string usbDev;
    if (filterDeviceEnum_ == 0) {
        for (const auto& usbDevStr : availableBaseStations_) {
            newDevice = true;
            for (auto dev : devices_) {
                if (usbDevStr == dev->usbDevice()->name()) {
                    newDevice = false;
                    break;
                }
            }
            if (newDevice) {
                usbDev = usbDevStr;
                break;
            }
        }
    } else {
        int filterIdx = filterDeviceEnum_ - 1;  // 0 = first
        if (filterIdx < availableBaseStations_.size()) {
            newDevice = true;
            const auto& usbDevStr = availableBaseStations_[filterIdx];
            for (auto dev : devices_) {
                if (usbDevStr == dev->usbDevice()->name()) {
                    newDevice = false;
                    break;
                }
            }
            if (newDevice) usbDev = usbDevStr;
        }
    }
    if (newDevice) {
        char logbuf[100];
        snprintf(logbuf, 100, "new base %s", usbDev.c_str());
        logmsg(logbuf);

        EF_Harp* pDevice = new EF_BaseStation(*this);
        if (pDevice->create(usbDev)) {
            snprintf(logbuf, 100, "created base %s", pDevice->usbDevice()->name());
            logmsg(logbuf);
            devices_.push_back(pDevice);
            pDevice->start();
            return true;
        }
    }
    return false;
}
bool EigenLite::connectNewPico() {
    bool newDevice = true;
    std::string usbDev;
    if (filterDeviceEnum_ == 0) {
        for (const auto& usbDevStr : availablePicos_) {
            newDevice = true;
            for (auto dev : devices_) {
                if (usbDevStr == dev->usbDevice()->name()) {
                    newDevice = false;
                    break;
                }
            }
            if (newDevice) {
                usbDev = usbDevStr;
                break;
            }
        }
    } else {
        int filterIdx = filterDeviceEnum_ - 1;  // 0 = first
        if (filterIdx < availableBaseStations_.size()) {
            newDevice = true;
            const auto& usbDevStr = availablePicos_[filterIdx];
            for (auto dev : devices_) {
                if (usbDevStr == dev->usbDevice()->name()) {
                    newDevice = false;
                    break;
                }
            }
            if (newDevice) usbDev = usbDevStr;
        }
    }

    if (newDevice) {
        char logbuf[100];
        snprintf(logbuf, 100, "new pico %s", usbDev.c_str());
        logmsg(logbuf);
        EF_Harp* pDevice = new EF_Pico(*this);
        if (pDevice->create(usbDev)) {
            snprintf(logbuf, 100, "created pico %s", pDevice->usbDevice()->name());
            logmsg(logbuf);
            devices_.push_back(pDevice);
            pDevice->start();
            return true;
        }
    }
    return false;
}


bool EigenLite::poll() {
    // check for device changes
    if (!usbDevCheckSpinLock.test_and_set()) {
        if (usbDevChange_) {
            bool newDevice = false;
            beginCallbackIteration();
            for (auto& cb : callbacks_[callbacksReadIdx_]) {
                cb->beginDeviceInfo();
                int i = 0;
                for (auto& usb : availablePicos_) {
                    i++;
                    cb->deviceInfo(true, i, usb.c_str());
                }

                i = 0;
                for (auto& usb : availableBaseStations_) {
                    i++;
                    cb->deviceInfo(false, i, usb.c_str());
                }
                cb->endDeviceInfo();
            }
            endCallbackIteration();

            if (filterAllBasePico_ == 0 || filterAllBasePico_ == 1) {  // base
                newDevice = connectNewBaseStation();
            }  // base
            if (!newDevice && (filterAllBasePico_ == 0 || filterAllBasePico_ == 2)) {  // pico
                newDevice = connectNewPico();
            }  // pico


            if (newDevice) {
                usbDevCheckSpinLock.clear();
                return true;
            }
            usbDevChange_ = false;
        }  // usbDevChange_
        usbDevCheckSpinLock.clear();
    }  // usbDevCheckSpinLock - test n' set, else just wait till next time!

    // check for dead devices
    while (deadDevices_.size() > 0) {
        std::string usbname = *deadDevices_.begin();

        std::vector<EF_Harp*>::iterator iter;
        bool found = false;
        for (iter = devices_.begin(); !found && iter != devices_.end(); iter++) {
            EF_Harp* pDevice = *iter;
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

    // poll each device
    long long t = pic_microtime();
    long long diff = t - lastPollTime_;

    if (diff > pollTime_) {
        lastPollTime_ = t;
        bool ret = true;
        for (auto pDevice : devices_) { ret &= pDevice->poll(0); }
        return ret;
    }
    return false;
}

void EigenLite::setPollTime(unsigned pollTime) {
    pollTime_ = pollTime;
}

void EigenLite::fireBeginDeviceInfo() {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->beginDeviceInfo(); }
    endCallbackIteration();
}

void EigenLite::fireDeviceInfo(bool isPico, unsigned devNum, const char* dev) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->deviceInfo(isPico, devNum, dev); }
    endCallbackIteration();
}

void EigenLite::fireEndDeviceInfo() {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->endDeviceInfo(); }
    endCallbackIteration();
}

void EigenLite::fireConnectEvent(const char* dev, Callback::DeviceType dt) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->connected(dev, dt); }
    endCallbackIteration();
}

void EigenLite::fireDisconnectEvent(const char* dev) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->disconnected(dev); }
    endCallbackIteration();
}

void EigenLite::fireKeyEvent(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, float p,
                             float r, float y) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->key(dev, t, course, key, a, p, r, y); }
    endCallbackIteration();
}

void EigenLite::fireButtonEvent(const char* dev, unsigned long long t,  unsigned key, bool a) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->button(dev, t, key, a); }
    endCallbackIteration();
}


void EigenLite::fireBreathEvent(const char* dev, unsigned long long t, float val) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->breath(dev, t, val); }
    endCallbackIteration();
}

void EigenLite::fireStripEvent(const char* dev, unsigned long long t, unsigned strip, float val, bool a) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->strip(dev, t, strip, val, a); }
    endCallbackIteration();
}

void EigenLite::firePedalEvent(const char* dev, unsigned long long t, unsigned pedal, float val) {
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->pedal(dev, t, pedal, val); }
    endCallbackIteration();
}

void EigenLite::fireDeadEvent(const char* dev, unsigned reason) {
    deviceDead(dev, reason);
    beginCallbackIteration();
    for (auto cb : callbacks_[callbacksReadIdx_]) { cb->dead(dev, reason); }
    endCallbackIteration();
}

void EigenLite::setLED(const char* dev, unsigned course, unsigned key, unsigned colour) {
    for (auto pDevice : devices_) {
        if (dev == NULL || dev == pDevice->name() || strcmp(dev, pDevice->name()) == 0) {
            pDevice->setLED(course, key, colour);
        }
    }
}
}  // namespace EigenApi
