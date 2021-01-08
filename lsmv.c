//
//  lsmv.c
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include "movie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zip.h>

#define log(...) \
    printf("[lsmv] " __VA_ARGS__);

/* 
 * Files necessary for a lsmv file to load:
 *      controlsversion
 *      coreversion
 *      gametype
 *      input
 *      port1 ?
 *      port2 ? 
 *      projectid
 *      rerecords
 *      rrdata
 *      systemid
 */

static const char* core_version = "bsnes v085 (Compatibility core)";

static int lsmv_read(movie* m, const char* filename)
{
    int err;
    char error_buffer[256];
    
    struct zip* archive;
    struct zip_file* file;
    struct zip_stat st;
    
    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        log("failed to open file '%s'\n", filename);
        return -1;
    }
    
    /* Binary file check */
    if (fgetc(fp) == 'l' && 
        fgetc(fp) == 's' && 
        fgetc(fp) == 'm' &&
        fgetc(fp) == 'v' &&
        fgetc(fp) == '\x1A')
    {
        log("lsmv binary files not yet supported\n");
        return -1;
    }
    
    /* Open the .lsmv zip archive */
    archive = zip_open(filename, ZIP_RDONLY, &err);
    if (!archive)
    {
        zip_error_to_str(error_buffer, sizeof(error_buffer), err, errno);
        log("failed to open zip archive: %s\n", error_buffer);
        return -1;
    }
    
    int got_gametype = 0;
    int got_port1 = 0;
    int got_port2 = 0;
    int got_input = 0;
    int got_projectid = 0;
    
    char* input_buffer = NULL;
    zip_int64_t input_buffer_size;
    
    /* Default port configuration */
    enum port_type port_types[2] = {GAMEPAD, NONE};
    
    /* For each archive entry */
    for (zip_int64_t n = 0; n < zip_get_num_entries(archive, 0); n++)
    {
        if (zip_stat_index(archive, n, 0, &st) == 0)
        {
            file = zip_fopen_index(archive, n, ZIP_RDONLY);
            if (!file)
            {
                log("failed to open archived file entry '%s'\n", st.name);
                return -1;
            }
            
            log("found entry: %s\n", st.name);
            
            /* Read file data */
            char* data = malloc(st.size);
            memset(data, 0, st.size);
            if (zip_fread(file, data, st.size) < 0)
            {
                log("failed to read data from entry '%s'\n", st.name);
                return -1;
            }
            
            if (!strcmp(st.name, "savestate"))
            {
                log("this file is not a movie\n");
                return -1;
            }
            
            if (!strcmp(st.name, "rerecords"))
            {
                m->rerecords = atoi(data);
            }
            
            if (!strcmp(st.name, "port1") ||
                !strcmp(st.name, "port2"))
            {
                int p = atoi(st.name + 4) - 1;
                p == 0 ? (got_port1 = 1) : (got_port2 = 1);
                
                data = strtok(data, "\n");
                
                if (!strcmp(data, "none"))       port_types[p] = NONE;
                if (!strcmp(data, "gamepad"))    port_types[p] = GAMEPAD;
                if (!strcmp(data, "gamepad16"))  port_types[p] = GAMEPAD16;
                if (!strcmp(data, "ygamepad16")) port_types[p] = YGAMEPAD16;
                if (!strcmp(data, "multitap"))   port_types[p] = MULTITAP;
                if (!strcmp(data, "multitap16")) port_types[p] = MULTITAP16;
                if (!strcmp(data, "mouse"))      port_types[p] = MOUSE;
                if (!strcmp(data, "superscope")) port_types[p] = SUPERSCOPE;
                if (!strcmp(data, "justifier"))  port_types[p] = JUSTIFIER;
                if (!strcmp(data, "justifiers")) port_types[p] = JUSTIFIERS;
            }
            
            if (!strcmp(st.name, "input"))
            {
                input_buffer = malloc(st.size);
                memset(input_buffer, 0, st.size);
                memcpy(input_buffer, data, st.size);
                input_buffer_size = st.size;
                
                got_input = 1;
            }
            
            if (!strcmp(st.name, "gametype"))
            {
                if (!strcmp(data, "snes_ntsc"))   m->gametype = SNES_NTSC;
                if (!strcmp(data, "snes_pal"))    m->gametype = SNES_PAL;
                if (!strcmp(data, "bsx"))         m->gametype = BSX;
                if (!strcmp(data, "bsxslotted"))  m->gametype = BSX_SLOTTED;
                if (!strcmp(data, "sufamiturbo")) m->gametype = SUFAMI_TURBO;
                if (!strcmp(data, "sgb_ntsc"))    m->gametype = SGB_NTSC;
                if (!strcmp(data, "sgb_pal"))     m->gametype = SGB_PAL;
                
                got_gametype = 1;
            }
            
            if (!strcmp(st.name, "projectid"))
            {
                /* 
                 * Assume the lsmv file is valid
                 * if we got only the projectid and gametype files
                 */
                got_projectid = 1;
            }
        }
    }
    
    if (!got_projectid)
    {
        log("projectid not found\n");
        return -1;
    }
    
    if (!got_gametype)
    {
        log("gametype not found\n");
        return -1;
    }
    
    if (!got_input)
    {
        log("movie input file not found\n");
        return -1;
    }
    
    char* p = input_buffer;
    while (*(p++) != '\0')
        if (*p == '\n') m->frames++;
    
    /* Allocate input frames */
    m->port[0] = port_alloc(port_types[0], m->frames);
    m->port[1] = port_alloc(port_types[1], m->frames);
    
    m->reset = malloc(m->frames);
    m->power = malloc(m->frames);
    memset(m->reset, 0, m->frames);
    memset(m->power, 0, m->frames);
    
    
    /* Begin reading input */
    for (unsigned int frame = 0; frame < m->frames; frame++)
    {
        char* line = strtok(input_buffer, "\n");
        
        /* TODO: Check if all movies start with the format F. X Y |, or if some movies start only with F.| */
        
        //printf("%s\n", line);
        
        /* Read frame 'header' */
        int x, y;
        char is_frame, reset;
        if (sscanf(line, "%c%c %d %d|", &is_frame, &reset, &x, &y) != 4)
        {
            log("invalid frame input format detected: stop.\n");
            return -1;
        }
        
        if (is_frame != 'F') /* TODO: Add support for subframes? */
        {
            log("encountered subframe: skipping...\n");
            goto end;
        }
        
        if (x != 0 || y != 0) /* TODO: Add support for delayed resets? */
        {
            log("encountered delayed reset: reading as regular reset instead\n");
            goto end;
        }
        
        if (reset != '.')
            m->reset[frame] = 1;
        
        for (unsigned int p = 0; p < 2; p++)
        {
            struct port port = m->port[p];
            
            if (port.type == NONE)
                goto end;
            
            int offset = 0;
            if (p == 1) /* Create offset for second port */
                offset = m->port[0].num_controllers * m->port[0].controller_num_indices;
            
            char* port_ptr = strchr(line + offset, '|') + 1;
            
            for (unsigned int c = 0; c < port.num_controllers; c++)
            {
                for (unsigned int i = 0; i < port.controller_num_indices; i++)
                {
                    char b = *port_ptr;
                    
                    if (b != '.')
                        set_port_index(m->port[p], frame, c, i, 1);
                    
                    port_ptr++;
                }
                
                port_ptr++;
            }
        }
    end:
        
        input_buffer += strlen(line) + 1;
    }
    
    return 0;
}

