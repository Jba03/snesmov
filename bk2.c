//
//  bk2.c
//  snesmov
//
//  Created by Jba03 on 2021-01-04.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include "movie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zip.h>

#define log(...) \
    printf("[bk2] " __VA_ARGS__);

static int bk2_read(movie* m, const char* filename)
{
    int err;
    char error_buffer[256];
    
    struct zip* archive;
    struct zip_file* file;
    struct zip_stat st;
    
    /* Open the .lsmv zip archive */
    archive = zip_open(filename, ZIP_RDONLY, &err);
    if (!archive)
    {
        zip_error_to_str(error_buffer, sizeof(error_buffer), err, errno);
        log("failed to open zip archive: %s\n", error_buffer);
        return -1;
    }
    
    int got_header = 0;
    int got_input = 0;
    
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
            
            /* Read file data */
            char* data = malloc(st.size);
            memset(data, 0, st.size);
            if (zip_fread(file, data, st.size) < 0)
            {
                log("failed to read data from entry '%s'\n", st.name);
                return -1;
            }
            
            if (!strcmp(st.name, "Header.txt"))
            {
                while (*(data++) != '\0')
                {
                    if (!strncmp(data, "GameName", 8))
                    {
                        m->gamename = malloc(256);
                        memset(m->gamename, 0, 256);
                        sscanf(data, "GameName %[^\r]", m->gamename);
                    }
                    
                    if (!strncmp(data, "Author", 6))
                    {
                        m->authors = malloc(512);
                        memset(m->authors, 0, 512);
                        sscanf(data, "Author %[^\r]", m->authors);
                    }
                    
                    if (!strncmp(data, "rerecordCount", 13))
                    {
                        sscanf(data, "rerecordCount %d", &m->rerecords);
                    }
                    
                    if (!strncmp(data, "Platform", 8))
                    {
                        char platform[16];
                        sscanf(data, "Platform %[^\r]", platform);
                        
                        /* TODO: Verify region names */
                        if (!strcmp(platform, "SNES"))      m->gametype = SNES_NTSC; else
                        if (!strcmp(platform, "SNES_PAL"))  m->gametype = SNES_PAL; else
                        if (!strcmp(platform, "SGB"))       m->gametype = SGB_NTSC; else
                        if (!strcmp(platform, "SGB_PAL"))   m->gametype = SGB_PAL; 
                        else
                        {
                            log("unsupported system '%s'\n", platform);
                            return -1;
                        }
                    }
                    
                    if (!strncmp(data, "StartsFromSavestate", 19))
                    {
                        log("movies that start from savestates are unsupported\n");
                        return -1;
                    }
                }
                
                got_header = 1;
            }
            
            if (!strcmp(st.name, "Input Log.txt"))
            {
                int i = 0, p = 0;
                int port_num_controllers[2];
                int port_controller_num_indices[2];
                enum port_type port_type[2];
                
                char* logkey = NULL;
                char* beginning = NULL;
                
                while (i != 2) 
                {
                    if (*(data++) == '\n') i++;
                    
                    /* 
                    * Assume first four characters of frame
                    * always contain reset and power lines
                    */
                    if (i == 0) logkey = data + 21;
                }
                
                beginning = data;
                
                while (*(data++) != '\0') 
                    if (*(data) == '\n') m->frames++;
                
                m->frames -= 1;
                
                /* Reads log key and determines number of controllers */
                while (*logkey != '\r')
                {
                    /* #P2 indicates second port? */
                    if (!strncmp(logkey, "#P2", 3))
                        p++;
                    
                    if (*logkey == '#') /* New controller */
                    {
                        port_num_controllers[p]++;
                        port_controller_num_indices[p] = 0;
                    }
                    
                    if (*logkey == '|') /* New button/index */
                        port_controller_num_indices[p]++;
                    
                    logkey++;
                }
                
                /* Allocate ports */
                for (p = 0; p < 2; p++)
                {
                    port_type[p] = find_port_type(port_num_controllers[p], port_controller_num_indices[p]);
                    m->port[p] = port_alloc(port_type[p], m->frames);
                }
                
                m->reset = malloc(m->frames * sizeof(unsigned char));
                m->power = malloc(m->frames * sizeof(unsigned char)); /* What is the power line anyways? */
                memset(m->reset, 0, m->frames * sizeof(unsigned char));
                memset(m->power, 0, m->frames * sizeof(unsigned char));
                
                data = beginning;
                
                for (unsigned int frame = 0; frame < m->frames; frame++)
                {
                    char* line = strtok(data, "\r");
                    
                    char reset;
                    char power;
                    if (sscanf(line, "|%c%c|", &reset, &power) != 2)
                    {
                        log("invalid frame input format detected\n");
                        return -1;
                    }
                    
                    if (reset != '.') m->reset[frame] = 1;
                    if (power != '.') m->power[frame] = 1;
                    
                    data = line += 3; /* Skip reset & power */
                    
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
                                
                                /* BK2 gamepad format to internal (lsnes) gamepad format */
                                const char* fmt = "BYsSUDLRAXlr0123";
                                
                                for (int n = 0; n < 16; n++)
                                {
                                    if (fmt[n] == b)
                                    {
                                        set_port_index(port, frame, c, n, 1); 
                                    }
                                }
                                
                                port_ptr++;
                            }
                            
                            port_ptr++;
                        }
                    }
                end:
                    
                    data += strlen(line) + 2; /* Go to next line */
                }
                
                got_input = 1;
            }
            
            log("found entry: %s\n", st.name);
        }
    }
    
    if (!got_header)
    {
        log("movie file header not found\n");
        return -1;
    }
    
    if (!got_input)
    {
        log("movie frame input file not found\n");
        return -1;
    }
    
    return 0;
}

static int bk2_write(movie m, const char* filename)
{
    return 0;
}

const movie_file bk2 = { bk2_read, bk2_write };
