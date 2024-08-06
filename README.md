# organya.h

## Description

organya.h is a simple single-header Organya decoding library written in C89.

Organya is a sequenced music format created in 1999 by [Studio Pixel](https://studiopixel.jp/) and used in games such as
[Cave Story](https://www.cavestory.org/game-info/about-cave-story.php), [Azarashi (2001 version)](https://www.cavestory.org/pixels-works/azarashi.php), [STARGAZER](http://www5b.biglobe.ne.jp/~kiss-me/aji/star/), and more.

## Usage

To use this library, just `#include "organya.h"` in your project.

In one .c file, you should define `ORGANYA_IMPLEMENTATION` before including the header, in order
to create the implementation.

## Basic Example

Create a context:

    organya_context *ctx = organya_context_create();
    if (ctx == NULL) {
        Error
    }

Load a file:

    if (organya_context_load_song_file(ctx, "path/to/file.org")) {
        Error
    }

Start playing:

    organya_context_play(ctx);

Generate samples:

    organya_context_generate_samples(ctx, output_buffer, num_samples);

- This will output interleaved stereo host-endian signed 16-bit PCM to output_buffer.

When you're done:

    organya_context_destroy(ctx);
