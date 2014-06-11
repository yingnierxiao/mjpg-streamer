/*******************************************************************************
#                                                                              #
#      MJPG-streamer allows to stream JPG frames from an input-plugin          #
#      to several output plugins                                               #
#      This is output plugin to send data to web server directly,              #
#      useful if device has no white ip for example	                       #
#                                                                              #
#      Copyright (C) 2014 Ivan Rogozhev                                        #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <dirent.h>
#include <limits.h>

#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include "serverUtils.h"
#include "jpegUtils.h"

#include "../../utils.h"
#include "../../mjpg_streamer.h"

#define OUTPUT_PLUGIN_NAME "http push plugin"

static pthread_t worker;
static globals *pglobal;

// unconverted jpeg
static unsigned long  max_frame_size=0;
static unsigned char *frame = NULL;

// converted jpeg
static unsigned long progressiveBufferSize = 0;	
static unsigned char *progressiveBuffer = NULL;
// write socket
static int wSocket = 0;
struct sockaddr writeAddress={0};


static char* server = NULL;
static const char* DEF_SERVER="localhost";

static char* port = NULL;
static const char* DEF_PORT = "8080";
static int iPort = 8080;

static char* arguments = NULL;
static const char* DEF_ARG="";

static int frames = 0;
static int input_number = 0;
static int reconnectAttempts = -1;



const char POST_REQUEST[]="POST /%s HTTP/1.1\r\n"\
        "Server: %s:%d\r\n"\
        "User-Agent: mjpgStreamer/0.1\r\n"\
        "Content-Type: multipart/x-mixed-replace; boundary=boundarydonotcross\r\n\r\n";

const char FRAME_STRING[]="--boundarydonotcross\r\n"\
                       "Content-Type: image/jpeg\r\n"\
                        "Content-Length: %d\r\n\r\n";



/******************************************************************************
Description.: print a help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " ---------------------------------------------------------------\n" \
            " Help for output plugin..: "OUTPUT_PLUGIN_NAME"\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this plugin:\n\n" \
            " [-s | --server ]........: server to stream to, default localhost\n" \
            " [-p | --port ]..........: port to stream to, default 8080 \n" \
	    " [-a | --arguments ].....: arguments for server, default \"\" \n"\
            " [-f | --frames ]........: maximum fps, 0 as in input plugin(default)\n" \
            " [-i | --input ].........: read frames from the specified input plugin\n"\
	    " [-r | --reconnect ].....: reconnect attmpts. 0=stop on network fail, -1=infinite(default)\n"\
            " ---------------------------------------------------------------\n");
}

/******************************************************************************
Description.: clean up allocated resources
Input Value.: unused argument
Return Value: -
******************************************************************************/
void worker_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    //TODO fill

    if(!first_run) {
        DBG("already cleaned up resources\n");
        return;
    }

    first_run = 0;
    OPRINT("cleaning up resources allocated by worker thread\n");

    if(frame != NULL) {
        free(frame);
    }

   // if(progressiveBuffer) free(progressiveBuffer);
    
    if(wSocket>0)
    {
	close(wSocket);
    }


   // close(fd);
}

