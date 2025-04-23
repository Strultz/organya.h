/*  organya.h v1.02
    Author: Strultz

# organya.h

## Description

organya.h is a simple C89 library for reading and decoding Organya music (.org files).

Organya is a sequenced music format created in 1999 by Studio Pixel. It is the predecessor to PxTone
and was used in games such as Cave Story, Azarashi (2001 version), STARGAZER, and more.

## Usage

To use this library, just `#include "organya.h"` in your project. Define `ORGANYA_IMPLEMENTATION`
before including the header in one .c file to create the implementation.
You should also link with `-lm` on Linux/BSD systems.

## Example

```c
#define ORGANYA_IMPLEMENTATION
#include "organya.h"

int main()
{
    // Create the context:
    organya_context ctx;
    if (organya_context_init(&ctx) != ORG_RESULT_SUCCESS)
    {
        // Handle the error here
        return -1;
    }

    // Set everything up (these are the default settings):
    organya_context_set_sample_rate(&ctx, 44100);
    organya_context_set_interpolation(&ctx, ORG_INTERPOLATION_LAGRANGE);
    organya_context_set_volume(&ctx, 1);
    // Note: Using Lagrange interpolation produces output that sounds almost completely
    // identical to the original Organya playback (on Windows Vista and later)

    // Load a soundbank from a file path:
    if (organya_context_load_soundbank_file(&ctx, "path/to/file.wdb") != ORG_RESULT_SUCCESS)
    {
        // Handle the error here
        organya_context_deinit(&ctx);
        return -1;
    }
    // Then load a song from a file path:
    if (organya_context_load_song_file(&ctx, "path/to/file.org") != ORG_RESULT_SUCCESS)
    {
        // Handle the error here
        organya_context_deinit(&ctx);
        return -1;
    }
    // Alternatively, you can load both of these directly from some pointer using
    // organya_context_read_soundbank() and organya_context_read_song().

    // Generate samples (which would then be output to an audio player, or a .wav file, or etc.)
    // This will output interleaved stereo 32-bit floating point PCM to output_buffer.
    // output_buffer should be at least num_samples * sizeof(float) * 2 long.
    organya_context_generate_samples(&ctx, output_buffer, num_samples);

    // When you're done, deinitialize and free everything.
    organya_context_deinit(&ctx);
    return 0;
}
```

See bottom of file for license information.
*/

#ifndef ORGANYA_H_
#define ORGANYA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef ORG_USE_STDINT
    #include <stdint.h>
    typedef int8_t   org_int8;
    typedef uint8_t  org_uint8;
    typedef int16_t  org_int16;
    typedef uint16_t org_uint16;
    typedef int32_t  org_int32;
    typedef uint32_t org_uint32;
#else
    typedef signed   char  org_int8;
    typedef unsigned char  org_uint8;
    typedef signed   short org_int16;
    typedef unsigned short org_uint16;
    typedef signed   long  org_int32;
    typedef unsigned long  org_uint32;
#endif

typedef org_uint8 org_bool;
#define ORG_TRUE 1
#define ORG_FALSE 0

#ifndef ORG_API
    #define ORG_API extern
#endif

#ifndef ORG_PRIVATE
    #define ORG_PRIVATE static
#endif

#define ORG_WAVETABLE_COUNT             100             /* Total number of sounds for melody channels */
#define ORG_PERCUSSION_COUNT            42              /* Total number of sounds for percussion channels */

#define ORG_MELODY_CHANNEL_COUNT        8               /* Number of melody channels */
#define ORG_PERCUSSION_CHANNEL_COUNT    8               /* Number of percussion channels */

#define ORG_CHANNEL_COUNT               (ORG_MELODY_CHANNEL_COUNT + ORG_PERCUSSION_CHANNEL_COUNT)

#define ORG_PROPERTY_NOT_USED           0xFF            /* Value for event properties that aren't used */
#define ORG_DEFAULT_VOLUME              200             /* Default volume of notes */
#define ORG_DEFAULT_PAN                 6               /* Default pan of notes */

/* --- */

typedef enum organya_result_e
{
    ORG_RESULT_SUCCESS = 0,     /* Successful */
    ORG_RESULT_ERROR,           /* Generic failure status */
    ORG_RESULT_INVALID_ARGS,    /* Passed in arguments were invalid */
    ORG_RESULT_FILE_ERROR,      /* File didn't exist or hit EOF while reading */
    ORG_RESULT_MEMORY_ERROR     /* Memory allocation failed */
} organya_result;

/* --- Song Handling --- */

typedef struct organya_event_s
{
    org_uint32 position;        /* X position of the event */
    org_uint8 pitch;            /* Pitch of the note (0 to 95 or ORG_PROPERTY_NOT_USED if none) */
    org_uint8 length;           /* Length of the note (should never be 0) */
    org_uint8 volume;           /* Volume of the note (0 to 254 or ORG_PROPERTY_NOT_USED if none) */
    org_uint8 pan;              /* Panning of the note (0 to 12 or ORG_PROPERTY_NOT_USED if none) */
} organya_event;

typedef struct organya_channel_s
{
    org_uint8 instrument;       /* Channel instrument index */
    org_uint16 finetune;        /* Channel finetune, only used for melody channels (default is 1000) */
    org_bool pizzicato;         /* Notes will not loop, only used for melody channels */
    size_t event_count;
    organya_event *event_list;
} organya_channel;

typedef struct organya_song_s
{
    org_uint16 tempo_ms;        /* How long one tick takes in milliseconds */
    org_uint8 beats;            /* Number of beats per bar. Does not affect playback */
    org_uint8 steps;            /* Number of steps per beat. Does not affect playback */
    org_uint32 repeat_start;    /* Repeat range start X position */
    org_uint32 repeat_end;      /* Repeat range end X position */
    organya_channel channels[ORG_CHANNEL_COUNT];
} organya_song;

/**
 * Initializes a default song structure.
 *
 * @param song Pointer to the organya_song structure
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_song_init(organya_song *song);

/**
 * Frees any existing event data.
 *
 * @param song Pointer to the organya_song structure
 */
ORG_API void organya_song_deinit(organya_song *song);

/**
 * Frees any existing event data and sets properties to their defaults.
 *
 * @param song Pointer to the organya_song structure
 * @param file_path Path of file to load
 */
ORG_API void organya_song_clean(organya_song *song);

#ifndef ORG_NO_STDIO

/**
 * Loads and reads Organya data from a file.
 *
 * @param song Pointer to the organya_song structure
 * @param file_path Path of file to load
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_song_load_file(organya_song *song, const char *file_path);

#endif

/**
 * Reads Organya data.
 *
 * @param song Pointer to the organya_song structure
 * @param song_data Song data bytes - this would be the contents of an org file
 * @param data_length Length of song_data
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_song_read(organya_song *song, const org_uint8 *song_data, size_t data_length);

/* --- Context Handling --- */

#define ORG_MIN(a, b)   ((a) < (b) ? (a) : (b))
#define ORG_MAX(a, b)   ((a) > (b) ? (a) : (b))
#define ORG_ABS(a)      ((a) > 0 ? (a) : -(a))

