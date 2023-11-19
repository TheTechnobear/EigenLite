## Development notes

Some general notes about development of eigenapi, its ethos and future directions

# Goals

### A) easy to use, simple api
end user includes one header file, and use single EigenApi class
only use basic types 

### B) simple wrapper around eigenD code
a 'fairly' raw wrapper around the EigenD source code, present an un-opinionated view of this code.
expose all possibilities eigenD code offers, but easier.
keep it light, so less possibilties of bugs.



# EigenLite vs EigenD/Drivers behaviour

pre 1.x , I considered EigenLite to be a thin wrapper around the RAW Eigenharp **"drivers"** (lib_pico, lib_alpha2).
this allowed applications to then implement higher level functionality with flexibility.
also this thin layer meant any 'oddities' seen were likely to be part of the hardware not EigenLite.

however, this developing applications was not as simple as it could be, since device differences need to be taken into account.

with 1.x and beyond, Ive moved the abstraction to a slightly higher level.
basically, EigenLite is now more like the "Module" level interface that EigenD presents via pico_mananger (plg_pkbd) and alpha_manager (plg_keyboard)
this is where the 'differences' are isolated, and we start to see floats (unipolar and bipolar) rather than raw data. ie. the raw device data



# Current Limitations 
- no headphone support (alpha/tau only)
- no microphone support (alpha only)
- some value hysterisis is simplified
- abs/relative values for strips


# Plans / Ideas / Clarification

### Hysterisis simplifications 
EigenD has alot of configurable windows for things like axis, rounding, stepping etc.
Ive simplified alot of this in EigenLite, as Ive never seen it used - nor found its required.
so, my implementation is more based on what I have **practically** seen required. 
e.g. where Ive seen a 'gain' is needed, Ive gone to EigenD to see what is used there.
so rather than a strict copy of the implementation the basics (and values) are used.
If I get feedback for certain use cases I will 'review' this.


### Pico key press lights
on the Tau/Alpha the basestation firmware will make a key orange when pressed - non-configurable (afaik)
the Pico does NOT exhibit this behave, Im considering implementing this in EigenLite to remove this oddity.
this is being done in EigenD , so seems consistent with new behaviour limits of EigenLite


### auto connect
EigenLite has been automatically connecting to eigenharps as it sees them. this is why we need the 'filter'
perhaps I can invert the api, so that applications are informed of a Eigenharp's present.but then you explicitly connect to it.
this might mean we do not need the 'filter', the choice of which Eigenharp to connect becomes the applications responsiblities

this needs to be carefully considered, its a bit tricky to do due to the way the basestation works.


### Abs/Rel Strip values
EigenD exposes (at module level) abs and relative values - I supply ABS only, its up to the application to calculate relative if required.



### headphone support
mainly because originaly I used the api outside of audio rate applications.
I'd like to revisit, but there could be complications as Ive never checked its being initialised correctly etc.

### microphone support
similar to headphone support, howver, its complicated as I have no access to an alpha microphone to develop and test with.


