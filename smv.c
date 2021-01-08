//
//  smv.c
//  snesmov
//
//  Created by Jba03 on 2021-01-04.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include "movie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define log(...) \
    printf("[smv] " __VA_ARGS__);

#define CONTROLLER1 0
#define CONTROLLER2 1
#define CONTROLLER3 2
#define CONTROLLER4 3
#define CONTROLLER5 4

#define OPTIONS_SRAM    (1 << 0)
#define OPTIONS_PAL     (1 << 1)

#define R       (1 <<  4)
#define L       (1 <<  5)
#define X       (1 <<  6)
#define A       (1 <<  7)
#define RIGHT   (1 <<  8)
#define LEFT    (1 <<  9)
#define DOWN    (1 << 10)
#define UP      (1 << 11)
#define START   (1 << 12)
#define SELECT  (1 << 13)
#define Y       (1 << 14)
#define B       (1 << 15)

static int smv_read(movie* m, const char* filename)
{
    unsigned int num_controllers = 0;
    unsigned int num_mouse_ports = 0;
    unsigned int num_superscope_ports = 0;
    unsigned int num_justifier_ports = 0;
    
    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        log("failed to open file '%s'\n", filename);
        return -1;
    }
    
    if (fgetc(fp) != 'S' &&
        fgetc(fp) != 'M' &&
        fgetc(fp) != 'V' &&
        fgetc(fp) != '\x1A')
    {
        log("invalid file signature\n");
        return -1;
    }
    
    fseek(fp, 4, SEEK_SET);
    
    uint32_t version;
    fread(&version, sizeof(uint32_t), 1, fp);
    if (version == 1) log("version 1.43\n");
    if (version == 4) log("version 1.51\n"); /* 1.51: extended header */
    
    /* Skip UID */
    fgetc(fp);
    fgetc(fp);
    fgetc(fp);
    fgetc(fp);
    
    fread(&m->rerecords, sizeof(unsigned int), 1, fp);
    fread(&m->frames, sizeof(unsigned int), 1, fp);
    
    uint8_t controller_mask = fgetc(fp);
    for (int i = 0; i < 8; i++)
        if (controller_mask & (1 << i)) num_controllers++;
    
    uint8_t movie_options = fgetc(fp);
    if (!(movie_options & OPTIONS_SRAM))
    {
        log("quicksave snapshots are not supported\n")
        return -1;
    }
    
    if (movie_options & OPTIONS_PAL)
        m->gametype = SNES_PAL;
    else
        m->gametype = SNES_NTSC;
    
    uint8_t sync_options1 = fgetc(fp);
    uint8_t sync_options2 = fgetc(fp);
    
    /* Skip savestate offset */
    fgetc(fp);
    fgetc(fp);
    fgetc(fp);
    fgetc(fp);
    
    uint32_t input_offset;
    fread(&input_offset, sizeof(uint32_t), 1, fp);
    
    long s = ftell(fp);
    
    if (version == 4)
    {
        fseek(fp, 0x24, SEEK_SET);
        
        uint8_t x = fgetc(fp);
        uint8_t y = fgetc(fp);
        
        num_mouse_ports      = (x == 2) + (y == 2);
        num_superscope_ports = (x == 3) + (y == 3);
        num_justifier_ports  = (x == 2) + (y == 4);
    }
    
    fseek(fp, s, SEEK_SET);
    
    
    /* Read controller ports */
    enum controller { none, joypad, mouse, superscope, justifier, multitap5 };
    enum controller c[2] = { fgetc(fp), fgetc(fp) };
    
    for (unsigned int p = 0; p < 2; p++)
    {
        enum port_type type = NONE;
        
        switch (c[p])
        {
            case none:          type = NONE;        break;
            case joypad:        type = GAMEPAD;     break;
            case mouse:         type = MOUSE;       break; /* verify */
            case superscope:    type = SUPERSCOPE;  break; /* verify */
            case justifier:     type = JUSTIFIER;   break; /* verify */
            case multitap5:     type = MULTITAP;    break; /* verify */
            default: type = GAMEPAD; break;
        }
        
        m->port[p] = port_alloc(type, m->frames);
    }
    
    m->reset = malloc(m->frames);
    memset(m->reset, 0, m->frames);
    
    /* Seek to input buffer offset */
    fseek(fp, input_offset, SEEK_SET);
    
    /* For 1.51 */
    unsigned int framesize = 2 * num_controllers +
                             5 * num_mouse_ports +
                             6 * num_superscope_ports +
                            11 * num_justifier_ports;
    
    
    unsigned int port_size[2] =
    {
        m->port[0].num_controllers * 2, /* Two bytes per controller */
        m->port[1].num_controllers * 2,
    };
    
    for (int p = 0; p < 2; p++)
        if (port_size[p] == 0) port_size[p] = 1;
    
    unsigned int movie_size = port_size[0] * port_size[1] * (m->frames + 1);
    
    unsigned char* input_buffer = malloc(movie_size * sizeof(unsigned char));
    memset(input_buffer, 0, movie_size * sizeof(unsigned char));
    
    fread(input_buffer, sizeof(unsigned char), movie_size, fp);
    
    for (unsigned int frame = 0; frame < m->frames; frame++)
    {
        for (unsigned int p = 0; p < 2; p++)
        {
            struct port port = m->port[p];
            
            if (port.type == NONE)
                goto end;
            
            for (unsigned int c = 0; c < port.num_controllers; c++)
            {
                uint16_t input = *(input_buffer + 0) | (*(input_buffer + 1) << 8);
                input_buffer += 2;
                
                char ch = 0;
                
                /*
                if (l == 0x00 & h == 0x10) ch = 'S';
                */
                
                // BYsSudlrAXLR0123
                
                if (p == 0 && frame > 5349)
                {
                   printf("%04x", input);
                }
                
                if (input & R) set_port_index(port, frame, c, 11, 1); /* R */
                if (input & L) set_port_index(port, frame, c, 10, 1); /* L */
                if (input & X) set_port_index(port, frame, c, 9,  1); /* X */
                if (input & A) set_port_index(port, frame, c, 8,  1); /* A */
                if (input & RIGHT) set_port_index(port, frame, c, 7,  1); /* r */
                if (input & LEFT) set_port_index(port, frame, c, 6,  1); /* l */
                if (input & DOWN) set_port_index(port, frame, c, 5,  1); /* d */
                if (input & UP) set_port_index(port, frame, c, 4,  1); /* u */
                if (input & START) set_port_index(port, frame, c, 3,  1); /* S */
                if (input & SELECT) set_port_index(port, frame, c, 2,  1); /* s */
                if (input & Y) set_port_index(port, frame, c, 1,  1); /* Y */
                if (input & B) set_port_index(port, frame, c, 0,  1); /* B */
                
                /*
                for (unsigned int i = 0; i < port.controller_num_indices; i++)
                {
                    char ch = '.';
                    
                    if (l == 0x00 & h == 0x10) ch = 'S';
                    
                    printf("%c", ch);
                }*/
            }
            
            
        end: {}
            //printf("|");
        }
        
        printf("%04d: |", frame);
        movie_print_frame(*m, frame);
        if (frame == 5352)
        {
            //movie_print_frame(*m, frame);
            //exit(0);
        }
        
        //printf("\n");
    }
    
    fclose(fp);
    
    return 0;
}

static int smv_write(movie m, const char* filename)
{
    return 0;
}

const movie_file smv = { smv_read, smv_write };