#ifndef ORG_LANCZOS_WINDOW
    #define ORG_LANCZOS_WINDOW 4
#endif

#define ORG_MAX_MARGIN  ORG_MAX(ORG_LANCZOS_WINDOW, 2)
#define ORG_MAX_TAPS    (ORG_MAX_MARGIN * 2)

typedef enum organya_interpolation_e
{
    ORG_INTERPOLATION_NONE = 0,     /* Fastest speed, lowest quality */
    ORG_INTERPOLATION_LINEAR,       /* Fast speed, medium quality */
    ORG_INTERPOLATION_LAGRANGE,     /* Medium speed, high quality. Default. */
    ORG_INTERPOLATION_LANCZOS       /* Slow speed, highest quality */
} organya_interpolation;

/* Internal sample data */
typedef struct organya_internal_sound_s
{
    size_t sample_count;                                                /* Number of samples */
    org_int8 *data;                                                     /* Sample buffer */

    float samples[ORG_MAX_TAPS];                                        /* Active samples buffer */

    org_uint32 position;                                                /* Playback position */
    float sub_position;                                                 /* Subframe position */

    org_uint32 frequency;                                               /* Playback frequency */
    float position_increment;                                           /* Affects speed */

    org_uint32 ring;                                                    /* Samples position */

    org_bool playing;                                                   /* True if sound is currently playing */
    org_bool looping;                                                   /* True if sound should loop */

    org_uint32 out_sample_rate;                                         /* Output sample rate */
    org_uint32 out_volume_ramp;                                         /* Volume ramp speed */
    organya_interpolation interpolation;                                /* Current interpolation mode */

    float volume;                                                       /* Sound volume */
    float pan_left;                                                     /* Sound left pan */
    float pan_right;                                                    /* Sound right pan */

    float volume_left;                                                  /* Actual volume of left channel */
    float volume_right;                                                 /* Actual volume of right channel */

    float target_volume_left;                                           /* Target volume of left channel */
    float target_volume_right;                                          /* Target volume of right channel */

    org_uint32 volume_ticks;                                            /* Volume slide ticks */
    org_uint32 total_samples;                                           /* Internal number of samples */

    org_uint8 silence_timer;
} organya_internal_sound;

/* Channel playback data */
typedef struct organya_melody_s
{
    org_uint8 pitch;                      /* Current pitch (or ORG_PROPERTY_NOT_USED if nothing is playing) */
    org_uint8 volume;                     /* Current volume */
    org_uint8 pan;                        /* Current pan */

    size_t index;                         /* Next note index */
    org_uint32 ticks;                     /* Ticks left for current note */
    org_uint8 alt;                        /* Alternate key index */
    org_bool muted;                       /* Channel mute */

    organya_internal_sound sounds[8][2];  /* Internal sounds */
} organya_melody;

typedef struct organya_percussion_s
{
    org_uint8 pitch;                      /* Current pitch (or ORG_PROPERTY_NOT_USED if nothing is playing) */
    org_uint8 volume;                     /* Current volume */
    org_uint8 pan;                        /* Current pan */

    size_t index;                         /* Next note index */
    org_bool muted;                       /* Channel mute */

    organya_internal_sound sound;         /* Internal sound */
} organya_percussion;

/* The main context */
typedef struct organya_context_s
{
    /* Song playback data */
    organya_song song;
    org_uint32 position;
    org_uint32 last_position;
    double samples_to_next_tick;
    org_uint32 volume_ramp;

    organya_melody melody_index[ORG_MELODY_CHANNEL_COUNT];
    organya_percussion percussion_index[ORG_PERCUSSION_CHANNEL_COUNT];

    /* User settings */
    float volume;
    org_uint32 sample_rate;
    organya_interpolation interpolation;

    /* For soundbanks: */
    org_uint8 melody_wave_data[ORG_WAVETABLE_COUNT * 0x100];
    struct
    {
        org_uint32 length;
        org_uint8 *data;
    } percussion_wave_data[ORG_PERCUSSION_COUNT];
} organya_context;

/**
 * Initializes an Organya context.
 *
 * @param context Pointer to the organya_context structure
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_context_init(organya_context *context);

/**
 * Deinitializes an Organya context.
 *
 * @param context Pointer to the organya_context structure
 */
ORG_API void organya_context_deinit(organya_context *context);

#ifndef ORG_NO_STDIO

/**
 * Loads and reads soundbank data from a file.
 *
 * @param context Pointer to the organya_context structure
 * @param file_path Path of .wdb file to load
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_context_load_soundbank_file(organya_context *context, const char *file_path);

#endif

/**
 * Reads soundbank data.
 *
 * @param context Pointer to the organya_context structure
 * @param bank_data Soundbank data bytes - this would be the contents of a wdb file
 * @param data_length Length of bank_data
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_context_read_soundbank(organya_context *context, const org_uint8 *bank_data, size_t data_length);

/**
 * Sets the playback sample rate.
 *
 * @param context Pointer to the organya_context structure
 * @param sample_rate Samples per second
 */
ORG_API void organya_context_set_sample_rate(organya_context *context, org_uint32 sample_rate);

/**
 * Sets the playback volume.
 *
 * @param context Pointer to the organya_context structure
 * @param volume Playback volume (1 is full volume)
 */
ORG_API void organya_context_set_volume(organya_context *context, float volume);

/**
 * Sets the interpolation mode. Affects quality of pitch changes.
 *
 * @param context Pointer to the organya_context structure
 * @param mode Interpolation mode to use
 *
 * @see organya_interpolation
 */
ORG_API void organya_context_set_interpolation(organya_context *context, organya_interpolation mode);

#ifndef ORG_NO_STDIO

/**
 * Loads and reads Organya data from a file.
 *
 * @param context Pointer to the organya_context structure
 * @param file_path Path of file to load
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_context_load_song_file(organya_context *context, const char *file_path);

#endif

/**
 * Reads Organya data.
 *
 * @param context Pointer to the organya_context structure
 * @param song_data Song data bytes - this would be the contents of an org file
 * @param data_length Length of song_data
 *
 * @returns Success/failure status; see organya_result enum for possible values
 */
ORG_API organya_result organya_context_read_song(organya_context *context, const org_uint8 *song_data, size_t data_length);

/**
 * Unloads the currently loaded song.
 *
 * @param context Pointer to the organya_context structure
 */
ORG_API void organya_context_unload_song(organya_context *context);

/**
 * Set current song position.
 *
 * @param context Pointer to the organya_context structure
 * @param position Position to seek to
 */
ORG_API void organya_context_seek(organya_context *context, org_uint32 position);

/**
 * Set if a channel is muted.
 *
 * @param context Pointer to the organya_context structure
 * @param channel Index of channel to mute
 * @param mute If the channel should be muted
 */
ORG_API void organya_context_set_mute(organya_context *context, size_t channel, org_bool mute);

