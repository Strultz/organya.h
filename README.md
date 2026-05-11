# organya.h

## Description

organya.h is a simple C89 library for reading and decoding Organya music (.org files).

Organya is a sequenced music format created in 1999 by [Studio Pixel](https://studiopixel.jp/).
It is the predecessor to [PxTone](https://en.wikipedia.org/wiki/PxTone) and was used in games such as
[Cave Story](https://www.cavestory.org/game-info/about-cave-story.php),
[Azarashi (2001 version)](https://www.cavestory.org/pixels-works/azarashi.php),
[STARGAZER](http://www5b.biglobe.ne.jp/~kiss-me/aji/star/), and more.

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
    /* Create the context: */
    organya_context ctx;
    if (organya_context_init(&ctx) != ORG_RESULT_SUCCESS)
    {
        /* Handle the error here */
        return -1;
    }

    /* Set everything up (these are the default settings): */
    organya_context_set_sample_rate(&ctx, 44100);
    organya_context_set_volume(&ctx, 1);
    organya_context_set_interpolation(&ctx, ORG_INTERPOLATION_CUBIC);
    organya_context_set_output_format(&ctx, ORG_OUTPUT_FORMAT_F32);
    /* Note: Using Cubic interpolation produces output that sounds (almost)
     * identical to original Organya playback (on Windows Vista and later) */

    /* Load a soundbank from a file path: */
    if (organya_context_load_soundbank_file(&ctx, "path/to/file.wdb") != ORG_RESULT_SUCCESS)
    {
        /* Handle the error here */
        organya_context_deinit(&ctx);
        return -1;
    }
    /* Then load a song from a file path: */
    if (organya_context_load_song_file(&ctx, "path/to/file.org") != ORG_RESULT_SUCCESS)
    {
        /* Handle the error here */
        organya_context_deinit(&ctx);
        return -1;
    }
    /* Alternatively, you can load both of these directly from some pointer
     * using organya_context_read_soundbank() & organya_context_read_song(). */

    /* Generate samples (which would then be output to an audio player, or a
     * .wav file, or etc.). This will output interleaved stereo PCM samples in
     * the format set with organya_context_set_output_format to output_buffer.
     * output_buffer should be at least num_samples * sizeof(output_format) * 2
     * long. */
    organya_context_generate_samples(&ctx, output_buffer, num_samples);

    /* When you're done, deinitialize and free everything: */
    organya_context_deinit(&ctx);

    return 0;
}
```
