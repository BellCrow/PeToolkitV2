# PeToolkitV2

This is simple implementation of a DLL injector for x86/x64 bit windows executeables.

The goal of this littel project was to write a DLL injector, that works without using 
LoadLibrary() in the Target Process. 

Rather than using LoadLibrary to load the module into
the target process the injector will read the target memory, 
parse the dll to inject and fix up the dependencies and relocations
locally, before writing the binary to the target process and create a thread to execute the DLL main
