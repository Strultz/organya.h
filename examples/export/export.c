#include <stdlib.h>
#include <stdio.h>

#define ORGANYA_IMPLEMENTATION
#include "organya.h"

#define SAMPLE_RATE 44100

static void file_write_u16(FILE *file, org_uint16 value)
{
    fputc(value & 0xFF, file);
    fputc((value >> 8) & 0xFF, file);
}

static void file_write_u32(FILE *file, org_uint32 value)
{
    fputc(value & 0xFF, file);
    fputc((value >> 8) & 0xFF, file);
    fputc((value >> 16) & 0xFF, file);
    fputc((value >> 24) & 0xFF, file);
}

static org_uint32 ceil_double_u32(double value)
{
    org_uint32 out = (org_uint32)value;
    return value > out ? out + 1 : out;
}

int main(int argc, char *argv[])
{
    FILE *file;
    organya_context ctx;
    org_uint32 num_samples;
    org_uint32 stream_size;
    org_uint32 to_do;
    float buffer[128 * 2];

    /* Check if args are invalid */
    if (argc <= 2)
    {
        printf("Usage: %s <song path> <soundbank path> [output path]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Create Organya context */
    if (organya_context_init(&ctx) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        return EXIT_FAILURE;
    }

    /* Set properties */
    organya_context_set_sample_rate(&ctx, SAMPLE_RATE);
    organya_context_set_interpolation(&ctx, ORG_INTERPOLATION_CUBIC);
    organya_context_set_volume(&ctx, 1);

    /* Load soundbank from file */
    printf("Loading soundbank %s\n", argv[2]);

    if (organya_context_load_soundbank_file(&ctx, argv[2]) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        organya_context_deinit(&ctx);
        return EXIT_FAILURE;
    }

    /* Load .org file */
    printf("Loading song %s\n", argv[1]);

    if (organya_context_load_song_file(&ctx, argv[1]) != ORG_RESULT_SUCCESS)
    {
        puts("Error!");
        organya_context_deinit(&ctx);
        return EXIT_FAILURE;
    }

    /* Get number of samples */
    num_samples = ceil_double_u32(((double)ctx.song.tempo_ms * (double)SAMPLE_RATE / 1000.0) * ctx.song.repeat_end);
    stream_size = sizeof(float) * num_samples * 2;

    printf("Exporting to file %s\n", argc > 3 ? argv[3] : "out.wav");

    file = fopen(argc > 3 ? argv[3] : "out.wav", "wb");
    if (file == NULL)
    {
        puts("Error!");
        organya_context_deinit(&ctx);
        return EXIT_FAILURE;
    }

    /* Write .wav header */
    file_write_u32(file, 0x46464952); /* "RIFF" */
    file_write_u32(file, 36 + stream_size); /* "RIFF" length */
    file_write_u32(file, 0x45564157); /* "WAVE" */
    file_write_u32(file, 0x20746D66); /* "fmt " */
    file_write_u32(file, 0x10); /* "fmt " length */
    file_write_u16(file, 3); /* Floating-point PCM format */
    file_write_u16(file, 2); /* 2 channels for stereo */
    file_write_u32(file, SAMPLE_RATE); /* Samples per second */
    file_write_u32(file, SAMPLE_RATE * sizeof(float) * 2); /* Bytes per second */
    file_write_u16(file, 8); /* Bytes per sample */
    file_write_u16(file, 32); /* Bits per sample */
    file_write_u32(file, 0x61746164); /* "data" */
    file_write_u32(file, stream_size); /* "data" length */

    /* Generate samples */
    while (num_samples > 0)
    {
        to_do = num_samples < 128 ? num_samples : 128;
        organya_context_generate_samples(&ctx, buffer, to_do);
        fwrite(buffer, sizeof(float), to_do * 2, file);
        num_samples -= to_do;
    }

    fclose(file);

    return EXIT_SUCCESS;
}