/**
 * Generate samples of Organya playback.
 * This function will write interleaved stereo samples to output in 32-bit floating point PCM format.
 *
 * @param context Pointer to the organya_context structure
 * @param output Buffer, with sample_count * 2 elements
 * @param sample_count Number of samples to generate
 *
 * @returns Number of samples generated
 */
ORG_API size_t organya_context_generate_samples(organya_context *context, float *output, size_t sample_count);

/**
 * Ticks the internal Organya player.
 *
 * @param context Pointer to the organya_context structure
 */
ORG_API void organya_context_tick(organya_context *context);

#ifdef __cplusplus
}
#endif

#endif /* ORGANYA_H_ */

/* Implementation */
#ifdef ORGANYA_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef ORG_NO_STDIO
#include <stdio.h>
#endif

#ifndef ORG_MALLOC
    #define ORG_MALLOC(s) malloc(s)
#endif
#ifndef ORG_FREE
    #define ORG_FREE(p) free(p)
#endif

#define ORG_PI 3.14159265358979323846

#define ORG_CLAMP(a, n, x) ((a) > (x) ? (x) : ((a) < (n) ? (n) : (a)))
#define ORG_MOD_CLAMP(a, x, y) ((a) >= (x) ? (a) - (x) : ((a) < (y) ? ((a) - (y)) + (x) : (a)))

#define ORG_SINC(x) (sin(x) / (x))

#define ORG_READ_8_LE(p)    ((p)[0])
#define ORG_READ_16_LE(p)   (((p)[1] << 8) | (p)[0])
#define ORG_READ_32_LE(p)   (((p)[3] << 24) | ((p)[2] << 16) | ((p)[1] << 8) | (p)[0])

#define ORG_MIN_SONG_SIZE       (6 + 12 + (ORG_CHANNEL_COUNT * 6))
#define ORG_MIN_SOUNDBANK_SIZE  ((ORG_WAVETABLE_COUNT * 0x100) + (ORG_PERCUSSION_COUNT * 4))

/* --- Playback Handling --- */

ORG_PRIVATE organya_result organya_internal_sound_init(organya_internal_sound *sound, organya_context *context, size_t sample_count);
ORG_PRIVATE void organya_internal_sound_deinit(organya_internal_sound *sound);
ORG_PRIVATE void organya_internal_sound_set_frequency(organya_internal_sound *sound, org_uint32 freq);
ORG_PRIVATE void organya_internal_sound_set_volume(organya_internal_sound *sound, org_int16 volume_db);
ORG_PRIVATE void organya_internal_sound_set_pan(organya_internal_sound *sound, org_int16 pan_db);
ORG_PRIVATE void organya_internal_sound_play(organya_internal_sound *sound, org_bool loop);
ORG_PRIVATE void organya_internal_sound_stop(organya_internal_sound *sound);
ORG_PRIVATE void organya_internal_sound_generate_sample(organya_internal_sound *sound, float *output);

/* --- Context Handling --- */

