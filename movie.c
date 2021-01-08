//
//  movie.c
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include "movie.h"

void movie_init(movie* m)
{
    for (int i = 0; i < 2; i++)
    {
        struct port* p = &m->port[i];
        p->type = NONE;
        p->num_controllers = 0;
        p->controller_num_indices = 0;
        p->input = NULL;
    }
    
    m->reset = NULL;
    m->power = NULL;
    
    m->gametype = SNES_NTSC;
    m->gamename = NULL;
    m->authors = NULL;
    m->frames = 0;
    m->rerecords = 0;
    
    m->settings = 0;
}

void movie_print_frame(movie m, unsigned int frame)
{
    if (frame > m.frames)
        return;
    
    for (unsigned int p = 0; p < 2; p++)
    {
        struct port port = m.port[p];
        
        for (unsigned int c = 0; c < port.num_controllers; c++)
        {
            for (unsigned int i = 0; i < port.controller_num_indices; i++)
            {
                if (port_index_value(port, frame, c, i) > 0)
                    printf("%c", gamepad_format[i]);
                else
                    printf(".");
            }
            
            printf("|");
        }
    }
        
    printf("\n");
}