static int lsmv_write(movie m, const char* filename)
{
    int err;
    char error_buffer[256];
    struct zip* archive;
    
    /* Binary file writing not supported */
    
    archive = zip_open(filename, ZIP_CREATE, &err);
    if (!archive)
    {
        zip_error_to_str(error_buffer, sizeof(error_buffer), err, errno);
        log("failed to create zip archive: %s", error_buffer);
        return -1;
    }
    
    const char* gametype;
    const char* portn[2];
    char rerecords[16];
    
    switch (m.gametype)
    {
        case SNES_NTSC:     gametype = "snes_ntsc";     break;
        case SNES_PAL:      gametype = "snes_pal";      break;
        case BSX:           gametype = "bsx";           break;
        case BSX_SLOTTED:   gametype = "bsxslotted";    break;
        case SUFAMI_TURBO:  gametype = "sufamiturbo";   break;
        case SGB_NTSC:      gametype = "sgb_ntsc";      break;
        case SGB_PAL:       gametype = "sgb_pal";       break;
    }
    
    for (int p = 0; p < 2; p++)
    {
        switch(m.port[p].type)
        {
            case NONE:          portn[p] = "none";          break;
            case GAMEPAD:       portn[p] = "gamepad";       break;
            case GAMEPAD16:     portn[p] = "gamepad16";     break;
            case YGAMEPAD16:    portn[p] = "ygamepad16";    break;
            case MULTITAP:      portn[p] = "multitap";      break;
            case MULTITAP16:    portn[p] = "multitap16";    break;
            case MOUSE:         portn[p] = "mouse";         break;
            case SUPERSCOPE:    portn[p] = "superscope";    break;
            case JUSTIFIER:     portn[p] = "justifier";     break;
            case JUSTIFIERS:    portn[p] = "justifiers";    break;
        }
    }
    
    sprintf(rerecords, "%d", m.rerecords);
    
    if (!m.authors) m.authors = "none";
    
    zip_source_t* gametype_s = zip_source_buffer(archive, gametype, strlen(gametype), 0);
    zip_source_t* authors_s = zip_source_buffer(archive, m.authors, strlen(m.authors), 0);
    zip_source_t* port1_s = zip_source_buffer(archive, portn[0], strlen(portn[0]), 0);
    zip_source_t* port2_s = zip_source_buffer(archive, portn[1], strlen(portn[1]), 0);
    zip_source_t* rerecords_s = zip_source_buffer(archive, rerecords, strlen(rerecords), 0);
    zip_source_t* systemid_s = zip_source_buffer(archive, "lsnes-rr1", 9, 0);
    zip_source_t* controlsversion_s = zip_source_buffer(archive, "0", 1, 0);
    zip_source_t* coreversion_s = zip_source_buffer(archive, core_version, strlen(core_version), 0);
    zip_source_t* projectid_s = zip_source_buffer(archive, "0", 1, 0);
    zip_source_t* rrdata_s = zip_source_buffer(archive, "0", 1, 0);
    
    zip_file_add(archive, "gametype", gametype_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "authors", authors_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "port1", port1_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "port2", port2_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "systemid", systemid_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "controlsversion", controlsversion_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "coreversion", coreversion_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "projectid", projectid_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "rerecords", rerecords_s, ZIP_FL_OVERWRITE);
    zip_file_add(archive, "rrdata", rrdata_s, ZIP_FL_OVERWRITE);
    
    /* 144 bytes per frame should be more than enough */
    char* input_buffer = malloc(m.frames * 144);
    if (!input_buffer) /* Rare occurence */
    {
        log("not enough memory\n");
        return -1;
    }
    
    memset(input_buffer, 0, m.frames * 144);
    
    char* o = input_buffer;
    
    /* Encode input */
    for (unsigned int frame = 0; frame < m.frames; frame++)
    {
        char line[144];
        memset(line, 0, 144);
        
        char* ptr = &line[0];
        
        /* Write frame 'header' */
        char is_frame = 'F'; /* TODO: Add subframe support? */
        char reset = m.reset[frame] > 0 ? 'R' : '.';
        int x = 0, y = 0; /* Delayed resets */
        sprintf(line, "%c%c %d %d|", is_frame, reset, x, y);
        
        ptr += strlen(line);
        
        /* TODO: Check if correct */
        
        for (unsigned int p = 0; p < 2; p++)
        {
            struct port port = m.port[p];
            
            if (port.type == NONE)
                goto end;
            
            for (unsigned int c = 0; c < port.num_controllers; c++)
            {
                for (unsigned int i = 0; i < port.controller_num_indices; i++)
                {
                    if (port_index_value(port, frame, c, i) == 1)
                        *ptr = gamepad_format[i];
                    else
                        *ptr = '.';
                    
                    ptr++;
                }
                
                *(ptr++) = '|';
            }
        }
    end:
        
        //printf("%s\n", line);
        
        memcpy(input_buffer, line, strlen(line) - 1);
        input_buffer += strlen(line) + 0;
        *(input_buffer - 1) = '\n';
    }
    
    //printf("%s\n", o);
    
    zip_source_t* input = zip_source_buffer(archive, o, strlen(o), 0);
    zip_file_add(archive, "input", input, ZIP_FL_OVERWRITE);
    
    zip_close(archive);
    
    return 0;
}

const movie_file lsmv = { lsmv_read, lsmv_write };