/* Constant tables */
static const org_int16 organya_wave_size_table[ORG_MELODY_CHANNEL_COUNT] = {256, 256, 128, 128, 64, 32, 16, 8};
static const org_int16 organya_frequency_table[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
static const org_int16 organya_panning_table[13] = {0, 43, 86, 129, 172, 215, 256, 297, 340, 383, 426, 469, 512};

/* organya_song */

ORG_API organya_result organya_song_init(organya_song *song)
{
    size_t i;

    if (song == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    /* song_clean tries to free event_list, so make sure it's NULL */
    for (i = 0; i < ORG_CHANNEL_COUNT; ++i)
    {
        song->channels[i].event_list = NULL;
    }

    /* Initialize song data */
    organya_song_clean(song);

    return ORG_RESULT_SUCCESS;
}

ORG_API void organya_song_deinit(organya_song *song)
{
    size_t i;

    if (song == NULL)
    {
        return;
    }

    /* Free channels */
    for (i = 0; i < ORG_CHANNEL_COUNT; ++i)
    {
        if (song->channels[i].event_list != NULL)
        {
            ORG_FREE(song->channels[i].event_list);
            song->channels[i].event_list = NULL;
        }
    }
}

ORG_API void organya_song_clean(organya_song *song)
{
    size_t i;

    if (song == NULL)
    {
        return;
    }

    /* These are the default song properties in OrgMaker (as of 3.1.0) */
    song->tempo_ms = 125; /* 125 ms = 120 bpm (with the default settings) */
    song->beats = 4;
    song->steps = 4;
    song->repeat_start = 0;
    song->repeat_end = 100 * (song->beats * song->steps);

    /* Reset melody channel */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        song->channels[i].instrument = (org_uint8)(i * 11);
        song->channels[i].finetune = 1000;
        song->channels[i].pizzicato = ORG_FALSE;

        song->channels[i].event_count = 0;
        if (song->channels[i].event_list != NULL)
        {
            ORG_FREE(song->channels[i].event_list);
            song->channels[i].event_list = NULL;
        }
    }

    /* Reset percussion channel */
    for (i = ORG_MELODY_CHANNEL_COUNT; i < ORG_CHANNEL_COUNT; ++i)
    {
        song->channels[i].instrument = 0;
        song->channels[i].finetune = 1000;
        song->channels[i].pizzicato = ORG_FALSE;

        song->channels[i].event_count = 0;
        if (song->channels[i].event_list != NULL)
        {
            ORG_FREE(song->channels[i].event_list);
            song->channels[i].event_list = NULL;
        }
    }

    /* Default percussion instruments
     * XXX: This should be improved. Maybe a table? */
    song->channels[ORG_MELODY_CHANNEL_COUNT + 0].instrument = 0;
    song->channels[ORG_MELODY_CHANNEL_COUNT + 1].instrument = 2;
    song->channels[ORG_MELODY_CHANNEL_COUNT + 2].instrument = 5;
    song->channels[ORG_MELODY_CHANNEL_COUNT + 3].instrument = 6;
    song->channels[ORG_MELODY_CHANNEL_COUNT + 4].instrument = 4;
    song->channels[ORG_MELODY_CHANNEL_COUNT + 5].instrument = 8;
}

ORG_API organya_result organya_song_read(organya_song *song, const org_uint8 *song_data, size_t data_length)
{
    size_t i, j;
    size_t offset;
    org_uint16 version;

    if (song == NULL || song_data == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    /* Sanity check */
    if (data_length < ORG_MIN_SONG_SIZE)
    {
        return ORG_RESULT_FILE_ERROR;
    }

    offset = 0;

    /* .org files always start with "Org-" */
    if (ORG_READ_32_LE(&song_data[offset]) != 0x2D67724F) /* "Org-" */
    {
        return ORG_RESULT_FILE_ERROR;
    }

    offset += 4;

    /* Read and check version (read done manually here as it's in ASCII) */
    version = (song_data[offset] - 48) * 10 + (song_data[offset + 1] - 48); offset += 2;
    if (version < 1 || version > 3) /* Org-01, Org-02, Org-03 */
    {
        return ORG_RESULT_FILE_ERROR;
    }

    /* Clear the previous song data */
    organya_song_clean(song);

    /* Read song header */
    song->tempo_ms = ORG_READ_16_LE(&song_data[offset]); offset += 2;
    song->beats = ORG_READ_8_LE(&song_data[offset]); ++offset;
    song->steps = ORG_READ_8_LE(&song_data[offset]); ++offset;
    song->repeat_start = ORG_READ_32_LE(&song_data[offset]); offset += 4;
    song->repeat_end = ORG_READ_32_LE(&song_data[offset]); offset += 4;

    /* Read channel headers */
    for (i = 0; i < ORG_CHANNEL_COUNT; ++i)
    {
        song->channels[i].finetune = ORG_READ_16_LE(&song_data[offset]); offset += 2;
        song->channels[i].instrument = ORG_READ_8_LE(&song_data[offset]); ++offset;

        /* Org-01 doesn't have this option, but there's still a byte here in the file structure */
        song->channels[i].pizzicato = (version > 1 ? ORG_READ_8_LE(&song_data[offset]) : 0); ++offset;

        /* Check for invalid instrument */
        if ((i < ORG_MELODY_CHANNEL_COUNT && song->channels[i].instrument >= ORG_WAVETABLE_COUNT)
        || (i >= ORG_MELODY_CHANNEL_COUNT && song->channels[i].instrument >= ORG_PERCUSSION_COUNT))
        {
            song->channels[i].instrument = 0;
        }

        song->channels[i].event_count = ORG_READ_16_LE(&song_data[offset]); offset += 2;
    }

    /* Read event data */
    for (i = 0; i < ORG_CHANNEL_COUNT; ++i)
    {
        if (song->channels[i].event_count == 0)
        {
            /* No events here */
            continue;
        }

        /* Check file size. Each event is worth 8 bytes */
        if (offset + (song->channels[i].event_count * 8) > data_length)
        {
            organya_song_clean(song);
            return ORG_RESULT_FILE_ERROR;
        }

        /* Allocate event list */
        song->channels[i].event_list = (organya_event *)ORG_MALLOC(song->channels[i].event_count * sizeof(organya_event));
        if (song->channels[i].event_list == NULL)
        {
            organya_song_clean(song);
            return ORG_RESULT_MEMORY_ERROR;
        }

        /* Read events */
        for (j = 0; j < song->channels[i].event_count; j++)
        {
            song->channels[i].event_list[j].position = ORG_READ_32_LE(&song_data[offset + j * 4]);
            song->channels[i].event_list[j].pitch = ORG_READ_8_LE(&song_data[offset + song->channels[i].event_count * 4 + j]);
            song->channels[i].event_list[j].length = ORG_READ_8_LE(&song_data[offset + song->channels[i].event_count * 5 + j]);
            song->channels[i].event_list[j].volume = ORG_READ_8_LE(&song_data[offset + song->channels[i].event_count * 6 + j]);
            song->channels[i].event_list[j].pan = ORG_READ_8_LE(&song_data[offset + song->channels[i].event_count * 7 + j]);

            /* Note pitch goes from 0-95, or 255 if there is no note there */
            if (song->channels[i].event_list[j].pitch >= (12 * 8) && song->channels[i].event_list[j].pitch != ORG_PROPERTY_NOT_USED)
            {
                song->channels[i].event_list[j].pitch = ORG_PROPERTY_NOT_USED;
            }

            /* Note volume goes from 0-254, or 255 if there is no volume there */
            if (song->channels[i].event_list[j].length == 0)
            {
                song->channels[i].event_list[j].length = 1;
            }

            /* Note panning goes from 0-12, or 255 if there is no panning there */
            if (song->channels[i].event_list[j].pan > 12 && song->channels[i].event_list[j].pan != ORG_PROPERTY_NOT_USED)
            {
                song->channels[i].event_list[j].pan = ORG_DEFAULT_PAN;
            }
        }

        offset += song->channels[i].event_count * 8;
    }

    return ORG_RESULT_SUCCESS;
}

#ifndef ORG_NO_STDIO

ORG_API organya_result organya_song_load_file(organya_song *song, const char *file_path)
{
    organya_result result;
    size_t file_size;
    org_uint8 *buffer;
    FILE *file;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    errno_t err;

    /* Open file */
    err = fopen_s(&file, file_path, "rb");
    if (err != 0)
    {
        return ORG_RESULT_FILE_ERROR;
    }
#elif !defined(_WIN32) && !defined(__APPLE__) && defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64 && defined(_LARGEFILE64_SOURCE)
    file = fopen64(file_path, "rb");
#else
    file = fopen(file_path, "rb");
#endif

    if (file == NULL)
    {
        return ORG_RESULT_FILE_ERROR;
    }

    /* Seek to end */
    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Get file size */
    file_size = ftell(file);

    if (file_size == 0)
    {
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Return to start */
    rewind(file);

    /* Create buffer to store file contents in */
    buffer = (org_uint8 *)ORG_MALLOC(file_size);

    if (buffer == NULL)
    {
        fclose(file);
        return ORG_RESULT_MEMORY_ERROR;
    }

    /* Load file to buffer */
    if (fread(buffer, 1, file_size, file) < file_size)
    {
        ORG_FREE(buffer);
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Close file */
    fclose(file);

    /* Read file data */
    result = organya_song_read(song, buffer, file_size);
    ORG_FREE(buffer);
    return result;
}

#endif

/* organya_context */

ORG_PRIVATE organya_result organya_internal_context_load_instruments(organya_context *context)
{
    organya_result result;
    size_t i, j, k;
    size_t sample_count;
    size_t wave_index;

    const org_uint8 *percussion_data;

    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            sample_count = organya_wave_size_table[j];

            /* Increase the sound length if pizzicato is enabled.
             * Otherwise it will just sound like a pop. */
            if (context->song.channels[i].pizzicato)
            {
                sample_count *= (4 + j * 4);
            }

            for (k = 0; k < 2; ++k)
            {
                /* Delete old sound if there is one */
                organya_internal_sound_deinit(&context->melody_index[i].sounds[j][k]);

                /* Create sound object */
                result = organya_internal_sound_init(&context->melody_index[i].sounds[j][k], context, sample_count);
                if (result != ORG_RESULT_SUCCESS)
                {
                    return result;
                }
            }

            /* Load waveform into sound */
            wave_index = 0;

            for (k = 0; k < sample_count; ++k)
            {
                /* Set both sound's samples */
                context->melody_index[i].sounds[j][0].data[k] = context->melody_index[i].sounds[j][1].data[k] =
                        context->melody_wave_data[context->song.channels[i].instrument * 0x100 + wave_index];

                wave_index = (wave_index + (0x100 / organya_wave_size_table[j])) & 0xFF;
            }
        }
    }

    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        percussion_data = context->percussion_wave_data[context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].instrument].data;

        if (percussion_data == NULL)
        {
            continue;
        }

        /* Read sound length */
        sample_count = context->percussion_wave_data[context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].instrument].length;

        /* Delete old sound if there is one */
        organya_internal_sound_deinit(&context->percussion_index[i].sound);

        /* Create sound object */
        result = organya_internal_sound_init(&context->percussion_index[i].sound, context, sample_count);
        if (result != ORG_RESULT_SUCCESS)
        {
            return result;
        }

        /* Read percussion samples */
        for (j = 0; j < sample_count; ++j)
        {
            context->percussion_index[i].sound.data[j] = percussion_data[j] + 0x80;
        }
    }

    return ORG_RESULT_SUCCESS;
}

