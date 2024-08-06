#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define ORGANYA_IMPLEMENTATION
#include "organya.h"

#define WAV_HEADER_SIZE 44
#define PATH_SIZE     2048

#define WRITE_16_LE(p, v) \
    do { \
        *p++ = ((v) & 0xFF); \
        *p++ = ((v) >> 8 & 0xFF); \
    } while(0)

#define WRITE_32_LE(p, v) \
    do { \
        *p++ = ((v) & 0xFF); \
        *p++ = ((v) >> 8 & 0xFF); \
        *p++ = ((v) >> 16 & 0xFF); \
        *p++ = ((v) >> 24 & 0xFF); \
    } while(0)

int main(int argc, char *argv[]) {
    char *pC;
    FILE *file;
    org_uint8 *p;
    char save_path[PATH_SIZE];
    organya_context *ctx;
    organya_song *song;
    int resample_mode;
    int sample_rate;
    int loop_count;
    org_uint32 num_samples;
    org_uint32 stream_size;
    org_uint8 *wav_buffer;

    /* Check if args are invalid */
    if (argc <= 1) {
        puts("Must supply filename");
        return EXIT_FAILURE;
    }
    
    /* Get properties */
    if (argc < 5) {
        if (argc < 4) {
            if (argc < 3) {
                puts("Enter sample rate:");
                if (scanf("%d", &sample_rate) != 1) {
                    sample_rate = -1;
                }
                putchar('\n');
            } else {
                sample_rate = strtoul(argv[2], NULL, 10);
            }
            
            puts("Enter loop count:");
            if (scanf("%d", &loop_count) != 1) {
                loop_count = -1;
            }
            putchar('\n');
        } else {
            sample_rate = strtoul(argv[2], NULL, 10);
            loop_count = strtoul(argv[3], NULL, 10);
        }
        
        puts("Enter resample mode:");
        puts("  0 - Nearest");
        puts("  1 - Linear (default)");
        puts("  2 - Lanczos");
        if (scanf("%d", &resample_mode) != 1) {
            resample_mode = -1;
        }
        putchar('\n');
    } else {
        sample_rate = strtoul(argv[2], NULL, 10);
        loop_count = strtoul(argv[3], NULL, 10);
        resample_mode = strtoul(argv[4], NULL, 10);
    }

    if (sample_rate <= 0) {
        sample_rate = 48000;
        puts("Invalid sample rate.");
    }
    
    if (loop_count < 0) {
        loop_count = 0;
        puts("Invalid loop count.");
    }
    
    if (resample_mode < 0 || resample_mode >= 3) {
        resample_mode = 1;
        puts("Invalid resample mode. Using Linear.");
    }
    
    /* Create Organya context */
    ctx = organya_context_create();
    
    if (ctx == NULL) {
        puts("Error!");
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Load .org file */
    printf("Loading file %s\n", argv[1]);
    
    if (organya_context_load_song_file(ctx, argv[1])) {
        puts("Error!");
        organya_context_destroy(ctx);
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Set properties */
    organya_context_set_sampling_rate(ctx, sample_rate);
    organya_context_set_resample_mode(ctx, (organya_resample_mode)resample_mode);
    
    /* Get song */
    song = organya_context_get_song(ctx);
    
    /* Get save path */
    save_path[0] = '\0';
    strncat(save_path, argv[1], PATH_SIZE - 1);
    pC = strrchr(save_path, '.');
    if (pC != NULL) pC[0] = '\0';
    strncat(save_path, ".wav", PATH_SIZE - 1);
    
    printf("Exporting to file %s\n", save_path);
    
    /* Get number of samples */
    num_samples = (org_uint32)ceil(
        ((double)song->tempo_ms * (double)sample_rate / 1000.0)
        * (song->repeat_end + ((song->repeat_end - song->repeat_start) * loop_count))
    );
    
    stream_size = sizeof(org_int16) * num_samples * 2;
    
    /* Allocate buffer */
    wav_buffer = (org_uint8 *)malloc(WAV_HEADER_SIZE + stream_size);
    
    if (wav_buffer == NULL) {
        puts("Error!");
        organya_context_destroy(ctx);
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Write wav header */
    p = wav_buffer;
    
    WRITE_32_LE(p, 0x46464952); /* "RIFF" */
    WRITE_32_LE(p, 36 + stream_size); /* "RIFF" length */
    WRITE_32_LE(p, 0x45564157); /* "WAVE" */
    WRITE_32_LE(p, 0x20746D66); /* "fmt " */
    WRITE_32_LE(p, 0x10); /* "fmt " length */
    WRITE_16_LE(p, 1); /* PCM format */
    WRITE_16_LE(p, 2); /* 2 channels for stereo */
    WRITE_32_LE(p, sample_rate); /* Samples per second */
    WRITE_32_LE(p, sample_rate * sizeof(org_int16) * 2); /* Bytes per second */
    WRITE_16_LE(p, 4); /* Bytes per sample */
    WRITE_16_LE(p, 16); /* Bits per sample */
    WRITE_32_LE(p, 0x61746164); /* "data" */
    WRITE_32_LE(p, stream_size); /* "data" length */
    
    /* Start playing */
    organya_context_play(ctx);
    
    /* Generate num_samples samples into wav_buffer */
    organya_context_generate_samples(ctx, (org_int16 *)(wav_buffer + WAV_HEADER_SIZE), num_samples);
    
    /* Stop playing */
    organya_context_stop(ctx);
    
    /* Destroy the context */
    organya_context_destroy(ctx);
    
    /* Open file */
    file = fopen(save_path, "wb");
    if (file == NULL) {
        puts("Error!");
        free(wav_buffer);
        return EXIT_FAILURE; /* Failed */
    }
    
    /* Write data */
    fwrite(wav_buffer, 1, WAV_HEADER_SIZE + stream_size, file);
    
    /* Close file */
    fclose(file);
    
    /* Done */
    puts("File saved");
    
    return EXIT_SUCCESS;
}