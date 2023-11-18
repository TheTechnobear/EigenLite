## Development notes

Some general notes about development of eigenapi,
its ethos and future directions

# Goals

### A) easy to use, simple api
end user includes one header file, and use single EigenApi class
only use basic types 

### B) simple wrapper around eigenD code
a 'fairly' raw wrapper around the EigenD source code, present an un-opinionated view of this code.
expose all possibilities eigenD code offers, but easier.
keep it light, so less possibilties of bugs.



# EigenLite vs EigenD behaviour

pre 1.x the lightweight wrapper was something I saw as top priority.
this mean I did not add any functionality.
if you saw a bug/strange behaviour whilst using EigenLite, it was likely also the same in EigenD.

however, the flip side of this is, many 'oddities' in the the EigenD code/firmware are exposed to the (dev) users of EigenLite.
this means goal A and B can be at odds.
also, it means that my experience with handling these oddities cannot be leveraged by other devs.

my plan for 1.x is to therefore **slightly** increase the scope of EigenLite, to make the api easier to use, and to handle some oddities that exist.
I will try to includ in these dev notes where the EigenLite API is extending/changing the basic EigenD functionality




# Plans / Ideas

a) values to become floats 
```
        void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, unsigned p, int r, int y);
```

EigenD expresses pressure, roll, yaw, breath, strips (etc) as integers... so 0..4096
however, this is pretty uninituitive, esp. as some are bipolar, so are unipolar.
we can encapsulate this by moving to something like: 
```
        void key(const char* dev, unsigned long long t, unsigned course, unsigned key, bool a, float p, float r, float y);
```
where p will be 0..1.0f r = -1.0f to 1.0f y = -1.0f to 1.0f 
this also (potentially) allow us to handle calibration within the api.

b) device filter
```
        // basestation = 0, pico =1
        void setDeviceFilter(bool baseOrPico, unsigned devEnum);
```

this allows this instance of EigenLite to only connected to particular eigenharps... 
previously EigenLite used to connect to ALL eigenharps which would prevent multiple applications from 


pico/basestation are treated differently at a pretty low level - this api may changed?!


c) auto connect
EigenLite has been automatically connecting to eigenharps as it sees them. this is why we need the 'filter'
perhaps I can invert the api, so that applications are informed of a Eigenharp's present.
but then you explicitly connect to it.
this might mean we do not need the 'filter', the choice of which Eigenharp to connect becomes the applications responsiblities



# Limitations 
### headphone support
(alpha/tau only)
there is no headphone support currently, this was mainly because often the api was not used at 'audio rate'.
this needs to be revisited and investigate, I may add in the future.


### microphone support
(alpha only)
there is no microphone support currently, this was mainly because often the api was not used at 'audio rate'.
this needs to be revisited and investigate, I may add in the future.




//////////////////////////////////////


# alpha/tau
kbd_bundle.cpp

```
    static float __clip(float x,float l)
    {
        if(l<0.0001)
        {
            return 0;
        }

        x = std::max(x,-l);
        x = std::min(x,l);
        x = x*1.0/l;
        x = std::max(x,-1.0f);
        x = std::min(x,1.0f);

        return x;
    }
```


```
 void kwire_t::update(const blob_t *b)
 
        float p = float(b->p);
        float r = 2047.0-float(b->r);
        float y = 2047.0-float(b->y);

        output_.add_value(2,piw::makefloat_bounded_nb(1,0,0,p/4096.0,t));
        output_.add_value(3,piw::makefloat_bounded_nb(1,-1,0,__clip(r/1024.0,keyboard_->roll_axis_window_),t));
        output_.add_value(4,piw::makefloat_bounded_nb(1,-1,0,__clip(y/1024.0,keyboard_->yaw_axis_window_),t));
```


# pico

pkbd_bundle.cpp

check   void breath_t::update(unsigned long long t, unsigned b)

```

    void kwire_t::key(unsigned long long t, bool a, unsigned p, int r, int y)

            output_.add_value(2,piw::makefloat_bounded_nb(1.0,0.0,0.0,p/4096.0,t));
            output_.add_value(3,piw::makefloat_bounded_nb(1.0,-1.0,0.0,r2,t));
            output_.add_value(4,piw::makefloat_bounded_nb(1.0,-1.0,0.0,y2,t));

       float r2 = __clip(((float)r)/2048.0,keyboard->roll_axis_window_);
       float y2 = __clip(((float)y)/2048.0,keyboard->roll_axis_window_);
```
 



### from MetaMorph !!!
    int breathZeroPoint_ = -1;


        if (breathZeroPoint_ < 0) breathZeroPoint_ = val;
        int iVal = int(val) - breathZeroPoint_;
        if (iVal > 0) {
            iVal = (float(iVal) / (4096 - breathZeroPoint_)) * 4096;
        } else {
            iVal = (float(iVal) / breathZeroPoint_) * 4096;
        }


