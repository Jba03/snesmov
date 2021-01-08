//
//  port.h
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#ifndef port_h
#define port_h

#include <stdio.h>

enum port_type
{
    NONE,
    GAMEPAD,    /* Standard gamepad, 12-button */
    GAMEPAD16,  /* Standard gamepad, 16-button */
    YGAMEPAD16, /* Y-cabled gamepads */
    MULTITAP,   /* Multitap, four gamepads in one port */
    MULTITAP16, /* Multitap, four 16-button gamepads in one port  */
    MOUSE,      /* Super NES mouse */
    SUPERSCOPE, /* Super Scope */
    JUSTIFIER,  /* Konami justifier */
    JUSTIFIERS,
};

struct port
{
    int framecount;
    
    enum port_type type;
    int num_controllers;
    int controller_num_indices;
    
    /*
     * Movie input
     *
     * Standard gamepad format:
     *      BYsSudlrAXLR{0123} (lsnes format)
     */
    unsigned char* input;
};

/* Allocates movie input for the specified port */
struct port port_alloc(enum port_type type, int frames);

/* Returns the value of a specified port index */
unsigned char port_index_value(struct port p, int frame, int controller, int index);

/* Sets the value of a specified port index */
void set_port_index(struct port p, int frame, int controller, int index, unsigned char value);

/* Finds a port type based on the number of controllers and controller buttons/indices */
enum port_type find_port_type(int num_controllers, int controller_num_indices);

#endif /* port_h */