/******************************************************************************
Description.: this is the main worker thread
              it loops forever, grabs a frame and sends it to server
Input Value.:
Return Value:
******************************************************************************/
void *worker_thread(void *arg)
{

    bool connected = false;
    int connectAttempts = reconnectAttempts>=0?reconnectAttempts+1:reconnectAttempts;
    
	
    unsigned char *tmp_framebuffer = NULL;
    long frame_size = 0;
    /* set cleanup handler to cleanup allocated ressources */
    pthread_cleanup_push(worker_cleanup, NULL);

    static struct timespec sleepTime1={0,1000000};
    static struct timespec remain={0,0};

    unsigned long cropSize = ULONG_MAX;
    //long progressiveSize=0;
    my_timestamp frameTime = frames>0?1000000/frames:0;


    unsigned long actualSize;

	pthread_mutex_lock(&pglobal->in[input_number].db);
        
	pthread_cond_wait(&pglobal->in[input_number].db_update, &pglobal->in[input_number].db);
	pthread_mutex_unlock(&pglobal->in[input_number].db);

    while(reconnectAttempts != 0 && !pglobal->stop) {
        

	
	
	if(!connected)
	{
		connected = su_connectOutput(&wSocket,writeAddress);
		if(connected)
		{
			connectAttempts=reconnectAttempts;
			connected=su_startPost(wSocket,POST_REQUEST,arguments,server,iPort); 
		}else
		{
			connectAttempts--;
			nanosleep(&sleepTime1,&remain);
		}
	}		
	
		
	if(!connected){
		DBG("Connection failed! connected =%d\n",connected);
		continue;	


	}
	DBG("waiting for fresh frame\n");
	
	
	my_timestamp startTime = su_getCurUsec();

        pthread_mutex_lock(&pglobal->in[input_number].db);
        
	// just sending all
	if(frameTime==0){
		pthread_cond_wait(&pglobal->in[input_number].db_update, &pglobal->in[input_number].db);
	}
        /* read buffer */
        frame_size = pglobal->in[input_number].size;

        /* check if buffer for frame is large enough, increase it if necessary */
        if(frame_size > max_frame_size) {
            DBG("increasing buffer size to %d\n", (int)frame_size);

            max_frame_size = frame_size + (1 << 16);
            if((tmp_framebuffer = realloc(frame, max_frame_size)) == NULL) {
                pthread_mutex_unlock(&pglobal->in[input_number].db);
                LOG("not enough memory\n");
                return NULL;
            }

            frame = tmp_framebuffer;
        }

        /* copy frame to our local buffer now */
        memcpy(frame, pglobal->in[input_number].buf, frame_size);

        /* allow others to access the global buffer again */
        pthread_mutex_unlock(&pglobal->in[input_number].db);

	

	/*converting to progressive */


	ju_processFrame(frame, frame_size, &progressiveBuffer, &progressiveBufferSize);
	
	actualSize = ju_cropJpeg(progressiveBuffer,progressiveBufferSize,cropSize);
	
	my_timestamp codingTime=su_getCurUsec()-startTime;

	

	DBG("sending frame\n");
	connected =su_sendFrame(wSocket, FRAME_STRING, progressiveBuffer, actualSize );         
	


	OPRINT("ct = %d\n",(int) codingTime);
	if(connected)
	{

		if(frameTime>0){

			my_timestamp endtime = su_getCurUsec();
			my_timestamp realFrameTime = endtime-startTime;
			my_timestamp sendTime = endtime-codingTime;

			my_timestamp usableFrameTime =frameTime;
			if((codingTime>(3*frameTime/4)))
			{
				OPRINT("ERROR: Sending images too fast, unable to convert in time\n");
				usableFrameTime = codingTime*4/3;
			OPRINT("fps downed to %d, base ft = %d, ft = %d \n",(int)(1000000/usableFrameTime), (int)frameTime,(int)usableFrameTime);

			}
		
		

			//    bytes/microsec
			double bpu =((double)actualSize)/(double)sendTime;
			//   time we want to send data
			double desiredSendTime = (double)(usableFrameTime-codingTime);
			// cropping size for jpeg calculated			
			cropSize =(long)(desiredSendTime*bpu);
		
			my_timestamp timeToSleep = usableFrameTime-realFrameTime;
		
			if(timeToSleep>0)
			{
				struct timespec sleepTime={0,timeToSleep*1000};
				nanosleep(&sleepTime,&remain);
			}	
		}

	}	
        
    }


    if(progressiveBuffer) free(progressiveBuffer);

    /* cleanup now */
    pthread_cleanup_pop(1);

    return NULL;
}

