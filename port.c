//
//  port.c
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include "port.h"
#include "movie.h"
#include <stdlib.h>

#define NUM_CONTROLLERS     9

struct port_configuration
{
    enum port_type type;
    unsigned int num_controllers;
    unsigned int controller_num_indices;
};

static struct port_configuration config[NUM_CONTROLLERS] =
{
    {GAMEPAD,       1,  12},
    {GAMEPAD16,     1,  16},
    {YGAMEPAD16,    2,  16},
    {MULTITAP,      4,  12},
    {MULTITAP16,    4,  16},
    {MOUSE,         1,  0 }, /* not yet supported */
    {SUPERSCOPE,    1,  4 }, /* not yet supported */
    {JUSTIFIER,     1,  0 }, /* not yet supported */
    {JUSTIFIERS,    2,  0 }, /* not yet supported */
};

struct port port_alloc(enum port_type type, int frames)
{
    struct port p = { .type = type };
    
    if (type == NONE)
        return p;
    
    p.framecount = frames;
    
    /*
     * Find number of controllers and
     * number of buttons/indices per controller.
     */
    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        struct port_configuration cfg = config[i];
        
        if (cfg.type == type)
        {
            p.num_controllers = cfg.num_controllers;
            p.controller_num_indices = cfg.controller_num_indices;
            
            /* Allocate input */
            p.input = malloc(frames * cfg.num_controllers * cfg.controller_num_indices);
            
            for (int f = 0; f < frames; f++)
                for (int c = 0; c < cfg.num_controllers; c++)
                    for (int i = 0; i < cfg.controller_num_indices; i++)
                        set_port_index(p, f, c, i, 0);
        }
    }
    
    return p;
}

unsigned char port_index_value(struct port p, int frame, int controller, int index)
{
    return p.input[(index * p.framecount * p.num_controllers) + (controller * p.framecount) + frame];
}

void set_port_index(struct port p, int frame, int controller, int index, unsigned char value)
{
    p.input[(index * p.framecount * p.num_controllers) + (controller * p.framecount) + frame] = value;
}

enum port_type find_port_type(int num_controllers, int controller_num_indices)
{
    for (int i = 0; i < NUM_CONTROLLERS; i++)
    {
        struct port_configuration cfg = config[i];
        if (cfg.num_controllers == num_controllers &&
            cfg.controller_num_indices == controller_num_indices)
            return cfg.type;
    }
    
    return NONE;
}