ORG_PRIVATE void organya_internal_context_set_sound_settings(organya_context *context)
{
    size_t i, j, k;

    if (context == NULL)
    {
        return;
    }

    /* Update all melody sounds data */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            for (k = 0; k < 2; ++k)
            {
                context->melody_index[i].sounds[j][k].out_sample_rate = context->sample_rate;
                context->melody_index[i].sounds[j][k].out_volume_ramp = context->volume_ramp;
                context->melody_index[i].sounds[j][k].interpolation = context->interpolation;

                organya_internal_sound_set_frequency(&context->melody_index[i].sounds[j][k], context->melody_index[i].sounds[j][k].frequency);
            }
        }
    }

    /* Update all percussion sounds data */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        context->percussion_index[i].sound.out_sample_rate = context->sample_rate;
        context->percussion_index[i].sound.out_volume_ramp = context->volume_ramp;
        context->percussion_index[i].sound.interpolation = context->interpolation;

        organya_internal_sound_set_frequency(&context->percussion_index[i].sound, context->percussion_index[i].sound.frequency);
    }
}

ORG_PRIVATE organya_result organya_internal_context_generate_sample(organya_context *context, float *buffer)
{
    size_t i, j;

    if (context == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    buffer[0] = 0;
    buffer[1] = 0;

    if (context->samples_to_next_tick <= 0)
    {
        organya_context_tick(context);
    }
    --context->samples_to_next_tick;

    /* Generate melody samples */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            organya_internal_sound_generate_sample(&context->melody_index[i].sounds[j][0], buffer);
            organya_internal_sound_generate_sample(&context->melody_index[i].sounds[j][1], buffer);
        }
    }

    /* Generate percussion samples */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        organya_internal_sound_generate_sample(&context->percussion_index[i].sound, buffer);
    }

    /* Apply context volume */
    buffer[0] *= context->volume;
    buffer[1] *= context->volume;

    return ORG_RESULT_SUCCESS;
}

/* API functions */

ORG_API organya_result organya_context_init(organya_context *context)
{
    organya_result result;
    size_t i;

    if (context == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    /* Create song */
    result = organya_song_init(&context->song);
    if (result != ORG_RESULT_SUCCESS)
    {
        return result;
    }

    context->position = 0;
    context->last_position = 0;
    context->samples_to_next_tick = 0.0;

    context->sample_rate = 44100;
    /* Lagrange interpolation is the closest to original Organya playback on Windows Vista and later. */
    context->interpolation = ORG_INTERPOLATION_LAGRANGE;
    context->volume = 1;

    /* Should be 4ms */
    context->volume_ramp = (org_uint32)(context->sample_rate * 0.004F);

    /* Initialize melody channel status */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        context->melody_index[i].pitch = ORG_PROPERTY_NOT_USED;
        context->melody_index[i].volume = ORG_DEFAULT_VOLUME;
        context->melody_index[i].pan = ORG_DEFAULT_PAN;

        context->melody_index[i].index = 0;
        context->melody_index[i].ticks = 0;
        context->melody_index[i].alt = 0;
        context->melody_index[i].muted = ORG_FALSE;
    }

    /* Initialize percussion channel status */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        context->percussion_index[i].pitch = ORG_PROPERTY_NOT_USED;
        context->percussion_index[i].volume = ORG_DEFAULT_VOLUME;
        context->percussion_index[i].pan = ORG_DEFAULT_PAN;

        context->percussion_index[i].index = 0;
        context->percussion_index[i].muted = ORG_FALSE;
    }

    memset(context->melody_wave_data, 0, sizeof(context->melody_wave_data));
    memset(context->percussion_wave_data, 0, sizeof(context->percussion_wave_data));

    return ORG_RESULT_SUCCESS;
}

ORG_API void organya_context_deinit(organya_context *context)
{
    size_t i;

    if (context == NULL)
    {
        return;
    }

    /* Unload song from context and destroy it */
    organya_context_unload_song(context);
    organya_song_deinit(&context->song);

    /* Delete percussion samples */
    for (i = 0; i < ORG_PERCUSSION_COUNT; ++i)
    {
        if (context->percussion_wave_data[i].data != NULL)
        {
            ORG_FREE(context->percussion_wave_data[i].data);
        }
    }
}

ORG_API organya_result organya_context_read_soundbank(organya_context *context, const org_uint8 *bank_data, size_t data_length)
{
    size_t i;
    size_t offset;

    if (bank_data == NULL || data_length < ORG_MIN_SOUNDBANK_SIZE)
    {
        return ORG_RESULT_FILE_ERROR;
    }

    /* Copy wavetable data */
    memcpy(context->melody_wave_data, bank_data, ORG_WAVETABLE_COUNT * 0x100);

    /* Read percussion samples */
    offset = ORG_WAVETABLE_COUNT * 0x100;

    for (i = 0; i < ORG_PERCUSSION_COUNT; ++i)
    {
        /* Free old data */
        if (context->percussion_wave_data[i].data != NULL)
        {
            ORG_FREE(context->percussion_wave_data[i].data);
            context->percussion_wave_data[i].data = NULL;
        }

        if (offset + 4 > data_length)
        {
            return ORG_RESULT_FILE_ERROR;
        }

        context->percussion_wave_data[i].length = ORG_READ_32_LE(&bank_data[offset]);
        offset += 4;

        if (context->percussion_wave_data[i].length == 0)
        {
            /* Invalid sample */
            continue;
        }

        if (offset + context->percussion_wave_data[i].length > data_length)
        {
            return ORG_RESULT_FILE_ERROR;
        }

        context->percussion_wave_data[i].data = (org_uint8 *)ORG_MALLOC(context->percussion_wave_data[i].length);
        if (context->percussion_wave_data[i].data == NULL)
        {
            return ORG_RESULT_MEMORY_ERROR;
        }

        memcpy(context->percussion_wave_data[i].data, &bank_data[offset], context->percussion_wave_data[i].length);
        offset += context->percussion_wave_data[i].length;
    }

    return ORG_RESULT_SUCCESS;
}

#ifndef ORG_NO_STDIO