/*** plugin interface functions ***/
/******************************************************************************
Description.: this function is called first, in order to initialize
              this plugin and pass a parameter string
Input Value.: parameters
Return Value: 0 if everything is OK, non-zero otherwise
******************************************************************************/
/*

            " [-s | --server ]........: server to stream to, default localhost\n" \
            " [-p | --port ]......... : port to stream to, default 8080 \n" \
            " [-f | --frames ]........: maximum fps, 0 = as in input plugin(default)\n" \
            " [-i | --input ].........: read frames from the specified input plugin\n"\
	    " [-r | --reconnect ].....: reconnect attmpts. 0=stop on network fail, -1=infinite(default)\n"\
	    " [-a | --arguments ].....: arguments for server, default \"\" \n"\

*/
int output_init(output_parameter *param, int id)
{
	int i;
    pglobal = param->global;
    pglobal->out[id].name = malloc((1+strlen(OUTPUT_PLUGIN_NAME))*sizeof(char));
    sprintf(pglobal->out[id].name, "%s", OUTPUT_PLUGIN_NAME);
    DBG("OUT plugin %d name: %s\n", id, pglobal->out[id].name);

    param->argv[0] = OUTPUT_PLUGIN_NAME;

    /* show all parameters for DBG purposes */
    for(i = 0; i < param->argc; i++) {
        DBG("argv[%d]=%s\n", i, param->argv[i]);
    }

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0
            },
            {"help", no_argument, 0, 0},
            {"s", required_argument, 0, 0},
            {"server", required_argument, 0, 0},
            {"p", required_argument, 0, 0},
            {"port", required_argument, 0, 0},
            {"i", required_argument, 0, 0},
            {"input", required_argument, 0, 0},
            {"r", required_argument, 0, 0},
            {"reconnect", required_argument, 0, 0},
            {"a", required_argument, 0, 0},
            {"arguments", required_argument, 0, 0},
            {"f", required_argument, 0, 0},
            {"frames", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(param->argc, param->argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return 1;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            DBG("case 0,1\n");
            help();
            return 1;
            break;

            /* s, server */
        case 2:
        case 3:
	DBG("case 2,3\n");
            server = su_cloneString(optarg);
	break;

            /* p, port */
        case 4:
        case 5:
	    DBG("case 4,5\n");
	    port =  su_cloneString(optarg);
	    iPort = atoi(port); 
            break;

            /* i, input*/
        case 6:
        case 7:
            DBG("case 6,7\n");
	    input_number = atoi(optarg);
	    break;

            /* r, reconnect */
        case 8:
        case 9:
            DBG("case 8,9\n");
            reconnectAttempts = atoi(optarg);
	    break;
	   /* a, arguments*/
        case 10:
        case 11:
            DBG("case 10,11\n");
            arguments =  su_cloneString(optarg);
	    break;
          /* f, frames*/
        case 12:
        case 13:
            DBG("case 12,13\n");
	    frames = atoi(optarg);
	    break;

        }
    }


    if(!server)
    {
	server = su_cloneString(DEF_SERVER);
    }
    if(!port)
    {
	port = su_cloneString(DEF_PORT);
    }
    if(!arguments)
    {
	arguments =  su_cloneString(DEF_ARG);
    }

    OPRINT("input plugin......: %d: %s\n", input_number, pglobal->in[input_number].plugin);		
    OPRINT("output address....: %s:%s/%s\n", server,port,arguments);
    OPRINT("frame rate........: ");
    if(frames)
    {
	OPRINT("%d\n", frames);
    
    }else
    {
	OPRINT("same as input\n");
    }

    OPRINT("reconnect attempts: ");
    switch (reconnectAttempts){
    	case 0:
	   OPRINT("none\n");
	   break;
	case -1:
	   OPRINT("infinite\n");
	   break;
	default:
	   OPRINT("%d\n", reconnectAttempts);
    }

    param->global->out[id].parametercount = 0;

    param->global->out[id].out_parameters =  NULL;

    OPRINT("\nValidating:\n");

    if(!(input_number < pglobal->incnt)) {
        OPRINT("ERROR: the %d input_plugin number is too much only %d plugins loaded\n", input_number, param->global->incnt);
        return 1;
    }


    OPRINT("Resolving adress...\n");
    
    if((wSocket = su_getProperSocketAddress(server,port,&writeAddress))>0)
    {
	OPRINT("Success!\n");
    }else
    {
	OPRINT("ERROR: Could not resolve host, exit now\n");
	return 2;
    }
    



//(control*) calloc(2, sizeof(control));
/*
    control take_ctrl;
	take_ctrl.group = IN_CMD_GENERIC;
	take_ctrl.menuitems = NULL;
	take_ctrl.value = 1;
	take_ctrl.class_id = 0;

	take_ctrl.ctrl.id = OUT_FILE_CMD_TAKE;
	take_ctrl.ctrl.type = V4L2_CTRL_TYPE_BUTTON;
	strcpy((char*) take_ctrl.ctrl.name, "Take snapshot");
	take_ctrl.ctrl.minimum = 0;
	take_ctrl.ctrl.maximum = 1;
	take_ctrl.ctrl.step = 1;
	take_ctrl.ctrl.default_value = 0;

	param->global->out[id].out_parameters[0] = take_ctrl;

    control filename_ctrl;
	filename_ctrl.group = IN_CMD_GENERIC;
	filename_ctrl.menuitems = NULL;
	filename_ctrl.value = 1;
	filename_ctrl.class_id = 0;

	filename_ctrl.ctrl.id = OUT_FILE_CMD_FILENAME;
	filename_ctrl.ctrl.type = V4L2_CTRL_TYPE_STRING;
	strcpy((char*) filename_ctrl.ctrl.name, "Filename");
	filename_ctrl.ctrl.minimum = 0;
	filename_ctrl.ctrl.maximum = 32;
	filename_ctrl.ctrl.step = 1;
	filename_ctrl.ctrl.default_value = 0;

	param->global->out[id].out_parameters[1] = filename_ctrl;
*/

    return 0;
}

