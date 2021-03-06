#ifndef MIDI_FILE_STATS_H
#define MIDI_FILE_STATS_H
#include "midifile/include/MidiFile.h"
#include "midifile/include/Options.h"
#include "portmidi/include/portmidi.h"
#include "portmidi/include/porttime.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define DEBUG

#ifdef DEBUG
#define DEBUG_MSG(__msg__) \
    fprintf(stderr, "%d:%s:%s:DEBUG:%s\n", \
            __LINE__, __FILE__, __func__,\
            (__msg__)\
          );
#else
#define DEBUG_MSG(__msg__)
#endif

#define ASSERT_MSG(__check__, __msg__) \
    if (!(__check__)) {\
        fprintf(stderr, "%d:%s:%s:ERROR:%d:%s:%s\n", \
                __LINE__, __FILE__, __func__,\
                errno, errno ? strerror(errno) : "errno not useful",\
                (__msg__)\
              ); \
        exit(errno ? errno : EXIT_FAILURE);\
    }
#define ASSERT(__check__) ASSERT_MSG((__check__), "no message provided")

#endif /* MIDI_FILE_STATS_H */
