//
//  movie.h
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#ifndef movie_h
#define movie_h

#include "port.h"

typedef struct movie movie;
typedef struct movie_file movie_file;
typedef struct controller controller;

enum gametype
{
    SNES_NTSC,
    SNES_PAL,
    BSX,
    BSX_SLOTTED,
    SUFAMI_TURBO,
    SGB_NTSC,
    SGB_PAL,
};

enum settings
{
    SETTING_HARDRESET,  /* Support hard resets */
    SETTING_SAVEEVERY,  /* Emulate saving every frame */
    SETTING_RANDOMINIT, /* Random initial state */
    SETTING_COMPACT,    /* Don't support delayed resets */
    SETTING_ALTTIMINGS, /* Alternate poll timings (bsnes) */
    SETTING_BUSFIXES,   /* System bus fixes (bsnes) */
    SETTING_MOUSESPEED, /* Support mouse speeds (bsnes) */
};

struct movie
{
    struct port port[2];
    unsigned char* reset; /* Reset input */
    unsigned char* power; /* Power-on input */
    
    enum gametype gametype;
    char* gamename;
    char* authors;
    unsigned int frames;
    unsigned int rerecords;
    
    unsigned int settings;
};

struct movie_file
{
    int (*read)(movie* m, const char* filename);
    int (*write)(movie m, const char* filename);
};

void movie_init(movie* m);

void movie_print_frame(movie m, unsigned int frame);

/* File formats */
extern const movie_file lsmv;
extern const movie_file bk2;
extern const movie_file smv;

extern int offset_initial_frame;

static const char* const gamepad_format = "BYsSudlrAXLR0123";

#endif /* movie_h */