ORG_API organya_result organya_context_load_soundbank_file(organya_context *context, const char *file_path)
{
    organya_result result;
    size_t file_size;
    org_uint8 *buffer;
    FILE *file;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    errno_t err;

    /* Open file */
    err = fopen_s(&file, file_path, "rb");
    if (err != 0)
    {
        return ORG_RESULT_FILE_ERROR;
    }
#elif !defined(_WIN32) && !defined(__APPLE__) && defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64 && defined(_LARGEFILE64_SOURCE)
    file = fopen64(file_path, "rb");
#else
    file = fopen(file_path, "rb");
#endif

    if (file == NULL)
    {
        return ORG_RESULT_FILE_ERROR;
    }

    /* Seek to end */
    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Get file size */
    file_size = ftell(file);

    if (file_size == 0)
    {
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Return to start */
    rewind(file);

    /* Create buffer to store file contents in */
    buffer = (org_uint8 *)ORG_MALLOC(file_size);

    if (buffer == NULL)
    {
        fclose(file);
        return ORG_RESULT_MEMORY_ERROR;
    }

    /* Load file to buffer */
    if (fread(buffer, 1, file_size, file) < file_size)
    {
        ORG_FREE(buffer);
        fclose(file);
        return ORG_RESULT_FILE_ERROR;
    }

    /* Close file */
    fclose(file);

    /* Read file data */
    result = organya_context_read_soundbank(context, buffer, file_size);
    ORG_FREE(buffer);
    return result;
}

#endif

ORG_API void organya_context_set_sample_rate(organya_context *context, org_uint32 sample_rate)
{
    if (context == NULL)
    {
        return;
    }

    context->sample_rate = sample_rate;

    /* Update volume ramp (should be 4ms) */
    context->volume_ramp = (org_uint32)(context->sample_rate * 0.004F);

    /* Update all sounds data */
    organya_internal_context_set_sound_settings(context);
}

ORG_API void organya_context_set_volume(organya_context *context, float volume)
{
    if (context == NULL)
    {
        return;
    }

    context->volume = volume;
}

ORG_API void organya_context_set_interpolation(organya_context *context, organya_interpolation interpolation)
{
    if (context == NULL)
    {
        return;
    }

    context->interpolation = interpolation;

    /* Update all sounds data */
    organya_internal_context_set_sound_settings(context);
}

ORG_API organya_result organya_context_read_song(organya_context *context, const org_uint8 *song_data, size_t data_length)
{
    organya_result result;

    if (context == NULL || song_data == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    result = organya_song_read(&context->song, song_data, data_length);
    if (result != ORG_RESULT_SUCCESS)
    {
        return result;
    }

    /* Seek to start */
    organya_context_seek(context, 0);

    /* Create and load instrument data */
    return organya_internal_context_load_instruments(context);
}

#ifndef ORG_NO_STDIO

ORG_API organya_result organya_context_load_song_file(organya_context *context, const char *file_path)
{
    organya_result result;

    if (context == NULL || file_path == NULL)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    result = organya_song_load_file(&context->song, file_path);
    if (result != ORG_RESULT_SUCCESS)
    {
        return result;
    }

    /* Seek to start */
    organya_context_seek(context, 0);

    /* Create and load instrument data */
    return organya_internal_context_load_instruments(context);
}

#endif

ORG_API void organya_context_unload_song(organya_context *context)
{
    size_t i, j;

    if (context == NULL)
    {
        return;
    }

    /* Destroy all melody sounds */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            organya_internal_sound_deinit(&context->melody_index[i].sounds[j][0]);
            organya_internal_sound_deinit(&context->melody_index[i].sounds[j][1]);
        }

        context->melody_index[i].pitch = ORG_PROPERTY_NOT_USED;
    }

    /* Destroy all percussion sounds */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        organya_internal_sound_deinit(&context->percussion_index[i].sound);

        context->percussion_index[i].pitch = ORG_PROPERTY_NOT_USED;
    }

    /* Clear song info */
    organya_song_clean(&context->song);
}

ORG_API void organya_context_seek(organya_context *context, org_uint32 position)
{
    size_t i, j;
    organya_event *event;

    if (context == NULL)
    {
        return;
    }

    context->last_position = position;
    context->position = position;

    /* Get melody indexes */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        context->melody_index[i].index = 0;

        for (j = 0; j < context->song.channels[i].event_count; ++j)
        {
            event = &context->song.channels[i].event_list[j];

            if (event != NULL && context->position <= event->position)
            {
                context->melody_index[i].index = j;
                break;
            }
        }
    }

    /* Get percussion indexes */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        context->percussion_index[i].index = 0;

        for (j = 0; j < context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].event_count; ++j)
        {
            event = &context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].event_list[j];

            if (event != NULL && context->position <= event->position)
            {
                context->percussion_index[i].index = j;
                break;
            }
        }
    }
}

ORG_API void organya_context_set_mute(organya_context *context, size_t channel, org_bool mute)
{
    if (context == NULL || channel >= ORG_CHANNEL_COUNT)
    {
        return;
    }

    if (channel < ORG_MELODY_CHANNEL_COUNT)
    {
        context->melody_index[channel].muted = mute;
    }
    else
    {
        context->percussion_index[channel - ORG_MELODY_CHANNEL_COUNT].muted = mute;
    }
}

