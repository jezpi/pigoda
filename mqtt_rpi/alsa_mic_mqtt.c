/* 
  A Minimal Capture Program
  This program opens an audio interface for capture, configures it for
  stereo, 16 bit, 44.1kHz, interleaved conventional read/write
  access. Then its reads a chunk of random data from it, and exits. It
  isn't meant to be a real program.
  From on Paul David's tutorial : http://equalarea.com/paul/alsa-audio.html
  Fixes rate and buffer problems
  sudo apt-get install libasound2-dev
  gcc -o alsa-record-example -lasound alsa-record-example.c && ./alsa-record-example hw:0
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <mosquitto.h>

const char *audio_device = "hw:0";
static int  MQTT_pub(struct mosquitto *, const char *, bool , const char *, ...);
static char *mqtt_host, *mqtt_user, *mqtt_password, *mqtt_topic, *mqtt_identity;
static int mqtt_port = 1883;
static unsigned short debug_mode;
static unsigned short verbose_flag;
#define dfprintf if (debug_mode) fprintf
int	      
main (int argc, char *argv[])
{
  int i;
  int pcm_err;
  char *buffer;
  int opt;
  struct mosquitto *m;
  
  mqtt_identity = "alsa_capture";
  mqtt_host = "localhost";
  mqtt_topic = "/environment/mic";

	while ((opt = getopt(argc, argv, "a:i:h:u:p:P:vdt:")) != -1) {
		switch(opt) {
			case 'a':
				audio_device = strdup(optarg);
				break;
			case 'h': /* host */
				mqtt_host = strdup(optarg);
				break;
			case 't': /* topic */
				mqtt_topic = strdup(optarg);
				break;
			case 'p': /* port */
				mqtt_port = atoi(optarg);
				break;
			case 'P': /* password */
				mqtt_password = strdup(optarg);
				break;
			case 'u': /* user */
				mqtt_user = strdup(optarg);
				break;
			case 'i':
				mqtt_identity = strdup(optarg);
				break;
			case 'v':
				verbose_flag++;
				break;
			case 'd':
				debug_mode++;
				break;
			default:
				printf("usage: mqtt_mic [-i identity] [-t topic] [-u user] [-h host] [-P passowrd] [-p port]\n");
				exit(64);
		}
	}
   if (mqtt_host == NULL || mqtt_user == NULL || mqtt_password == NULL || mqtt_topic == NULL) {
	   printf("too few arguments\n");
	   exit(64);
   }

   mosquitto_lib_init();
   m= mosquitto_new(mqtt_identity, true, NULL);
   mosquitto_username_pw_set(m, mqtt_user, mqtt_password);

   if (mosquitto_connect(m, mqtt_host, mqtt_port, 300) != MOSQ_ERR_SUCCESS) {
	   dfprintf(stderr, "mosquitto connect failure %s\n",strerror(errno));
	   exit(0);
   }


  signed short sbuf[128];
  snd_pcm_uframes_t frames = 128;
  unsigned int rate = 44100;
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  if ((pcm_err = snd_pcm_open (&capture_handle, audio_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    dfprintf (stderr, "cannot open audio device %s (%s)\n", 
             audio_device,
             snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "audio interface opened\n");
		   
  if ((pcm_err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    dfprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params allocated\n");
				 
  if ((pcm_err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
    dfprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params initialized\n");
	
  if ((pcm_err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    dfprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params access setted\n");
	
  if ((pcm_err = snd_pcm_hw_params_set_format (capture_handle, hw_params, format)) < 0) {
    dfprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params format setted\n");
	
  int dir;
  if ((pcm_err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, &dir)) < 0) {
    dfprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (pcm_err));
    exit (1);
  }
	
  dfprintf(stdout, "hw_params rate %d setted\n", rate);

  frames=32;
  if ((pcm_err = snd_pcm_hw_params_set_period_size_near(capture_handle, hw_params, &frames, &dir)) < 0) {
	  dfprintf(stderr, "cannot set period size %d\n", frames);
	  exit(3);
  }

  if ((pcm_err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
    dfprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params channels setted\n");
  if ((pcm_err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    dfprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "hw_params setted\n");
  snd_pcm_hw_params_free (hw_params);
  dfprintf(stdout, "hw_params freed\n");
  if ((pcm_err = snd_pcm_prepare (capture_handle)) < 0) {
    dfprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (pcm_err));
    exit (1);
  }

  dfprintf(stdout, "audio interface prepared\n");

  buffer = malloc(frames * snd_pcm_format_width(format) * 2);

  dfprintf(stdout, "buffer allocated\n");

  int n, fsum, absval;
  /*for (i = 0; i < 100; ++i) {*/
  for (;;) {
    if ((pcm_err = snd_pcm_readi (capture_handle, sbuf , frames)) != frames) {
      dfprintf (stderr, "read from audio interface failed (%d) %s\n",
               pcm_err, snd_strerror (pcm_err));
      if (errno == EPIPE) {
	      errno = 0;
	      snd_pcm_prepare(capture_handle);
      } else {
      	exit (1);
      }
    }
    fsum = 0;
    
    for (n=0; n < frames;++n) {
	    absval = abs((signed short) sbuf[n]);
	    fsum+=absval;
    }
    printf("%f\n", (float) (fsum/(float)frames));
    if (MQTT_pub(m, mqtt_topic, false, "%f", ((float) (fsum/frames))) < 0) {
	    fprintf(stderr, "publish on %s failure!\n", mqtt_topic);
    }
    sleep(1);
  }

  /*free(buffer);*/

  dfprintf(stdout, "buffer freed\n");
	
  snd_pcm_close (capture_handle);
  dfprintf(stdout, "audio interface closed\n");

  exit (0);
}

static int  
MQTT_pub(struct mosquitto *mosq, const char *topic, bool perm, const char *fmt, ...)
{
	size_t msglen;
	va_list lst;
	char	msgbuf[BUFSIZ];
	int	mid = 0;
	int	ret;

	va_start(lst, fmt);
	vsnprintf(msgbuf, sizeof msgbuf, fmt, lst);
	va_end(lst);
	msglen = strlen(msgbuf);
	/*mosquitto_publish(mosq, NULL, "/guernika/network/broadcast", 3, Mosquitto.mqh_msgbuf, 0, false);*/
	if ((ret = mosquitto_publish(mosq, &mid, topic, msglen, msgbuf, 0, perm)) == MOSQ_ERR_SUCCESS) {
		ret = mid;
	} else {
		fprintf(stderr, "mosquitto error %d %s\n", ret, mosquitto_strerror(ret));
		ret = -1;
	}
	return (ret);
}
