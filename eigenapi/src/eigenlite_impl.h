#pragma once

#include <vector>
#include <picross/pic_usb.h>
#include <lib_alpha2/alpha2_active.h>
#include <lib_pico/pico_active.h>
#include <memory>
#include <thread>
#include <set>
#include <sys/fcntl.h>

namespace EigenApi
{
	class EF_Harp;
	
    class EigenLite {
    public:
        explicit EigenLite();
        explicit EigenLite(IFW_Reader *fwReader);
        virtual ~EigenLite();

        void addCallback(Callback* api);
        void removeCallback(Callback* api);
        void clearCallbacks();
        virtual bool create();
        virtual bool destroy();
        virtual bool poll();

        const char* versionString();
        void setDeviceFilter(bool baseStation, unsigned devenum);

        void setPollTime(unsigned pollTime);
        void setLED(const char* dev, unsigned course, unsigned int key, unsigned int colour);

		// logging
        static void logmsg(const char* msg);
        virtual void fireNewDeviceEvent( Callback::DeviceType dt, const char* name);


        virtual void fireConnectEvent(const char* dev, Callback::DeviceType dt, const char* name);
        virtual void fireDisconnectEvent(const char* dev);
		virtual void fireKeyEvent(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y);
        virtual void fireBreathEvent(const char* dev, unsigned long long t, unsigned val);
        virtual void fireStripEvent(const char* dev, unsigned long long t, unsigned strip, unsigned val, bool a);
        virtual void firePedalEvent(const char* dev, unsigned long long t, unsigned pedal, unsigned val);
        virtual void fireDeadEvent(const char* dev, unsigned reason);


        bool checkUsbDev();

        IFW_Reader* fwReader() { return fwReader_;}

    private:
        void deviceDead(const char* dev,unsigned reason);

        unsigned long long lastPollTime_;
        unsigned pollTime_;
        std::vector<Callback*> callbacks_;
        std::vector<EF_Harp*> devices_;
        std::thread discoverThread_;

        std::vector<std::string> availablePicos_;
        std::vector<std::string> availableBaseStations_;
        volatile bool usbDevChange_=false;

        bool filterBaseStationOrPico_=false;
        unsigned filterDeviceEnum_ = 0;

        std::set<std::string> deadDevices_;
        IFW_Reader *fwReader_;
        IFW_Reader *internalReader_;

        std::atomic_flag usbDevCheckSpinLock;
    };
}