ORG_API void organya_context_tick(organya_context *context)
{
    size_t i, j, k;
    org_uint32 lp;
    organya_event *event;

    if (context == NULL)
    {
        return;
    }

    /* Tick melody channels */
    for (i = 0; i < ORG_MELODY_CHANNEL_COUNT; ++i)
    {
        /* Is there another event to handle? */
        if (context->melody_index[i].index < context->song.channels[i].event_count && !context->melody_index[i].muted)
        {
            event = &context->song.channels[i].event_list[context->melody_index[i].index];

            if (context->position == event->position)
            {
                if (event->pitch != ORG_PROPERTY_NOT_USED)
                {
                    /* Stop old sound */
                    if (context->melody_index[i].pitch != ORG_PROPERTY_NOT_USED)
                    {
                        if (!context->song.channels[i].pizzicato)
                        {
                            organya_internal_sound_play(
                                &context->melody_index[i].sounds[context->melody_index[i].pitch / 12][context->melody_index[i].alt],
                                ORG_FALSE
                            );
                        }

                        context->melody_index[i].alt ^= 1;
                    }

                    /* Update channel status */
                    context->melody_index[i].pitch = event->pitch;
                    context->melody_index[i].ticks = event->length;

                    /* Set frequency */
                    for (j = 0; j < 8; ++j)
                    {
                        for (k = 0; k < 2; ++k)
                        {
                            organya_internal_sound_set_frequency(
                                &context->melody_index[i].sounds[j][k],
                                (organya_wave_size_table[j]
                                        * organya_frequency_table[context->melody_index[i].pitch % 12]
                                        * (1 << j))
                                        / 8
                                        + (context->song.channels[i].finetune - 1000)
                            );
                        }
                    }

                    /* Play sound */
                    organya_internal_sound_play(
                        &context->melody_index[i].sounds[context->melody_index[i].pitch / 12][context->melody_index[i].alt],
                        !context->song.channels[i].pizzicato
                    );
                }

                if (event->volume != ORG_PROPERTY_NOT_USED)
                {
                    /* Update channel status */
                    context->melody_index[i].volume = event->volume;

                    /* Set playing volume */
                    if (context->melody_index[i].pitch != ORG_PROPERTY_NOT_USED)
                    {
                        organya_internal_sound_set_volume(
                            &context->melody_index[i].sounds[context->melody_index[i].pitch / 12][context->melody_index[i].alt],
                            ((context->melody_index[i].volume * 100 / 0x7F) - 0xFF) * 8
                        );
                    }
                }

                if (event->pan != ORG_PROPERTY_NOT_USED)
                {
                    /* Update channel status */
                    context->melody_index[i].pan = event->pan;

                    /* Set playing pan */
                    if (context->melody_index[i].pitch != ORG_PROPERTY_NOT_USED)
                    {
                        organya_internal_sound_set_pan(
                            &context->melody_index[i].sounds[context->melody_index[i].pitch / 12][context->melody_index[i].alt],
                            (organya_panning_table[context->melody_index[i].pan] - 0x100) * 10
                        );
                    }
                }

                ++context->melody_index[i].index;
            }
        }

        /* Tick note length */
        if (context->melody_index[i].ticks == 0)
        {
            if (context->melody_index[i].pitch != ORG_PROPERTY_NOT_USED && !context->song.channels[i].pizzicato)
            {
                /* Stop old sound */
                organya_internal_sound_play(
                    &context->melody_index[i].sounds[context->melody_index[i].pitch / 12][context->melody_index[i].alt],
                    ORG_FALSE
                );

                context->melody_index[i].pitch = ORG_PROPERTY_NOT_USED;
            }
        }
        else
        {
            --context->melody_index[i].ticks;
        }
    }

    /* Tick percussion channels */
    for (i = 0; i < ORG_PERCUSSION_CHANNEL_COUNT; ++i)
    {
        /* Is there another event to handle? */
        if (context->percussion_index[i].index < context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].event_count && !context->percussion_index[i].muted)
        {
            event = &context->song.channels[ORG_MELODY_CHANNEL_COUNT + i].event_list[context->percussion_index[i].index];

            if (context->position == event->position)
            {
                if (event->pitch != ORG_PROPERTY_NOT_USED)
                {
                    /* Stop old sound */
                    organya_internal_sound_stop(&context->percussion_index[i].sound);

                    /* Update channel status */
                    context->percussion_index[i].pitch = event->pitch;

                    /* Set frequency */
                    organya_internal_sound_set_frequency(
                        &context->percussion_index[i].sound,
                        (context->percussion_index[i].pitch * 800) + 100
                    );

                    /* Play new sound */
                    organya_internal_sound_play(&context->percussion_index[i].sound, ORG_FALSE);
                }

                if (event->volume != ORG_PROPERTY_NOT_USED)
                {
                    /* Update channel status */
                    context->percussion_index[i].volume = event->volume;

                    /* Set playing volume */
                    organya_internal_sound_set_volume(
                        &context->percussion_index[i].sound,
                        ((context->percussion_index[i].volume * 100 / 0x7F) - 0xFF) * 8
                    );
                }

                if (event->pan != ORG_PROPERTY_NOT_USED)
                {
                    /* Update channel status */
                    context->percussion_index[i].pan = event->pan;

                    /* Set playing pan */
                    organya_internal_sound_set_pan(
                        &context->percussion_index[i].sound,
                        (organya_panning_table[context->percussion_index[i].pan] - 0x100) * 10
                    );
                }

                ++context->percussion_index[i].index;
            }
        }
    }

    context->last_position = context->position;
    ++context->position;

    /* Check for repeat */
    if (context->position >= context->song.repeat_end)
    {
        lp = context->last_position;
        organya_context_seek(context, context->song.repeat_start);
        context->last_position = lp;
    }

    context->samples_to_next_tick += (double)context->sample_rate * (double)context->song.tempo_ms / 1000.0;
}

ORG_API size_t organya_context_generate_samples(organya_context *context, float *output, size_t sample_count)
{
    size_t i;
    float *p;

    if (context == NULL || output == NULL)
    {
        return 0;
    }

    p = output;

    /* Generate samples */
    for (i = 0; i < sample_count; ++i)
    {
        if (organya_internal_context_generate_sample(context, p) != ORG_RESULT_SUCCESS)
        {
            /* Failed */
            return i;
        }

        p += 2;
    }

    return sample_count;
}

ORG_PRIVATE organya_result organya_internal_sound_init(organya_internal_sound *sound, organya_context *context, size_t sample_count)
{
    if (sound == NULL || context == NULL || sample_count < 1)
    {
        return ORG_RESULT_INVALID_ARGS;
    }

    sound->sample_count = sample_count;

    /* Allocate sample buffer */
    sound->data = (org_int8 *)ORG_MALLOC(sound->sample_count);

    if (sound->data == NULL)
    {
        return ORG_RESULT_MEMORY_ERROR;
    }

    /* Initialize sample buffer */
    memset(sound->data, 0, sound->sample_count);
    memset(sound->samples, 0, ORG_MAX_TAPS * sizeof(float));

    sound->position = 0;
    sound->sub_position = 0;
    sound->silence_timer = 0;

    sound->total_samples = 0;

    sound->frequency = 22050;

    sound->volume = 1.0F;
    sound->pan_left = 1.0F;
    sound->pan_right = 1.0F;

    organya_internal_sound_set_frequency(sound, 22050);
    organya_internal_sound_set_volume(sound, 0);
    organya_internal_sound_set_pan(sound, 0);

    sound->volume_left = sound->target_volume_left;
    sound->volume_right = sound->target_volume_right;

    sound->playing = ORG_FALSE;
    sound->looping = ORG_FALSE;

    sound->volume_ticks = 0;
    sound->ring = 0;

    sound->out_sample_rate = context->sample_rate;
    sound->out_volume_ramp = context->volume_ramp;
    sound->interpolation = context->interpolation;

    return ORG_RESULT_SUCCESS;
}

ORG_PRIVATE void organya_internal_sound_deinit(organya_internal_sound *sound)
{
    if (sound == NULL)
    {
        return;
    }

    if (sound->data != NULL)
    {
        ORG_FREE(sound->data);
    }
}

ORG_PRIVATE void organya_internal_sound_set_frequency(organya_internal_sound *sound, org_uint32 frequency)
{
    if (sound == NULL)
    {
        return;
    }

    sound->frequency = frequency;
    sound->position_increment = (float)sound->frequency / sound->out_sample_rate;
}

ORG_PRIVATE void organya_internal_sound_set_volume(organya_internal_sound *sound, org_int16 volume_db)
{
    if (sound == NULL)
    {
        return;
    }

    if (volume_db < -10000)
    {
        volume_db = -10000;
    }
    else if (volume_db > 0)
    {
        volume_db = 0;
    }

    sound->volume = (float)pow(10.0F, volume_db / 2000.0F);

    sound->target_volume_left = sound->volume * sound->pan_left;
    sound->target_volume_right = sound->volume * sound->pan_right;

    if (sound->total_samples == 0)
    {
        sound->volume_left = sound->target_volume_left;
        sound->volume_right = sound->target_volume_right;
        sound->volume_ticks = 0;
    }
    else
    {
        sound->volume_ticks = sound->out_volume_ramp;
    }
}

