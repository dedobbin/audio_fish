/*
Based on example, reads from the default PCM device
and writes to standard output for 5 seconds of data.
*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "server.h"
#include "config_manager.h"

int main() {
	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *audio_buffer;

	/* Init config */
	config_init();
	char port_str[100];
	config_read("server_port", port_str);
	if (strcmp(port_str, "") == 0){
		printf("Failed to read from config\n");
		exit(1);
	}
	char message_size_str[100];
	config_read("server_message_size", message_size_str);
	if (strcmp(message_size_str, "") == 0){
		printf("Failed to read from config\n");
		exit(1);
	}
	int server_message_size = atoi(message_size_str);
	char audio_input_path[100];
	config_read("audio_input_path", audio_input_path);
	if (strcmp(audio_buffer, "") == 0){
		printf("Failed to read from config\n");
		exit(1);
	}
	config_exit();

	/* Init server */
	server_start(atoi(port_str));
	char server_send_buffer[server_message_size];
	int server_send_buffer_idx = 0;

	/* Audio stuff starts */
	rc = snd_pcm_open(&handle, "default",
						SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr,
				"unable to open pcm device: %s\n",
				snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,
						SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params,
								SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params,
									&val, &dir);

	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle,
								params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,
				"unable to set hw parameters: %s\n",
				snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames,
										&dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	audio_buffer = (char *) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,
										&val, &dir);
	/* 5 seconds in microseconds divided by
	* period time */
	loops = 25000000 / val;

	FILE* fp = fopen(audio_input_path, "r");
	while (loops > 0) {
		loops--;
		rc = fread(audio_buffer, 1, size, fp);
		if (rc == 0) {
			fprintf(stderr, "end of file on input\n");
			break;
		} else if (rc != size) {
			fprintf(stderr,
					"short read: read %d bytes\n", rc);
		}
		memcpy(server_send_buffer + server_send_buffer_idx, audio_buffer, size);
		server_send_buffer_idx+= size;
		if (server_send_buffer_idx >= server_message_size){
			server_send_buffer_idx = 0;
			server_send(server_send_buffer, server_message_size);
		}

		rc = snd_pcm_writei(handle, audio_buffer, frames);
		if (rc == -EPIPE) {
		/* EPIPE means underrun */
		fprintf(stderr, "underrun occurred\n");
		snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr,
					"error from writei: %s\n",
					snd_strerror(rc));
		}  else if (rc != (int)frames) {
		fprintf(stderr,
				"short write, write %d frames\n", rc);
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	
	printf("Requesting server shutdown..\n");
	server_close();
	
	free(audio_buffer);
	
	return 0;
}
