# snesmov  

A program to convert between different SNES movie file formats.
  
Supported formats:

        .lsmv (lsnes, reading + writing)
        .bk2  (BizHawk, reading)
        .smv  (Snes9x, reading)


Movies that use other controllers than the standard *12* or *[16-button]* gamepad, are currently not supported.

## Syncing movies
When converting from *.bk2* to *.lsmv*, remove one initial frame.  
~~When converting from *.lsmv* to *.bk2*, add one initial blank frame.~~

Converted Snes9x movie files desync more often than ones produced in emulators that use the bsnes or higan cores, due to emulation inaccuracy. Desyncs seem to occur most commonly during transitions, such as fading-effects or level changes.


### Dependencies
    libzip: https://libzip.org

*Todo: custom zip library?*
