# EigenLite
A lightweight interface for the Eigenharp.

# Usage
This is a developer API,  if you are looking for a project as an 'end user' see below



# Status
Under development


# Development notes
see tests directory for an example project using api and the CMakeFile to use

note: I would recommend using github submodule to include EigenLite to keep things simple and easy to update.



## building on macOS
by default this will build for your native architecture, you can change this by setting CMAKE_OSX_ARCHITECTURES

e.g.
```
cmake .. -DCMAKE_OSX_ARCHITECTURES=x86_64 
cmake .. -DCMAKE_OSX_ARCHITECTURES=arm64
```

note: 
subsequent cmake commands will use the architecture specified.

note: you can verify architecture is correct (on executables/shared libs) using `file` command



# Projects using EigenLite

Technobear's [MEC](https://github.com/TheTechnobear/MEC)
Kai Drange's [ECMapper](https://github.com/KaiDrange/ECMapper)

if you create a project using EigenLite, let me know, and I will add it here :) 

