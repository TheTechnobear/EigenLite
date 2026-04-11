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


# Technical notes

EigenLite is derived from the much larger EigenD project, EigenD is open source
https://github.com/thetechnobear/EigenD

Its purpose is to give a programmatic api for teh Eigenharps without the overhead and dependancies of EigenD, and more modern toolset.
so we use Cmake, not Scon, and have no dependancy on Python

essentially, I have copied the low level Eigenharp code, then I have adapted python to C++ where necessary.

as I also maintain EigenD, I want to have as little divergence on this low level code as possible.
even if this (for now) means putting up with compiler warnings.

the primary directories from EigenD are
- ./eigenapi/lib_alpha2
- ./eigenapi/lib_pico
- ./eigenapi/picross

the main eigenlite api is in ./eigenapi/src, and presents the abstraction others will use

also we need to handle lib_picoencoder as a special case.

lib_picoencoder is closed source, we only have access to the prebuilt libraries.
to allow this project to be under an open source library, Ive created a compatible 'stub' library that is included in this project.
which means the project is fully buildable without closed source libraries etc.


# EigenD 
I am the primary maintainer of EigenD, and the latest code and version is in my repo  https://github.com/thetechnobear/EigenD
(the original EigenLabs version is outdated, and without a maintainer, so there is no point in pushing changes, or sending PRs)

the current release is on the 2.2 branch - and this code is based on that release.
however, Ive been working active on a new version 3.0 which contains many modernisations - this is on the 3.0 branch
it can be found locally on ~/projects/EigenD

generally, if have improvements for the EigenD code (as detailed above), it should be in done in the EigenD project.
and then I 'copy' over to this project.

I want to retain the indepenence of this project to EigenD, do I do not want to include as a submodule etc.

TODO: 
I really should have an automated task to 
- compare EigenD and EigenLite sources
- allow me to copy as necessary
- potentially patch, if they need changes that are incompatible with EigenD (Ive avoided this so far)

document these shared files
also, for some of the EigenLite specific code, I should document / link to the equivelent C++ / python code in EigenD.
ideally, we could then check there is equivilence where necessary.

# Eigenharp firmware
Eigenharps (pico, tau, alpha) all have a 2 stage intialisation process.
- initially power on with a bootloader firmware, as a specific usb product type
- EigenLite/EigenD then uploads the 'real' firmware to them, and they re-initialise as the  usb device that the users sees, and that we interact with.

traditionally, this firmware was held in ihx files, which can be found in this project.
in recent versions, Ive converted these into C++ files which can be (optionally, by default) statically linked into eigenlite.

this is so that EigenLite can run without needing to be concerned about filesystems, and can be used as at static library.

as the Eigenharp firmware is no longer being developed, statically linking poses no disadvantages to the end user/developer



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


