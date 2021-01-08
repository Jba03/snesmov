//
//  main.c
//  snesmov
//
//  Created by Jba03 on 2021-01-03.
//  Copyright © 2021 Jba03. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

#include "movie.h"

/*
 * Offset to add blank frames
 * or to remove inital frames
 *
 * If positive: add amount of initial blank frames
 * If negative: remove amount of initial blank frames
 */
int offset_initial_frame;

static struct option long_options[] =
{
    {"add", required_argument, 0, 'a'},
    {"remove", required_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};

static void usage()
{
    printf("usage: snesmov input output\n");
    //printf("    -a: add initial blank frames     \n");
    //printf("    -r: remove initial frames        \n");
}

int main(int argc, char** argv)
{
    int c;
    char *filename[2], *extension[2];
    movie_file movie_file[2];
    
    if (argc < 3)
    {
        usage();
        return -1;
    }
    
    while (1)
    {
        int option_index;
        if ((c = getopt_long(argc, argv, "a:r:h", long_options, &option_index)) == -1)
            break;
            
        switch (c)
        {
            case 'a':
                offset_initial_frame = abs(atoi(optarg));
                break;
                
            case 'r':
                offset_initial_frame = -abs(atoi(optarg));
                break;
                
            case 'h':
                usage();
                return 0;
                
            default: break;
        }
    }
    
    for (int i = 0; i < 2; i++)
    {
        if (filename[i] == NULL)
            filename[i] = argv[argc - 2 + i];
        
        extension[i] = strchr(filename[i], '.');
        
        if (!strcmp(extension[i], ".lsmv")) movie_file[i] = lsmv; else
        if (!strcmp(extension[i], ".bk2")) movie_file[i] = bk2; else
        if (!strcmp(extension[i], ".smv"))
        {
            movie_file[i] = smv;
            printf("warning: a snes9x movie will most likely desync on accurate emulators\n");
        }
        else
        {
            fprintf(stderr, "unknown extension '%s'\n", extension[i]);
            return -1;
        }
    }
    
    movie m;
    {
        movie_init(&m);
        if (movie_file[0].read(&m, filename[0]) < 0) return -1;
        if (movie_file[1].write(m, filename[1]) < 0) return -1;
    }
    
    return 0;
}
