#include <stdlib.h>
#include <stdio.h>

#define ORGANYA_IMPLEMENTATION
#include "organya.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static organya_context *ctx;

void audio_process(ma_device *device, void *output_stream, const void *input_stream, ma_uint32 len) {
    (void)device;
    (void)input_stream;
    
    /* Generate len samples into output */
    organya_context_generate_samples(ctx, (org_int16 *)output_stream, len);
}

int main(int argc, char *argv[]) {
    char c;
    int resample_mode;
    ma_device device;
    ma_device_config config;
    
    /* Check if args are invalid */
    if (argc <= 1) {
        puts("Must supply filename");
        return EXIT_FAILURE; /* Invalid */
    }
    
    if (argc <= 2) { 
        puts("Enter resample mode:");
        puts("  0 - Nearest");
        puts("  1 - Linear (default)");
        puts("  2 - Lanczos");
        if (scanf("%d", &resample_mode) != 1) {
            resample_mode = -1;
        }
        putchar('\n');
    } else {
        resample_mode = strtoul(argv[2], NULL, 10);
    }
    
    if (resample_mode < 0 || resample_mode >= 3) {
        resample_mode = 1;
        puts("Invalid resample mode. Using Linear.");
    }
    
    /* Setup miniaudio device */
    config = ma_device_config_init(ma_device_type_playback);
	config.playback.pDeviceID = NULL;
	config.playback.format = ma_format_s16;
	config.playback.channels = 2;
	config.sampleRate = 0;
	config.dataCallback = audio_process;
	config.pUserData = NULL;
    
    /* Initialize device */
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        puts("Error!");
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Create Organya context */
    ctx = organya_context_create();
    
    if (ctx == NULL) {
        puts("Error!");
        ma_device_uninit(&device);
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Set properties */
    organya_context_set_sampling_rate(ctx, device.sampleRate);
    organya_context_set_resample_mode(ctx, (org_resample_mode)resample_mode);
    
    /* Load .org file */
    printf("Loading file %s\n", argv[1]);
    
    if (organya_context_load_song_file(ctx, argv[1]) < 0) {
        puts("Error!");
        ma_device_uninit(&device);
        organya_context_destroy(ctx);
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Start the miniaudio device */
    ma_device_start(&device);
    
    /* Start playing */
    organya_context_play(ctx);
    
    puts("Playing. Type C to close.");
    
    /* Pause */
    while ((c = getchar()) != 'c' && c != 'C') continue;
    
    /* Stop playing */
    organya_context_stop(ctx);
    
    /* Stop the miniaudio device */
    ma_device_stop(&device);
    ma_device_uninit(&device);
    
    /* Destroy the context */
    organya_context_destroy(ctx);
    
    /* Done */
    return 0;
}