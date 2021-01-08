# snesmov  

A program to convert between different SNES movie file formats.
  
Supported formats:

        .lsmv (lsnes, reading + writing)
        .bk2  (BizHawk, reading)
        .smv  (Snes9x, reading)


Movies that use other controllers than the standard *12* or *[16-button]* gamepad, are currently not supported.

## syncing movies
When converting from *.bk2* to *.lsmv*, remove one initial frame.  
~~When converting from *.lsmv* to *.bk2*, add one initial blank frame.~~

There is a small chance that movies converted between these two formats will desync. For example, http://tasvideos.org/6568S.html desyncs after the first "world", while http://tasvideos.org/6952S.html syncs perfectly throughout the entire movie. *(bk2 -> lsmv)*

Converted Snes9x movie files desync more often than ones produced in emulators that use the bsnes or higan cores, due to emulation inaccuracy. Desyncs seem to occur most commonly during transitions, such as fading-effects or level changes.


### dependencies
    libzip: https://libzip.org

*Todo: custom zip library?*
