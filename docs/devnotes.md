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






