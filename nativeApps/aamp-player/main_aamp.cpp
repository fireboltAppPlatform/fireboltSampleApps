/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include "main_aamp.h"

GMainLoop *gMainLoop = 0;

static void showUsage()
{
    printf( "Usage:\n" );
    printf( " rne_player [options] <uri>\n" );
    printf( "  uri - URI of video asset to play\n" );
    printf( "where [options] are:\n" );
    printf( "  -? : show usage\n" );
    printf( "\n" );
}

int main(int argc, char *argv[])
{
    int argidx= 1;
    bool retValue  = true;
    int volume = 10;
    int width = 1280, height = 720;
    int rate = 1;
    const char *uri= "http://nh.lab.xcal.tv/integration/files/neil/master.m3u8";

    while ( argidx < argc )
    {
        if ( argv[argidx][0] == '-' )
        {
            switch( argv[argidx][1] )
            {
                case '?':
                    showUsage();
                    retValue = false;
                    break;
                default:
                    printf( "unknown option %s\n\n", argv[argidx] );
                    retValue = false;
                    break;
            }
        }
        else
        {
            uri = argv[argidx];
        }

        ++argidx;
    }

    if ( NULL == uri )
    {
        printf( "missing uri argument\n" );
        retValue = false;
    }

    if( false != retValue )
    {
        gst_init(0, 0);
        gMainLoop = g_main_loop_new(0, FALSE);

        printf( "Playing asset: %s\n", uri );

        PlayerInstanceAAMP *playerInstance = new PlayerInstanceAAMP();

        /* Set URL */
        playerInstance->Tune(uri);
        /* Set video rectangle */
        playerInstance->SetVideoRectangle(0, 0, width, height);
        /* Set audio volume */
        playerInstance->SetAudioVolume(volume);
        /* Set rate */
        /* sf              : 0.5
           fastforward     : 16, 32
           rewind          : 4, 8, 16, 32 (set as negative )
           pause            : 0
           play            : 1
           stop            : playerInstance->Stop();
        */
        playerInstance->SetRate(rate);

        printf("Start main loop");
        g_main_loop_run(gMainLoop);

        g_main_loop_unref(gMainLoop);
        gMainLoop = 0;

        gst_deinit();
    }

    return 0;
}






