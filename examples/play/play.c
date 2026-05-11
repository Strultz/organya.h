#include <stdlib.h>
#include <stdio.h>

#define ORGANYA_IMPLEMENTATION
#include "organya.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static organya_context ctx;

void audio_process(ma_device *device, void *output_stream, const void *input_stream, ma_uint32 len)
{
    (void)device;
    (void)input_stream;

    /* Generate len samples into output */
    organya_context_generate_samples(&ctx, (float *)output_stream, len);
}

int main(int argc, char *argv[])
{
    ma_device device;
    ma_device_config config;

    /* Check if args are invalid */
    if (argc <= 2)
    {
        printf("Usage: %s <song path> <soundbank path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Setup miniaudio device */
    config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = NULL;
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 0;
    config.dataCallback = audio_process;
    config.pUserData = NULL;

    /* Initialize device */
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
    {
        puts("Error!");
        return EXIT_FAILURE;
    }

    /* Create Organya context */
    if (organya_context_init(&ctx) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        ma_device_uninit(&device);
        return EXIT_FAILURE;
    }

    /* Set properties */
    organya_context_set_sample_rate(&ctx, device.sampleRate);
    organya_context_set_interpolation(&ctx, ORG_INTERPOLATION_CUBIC);
    organya_context_set_volume(&ctx, 1);

    /* Load soundbank from file */
    printf("Loading soundbank %s\n", argv[2]);

    if (organya_context_load_soundbank_file(&ctx, argv[2]) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        ma_device_uninit(&device);
        organya_context_deinit(&ctx);
        return EXIT_FAILURE;
    }

    /* Load .org file */
    printf("Loading song %s\n", argv[1]);

    if (organya_context_load_song_file(&ctx, argv[1]) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        ma_device_uninit(&device);
        organya_context_deinit(&ctx);
        return EXIT_FAILURE;
    }

    /* Start the miniaudio device */
    ma_device_start(&device);

    puts("Playing.");
    while (1) {}

    return EXIT_SUCCESS;
}