ORG_PRIVATE void organya_internal_sound_set_pan(organya_internal_sound *sound, org_int16 pan_db)
{
    if (sound == NULL)
    {
        return;
    }

    if (pan_db < 0)
    {
        if (pan_db < -10000)
        {
            pan_db = -10000;
        }

        sound->pan_left = 1.0F;
        sound->pan_right = (float)pow(10.0F, pan_db / 2000.0F);
    }
    else
    {
        pan_db *= -1;
        if (pan_db < -10000)
        {
            pan_db = -10000;
        }

        sound->pan_left = (float)pow(10.0F, pan_db / 2000.0F);
        sound->pan_right = 1.0F;
    }

    sound->target_volume_left = sound->volume * sound->pan_left;
    sound->target_volume_right = sound->volume * sound->pan_right;

    if (sound->total_samples == 0)
    {
        sound->volume_left = sound->target_volume_left;
        sound->volume_right = sound->target_volume_right;
        sound->volume_ticks = 0;
    }
    else
    {
        sound->volume_ticks = sound->out_volume_ramp;
    }
}

ORG_PRIVATE void organya_internal_sound_play(organya_internal_sound *sound, org_bool loop)
{
    if (sound == NULL || sound->data == NULL)
    {
        return;
    }

    if (!sound->playing)
    {
        sound->position = 0;

        if (sound->silence_timer == 0)
        {
            sound->sub_position = 0;
        }
    }

    sound->playing = ORG_TRUE;
    sound->looping = loop;
}

ORG_PRIVATE void organya_internal_sound_stop(organya_internal_sound *sound)
{
    if (sound == NULL)
    {
        return;
    }

    sound->playing = ORG_FALSE;
    sound->silence_timer = ORG_MAX_TAPS;
}

ORG_PRIVATE void organya_internal_sound_generate_sample(organya_internal_sound *sound, float *output)
{
    org_uint32 i;
    org_int32 margin;
    org_uint32 last_position;
    float *p;
    float sample_mixed;

    if (sound == NULL || output == NULL)
    {
        return;
    }

    if (sound->playing || sound->silence_timer > 0)
    {
        p = output;

    #ifdef ORG_NO_VOLUME_SLIDE
        sound->volume_left = sound->target_volume_left;
        sound->volume_right = sound->target_volume_right;
    #else
        if (sound->volume_ticks > 0)
        {
            sound->volume_left += (sound->target_volume_left - sound->volume_left) / (float)sound->volume_ticks;
            sound->volume_right += (sound->target_volume_right - sound->volume_right) / (float)sound->volume_ticks;

            --sound->volume_ticks;
        }
    #endif

        /* Interpolate sample with chosen algorithm */
        switch (sound->interpolation)
        {
            case ORG_INTERPOLATION_NONE:
            {
                margin = sound->ring;

                sample_mixed = sound->samples[margin];
                break;
            }
            case ORG_INTERPOLATION_LINEAR:
            {
                float sample_a;
                float sample_b;

                margin = sound->ring - 1;

                sample_a = sound->samples[ORG_MOD_CLAMP(margin,     ORG_MAX_TAPS, 0)];
                sample_b = sound->samples[ORG_MOD_CLAMP(margin + 1, ORG_MAX_TAPS, 0)];

                sample_mixed = sample_a + (sample_b - sample_a) * sound->sub_position;
                break;
            }
            case ORG_INTERPOLATION_LAGRANGE:
            {
                float sample_a, sample_b, sample_c, sample_d;
                float c0, c1, c2, c3;

                margin = sound->ring - 2;

                sample_a = sound->samples[ORG_MOD_CLAMP(margin - 1, ORG_MAX_TAPS, 0)];
                sample_b = sound->samples[ORG_MOD_CLAMP(margin,     ORG_MAX_TAPS, 0)];
                sample_c = sound->samples[ORG_MOD_CLAMP(margin + 1, ORG_MAX_TAPS, 0)];
                sample_d = sound->samples[ORG_MOD_CLAMP(margin + 2, ORG_MAX_TAPS, 0)];

                c0 = sample_b;
                c1 = sample_c - 1 / 3.0F * sample_a - 1 / 2.0F * sample_b - 1 / 6.0F * sample_d;
                c2 = 1 / 2.0F * (sample_a + sample_c) - sample_b;
                c3 = 1 / 6.0F * (sample_d - sample_a) + 1 / 2.0F * (sample_b - sample_c);

                sample_mixed = ((c3 * sound->sub_position + c2) * sound->sub_position + c1) * sound->sub_position + c0;
                break;
            }
            case ORG_INTERPOLATION_LANCZOS:
            {
                org_int32 j;
                float sample_in;
                float t;
                float sample_out;

                margin = sound->ring - ORG_LANCZOS_WINDOW;
                sample_out = 0;

                for (j = -ORG_LANCZOS_WINDOW + 1; j <= ORG_LANCZOS_WINDOW; ++j)
                {
                    sample_in = sound->samples[ORG_MOD_CLAMP(margin + j, ORG_MAX_TAPS, 0)];

                    t = ORG_PI * (sound->sub_position - j);
                    sample_out += sample_in * (t == 0.0F ? 1.0F : (ORG_SINC(t) * ORG_SINC(t / ORG_LANCZOS_WINDOW)));
                }

                sample_mixed = sample_out;
                break;
            }
            default:
            {
                /* ??? */
                sample_mixed = 0;
                break;
            }
        }

        /* Write sample */
        *p++ += (sample_mixed * sound->volume_left);
        *p++ += (sample_mixed * sound->volume_right);

        /* Increment position */
        last_position = sound->position;

        sound->sub_position += sound->position_increment;
        sound->position += (org_uint32)sound->sub_position;
        sound->sub_position = (float)fmod(sound->sub_position, 1.0F);

        if (sound->position > last_position)
        {
            /* Update ring buffer position and write new sample(s) */
            for (i = 0; i < sound->position - last_position; ++i)
            {
                sound->ring = (sound->ring + 1) % (ORG_MAX_TAPS);

                if (sound->playing)
                {
                    if (sound->looping)
                    {
                        sound->samples[sound->ring] = (float)sound->data[(last_position + i) % sound->sample_count] / (float)(1 << 7);
                    }
                    else
                    {
                        sound->samples[sound->ring] = ((last_position + i) >= sound->sample_count ? 0 : (float)sound->data[(last_position + i)] / (float)(1 << 7));
                    }
                }
                else
                {
                    sound->samples[sound->ring] = 0;
                    --sound->silence_timer;
                }
            }
        }

        ++sound->total_samples;

        if (sound->playing)
        {
            /* Check if sound reached the end */
            if (sound->position >= sound->sample_count)
            {
                if (sound->looping)
                {
                    /* Make sure position is in range */
                    sound->position %= sound->sample_count;
                }
                else
                {
                    /* Stop playing */
                    sound->playing = ORG_FALSE;
                    sound->silence_timer = ORG_MAX_TAPS;
                }
            }
        }
        else
        {
            sound->position = 0;
        }
    }
}

#endif /* ORGANYA_IMPLEMENTATION */

/*
Copyright (c) 2025 Strultz

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/