/******************************************************************************
Description.: calling this function stops the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_stop(int id)
{
    DBG("will cancel worker thread\n");
    pthread_cancel(worker);

    // TODO free data from init?
    return 0;
}

/******************************************************************************
Description.: calling this function creates and starts the worker thread
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_run(int id)
{
    DBG("launching worker thread\n");
    pthread_create(&worker, 0, worker_thread, NULL);
    pthread_detach(worker);
    return 0;
}

int output_cmd(int plugin_id, unsigned int control_id, unsigned int group, int value, char *valueStr)
{
   // int i = 0;
    DBG("command (%d, value: %d) for group %d triggered for plugin instance #%02d\n", control_id, value, group, plugin_id);
    (void) valueStr;
  //  switch(group) {
//		case IN_CMD_GENERIC:
//			for(i = 0; i < pglobal->out[plugin_id].parametercount; i++) {
//				if((pglobal->out[plugin_id].out_parameters[i].ctrl.id == control_id) && (pglobal->out[plugin_id].out_parameters[i].group == IN_CMD_GENERIC)) {
//					DBG("Generic control found (id: %d): %s\n", control_id, pglobal->out[plugin_id].out_parameters[i].ctrl.name);
//					switch(control_id) {
//                            case OUT_FILE_CMD_TAKE: {
//                                if (valueStr != NULL) {
//                                    int frame_size = 0;
//                                    unsigned char *tmp_framebuffer = NULL;
//
//                                    if(pthread_mutex_lock(&pglobal->in[input_number].db)) {
//                                        DBG("Unable to lock mutex\n");
//                                        return -1;
//                                    }
                                    /* read buffer */
//                                    frame_size = pglobal->in[input_number].size;

                                    /* check if buffer for frame is large enough, increase it if necessary */
//                                    if(frame_size > max_frame_size) {
//                                        DBG("increasing buffer size to %d\n", frame_size);

//                                        max_frame_size = frame_size + (1 << 16);
//                                        if((tmp_framebuffer = realloc(frame, max_frame_size)) == NULL) {
//                                            pthread_mutex_unlock(&pglobal->in[input_number].db);
//                                            LOG("not enough memory\n");
//                                            return -1;
//                                        }

//                                        frame = tmp_framebuffer;
//                                    }

//                                    /* copy frame to our local buffer now */
//                                    memcpy(frame, pglobal->in[input_number].buf, frame_size);

                                    /* allow others to access the global buffer again */
//                                    pthread_mutex_unlock(&pglobal->in[input_number].db);

//                                    DBG("writing file: %s\n", valueStr);

//                                    int fd;
                                    /* open file for write */
//                                    if((fd = open(valueStr, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
//                                        OPRINT("could not open the file %s\n", valueStr);
//                                        return -1;
//                                    }

                                    /* save picture to file */
//                                    if(write(fd, frame, frame_size) < 0) {
//                                        OPRINT("could not write to file %s\n", valueStr);
//                                        perror("write()");
//                                        close(fd);
//                                        return -1;
//                                    }

//                                    close(fd);
//                                } else {
//                                    DBG("No filename specified\n");
//                                    return -1;
//                                }
//                            } break;
//                            case OUT_FILE_CMD_FILENAME: {
//                                DBG("Not yet implemented\n");
//                                return -1;
//                            } break;
//                            default: {
//                                DBG("Unknown command\n");
//                                return -1;
//                            } break;
//					}
//					DBG("Ctrl %s new value: %d\n", pglobal->out[plugin_id].out_parameters[i].ctrl.name, value);
//					return 0;
//				}
//			}
//			DBG("Requested generic control (%d) did not found\n", control_id);
//			return -1;
//			break;
//	}
    return 0;
}
