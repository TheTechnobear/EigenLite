#include <eigenapi.h>
#include "eigenlite_impl.h"

#include <picross/pic_config.h>

#define VERSION_STRING "eigenlite v0.4 Alpha/Tau/Pico, experimental - Author: TheTechnobear"

#include <picross/pic_time.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>
#include <string.h>


namespace EigenApi
{


void EigenLite::logmsg(const char* msg)
{
    pic::logmsg() << msg;
}


// public interface

EigenLite::EigenLite(const char* fwDir) : fwDir_(fwDir),pollTime_(1000)
{
}

EigenLite::~EigenLite()
{
    destroy();
}

void EigenLite::addCallback(EigenApi::Callback* api)
{
    // do not allow callback to be added twice
    std::vector<Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        if(*iter==api)
        {
            return;
        }
    }
    callbacks_.push_back(api);
}

void EigenLite::removeCallback(EigenApi::Callback* api)
{
    std::vector<Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        if(*iter==api)
        {
            callbacks_.erase(iter);
            return;
        }
    }
}

void EigenLite::clearCallbacks()
{
    std::vector<Callback*>::iterator iter;
    while(!callbacks_.empty())
    {
        callbacks_.pop_back();
    }
}



bool EigenLite::create()
{
    logmsg(VERSION_STRING);
    logmsg("create EigenLite");
    pic_init_time();
    pic_set_foreground(true);
    if(EF_BaseStation::isAvailable()) 
    {
		EF_Harp *pDevice = new EF_BaseStation(*this, fwDir_);
        pDevice->create();
		devices_.push_back(pDevice);
    }
    if(EF_Pico::isAvailable()) 
    {
		EF_Harp *pDevice = new EF_Pico(*this, fwDir_);
        pDevice->create();
		devices_.push_back(pDevice);
    }
    return devices_.size() > 0;
}

bool EigenLite::destroy()
{
    logmsg("destroy EigenLite....");
    //? stop();
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp *pDevice = *iter;
		ret &= pDevice->destroy();
	}	
	devices_.clear();
    logmsg("destroyed EigenLite");
    return true;
}

bool EigenLite::start()
{
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp *pDevice = *iter;
		ret &= pDevice->start();
	}	
    return true;
}

bool EigenLite::stop()
{
    bool ret = true;
    std::vector<EF_Harp*>::iterator iter;
	for(iter=devices_.begin();iter!=devices_.end();iter++)
	{    
		EF_Harp* pDevice = *iter;
		ret &= pDevice->stop();
	}	

    clearCallbacks();
    return true;
}

bool EigenLite::poll()
{
    bool ret=true;
    std::vector<EF_Harp*>::iterator iter;
    for(iter=devices_.begin();iter!=devices_.end();iter++)
    {
        EF_Harp *pDevice = *iter;
        ret &= pDevice->poll(pollTime_);
    }
    return ret;
}

void EigenLite::setPollTime(unsigned pollTime) {
    pollTime_ = pollTime;
}

    
void EigenLite::fireDeviceEvent(const char* dev, 
                                 Callback::DeviceType dt, int rows, int cols, int ribbons, int pedals)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
		EigenApi::Callback *cb=*iter;
		cb->device(dev, dt, rows, cols, ribbons, pedals);
	}
}
void EigenLite::fireKeyEvent(const char* dev,unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->key(dev, t, course, key, a, p, r, y);
    }
}
    
void EigenLite::fireBreathEvent(const char* dev, unsigned long long t, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->breath(dev, t, val);
    }
}
    
void EigenLite::fireStripEvent(const char* dev, unsigned long long t, unsigned strip, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->strip(dev, t, strip, val);
    }
}
    
void EigenLite::firePedalEvent(const char* dev, unsigned long long t, unsigned pedal, unsigned val)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->pedal(dev, t, pedal, val);
    }
}

void EigenLite::fireDeadEvent(const char* dev,unsigned reason)
{
    std::vector<EigenApi::Callback*>::iterator iter;
    for(iter=callbacks_.begin();iter!=callbacks_.end();iter++)
    {
        EigenApi::Callback *cb=*iter;
        cb->dead(dev,reason);
    }
}


    
void EigenLite::setLED(const char* dev, unsigned course, unsigned int key,unsigned int colour)
{
    std::vector<EF_Harp*>::iterator iter;
    for(iter=devices_.begin();iter!=devices_.end();iter++)
    {
        EF_Harp* pDevice = *iter;
        if(dev == NULL || dev == pDevice->name() || strcmp(dev,pDevice->name()) == 0) {
            pDevice->setLED(course, key,colour);
        }
    }
}


  
} // namespace EigenApi

