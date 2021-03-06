# ########## Project setup ##########
PROJECT(wildmidi C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

# WildMIDI Version
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 4)
SET(VERSION_RELEASE 0)
SET(WILDMIDI_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}")

# Lib Versions
SET(SOVERSION 2)
SET(VERSION 2.0.0)

# Find Macros
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE(CMakeDependentOption)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckIncludeFile)
INCLUDE(TestBigEndian)

# Set a default build type if none was specified
IF(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  MESSAGE(STATUS "Setting build type to 'Debug' as none was specified.")
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()
SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Set our optoins
OPTION(WANT_PLAYER      "Build WildMIDI player in addition to the libraries" ON)
OPTION(WANT_STATIC      "Build static library in addition to dynamic library" OFF)
CMAKE_DEPENDENT_OPTION(WANT_PLAYERSTATIC "Build a statically linked WildMIDI player" ON "WANT_STATIC" OFF)
OPTION(WANT_ALSA        "Include ALSA (Advanced Linux Sound Architecture) support" OFF)
OPTION(WANT_OSS         "Include OSS (Open Sound System) support" OFF)
#OPTION(WANT_COREAUDIO   "Include CoreAudio support (Driver support for Mac OS X" OFF)
OPTION(WANT_OPENAL      "Include OpenAL suport (Cross Platform) support" OFF)
OPTION(WANT_DEVTEST     "Build WildMIDI DevTest file to check files" OFF)
IF(UNIX AND NOT APPLE)
SET(WILDMIDI_CFG "/etc/wildmidi/wildmidi.cfg" CACHE STRING "default config location")
ELSE()
SET(WILDMIDI_CFG "wildmidi.cfg" CACHE STRING "default config location")
ENDIF()

# Platform specific defines
IF(UNIX)
    # allow 'large' files in 32 bit builds
    ADD_DEFINITIONS(
        -D_LARGEFILE_SOURCE
        -D_FILE_OFFSET_BITS=64
        -D_LARGE_FILES
        )
ENDIF()

IF(WIN32)
    ADD_DEFINITIONS(
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        )
ENDIF()

# Compiler specific settings
IF(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    ADD_DEFINITIONS(
        -Wall -W
        -fno-common
        )

    IF(NOT WIN32 AND NOT CYGWIN)
        SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -Werror")
        CHECK_C_SOURCE_COMPILES("int foo(void) __attribute__((visibility(\"default\")));
                                 int main(void) {return 0;}" HAVE_VISIBILITY_DEFAULT)
        IF(HAVE_VISIBILITY_DEFAULT)
            CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
        ENDIF()
        SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        ADD_DEFINITIONS( -ggdb3 -O0 )
    ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Release")
        ADD_DEFINITIONS( -O3 )
    ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
ENDIF()

IF("${CMAKE_C_COMPILER_ID}" MATCHES "SunPro")
    SET(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS} -xldscope=hidden")
# __SUNPRO_C >= 0x590
#   CHECK_C_SOURCE_COMPILES("int foo(void) __attribute__((visibility(\"default\")));
#                            int main(void) {return 0;}" HAVE_VISIBILITY_DEFAULT)
# __SUNPRO_C >= 0x550
    CHECK_C_SOURCE_COMPILES("__global int foo(void);
                             int main(void) {return 0;}" HAVE_LDSCOPE_GLOBAL)
    IF(HAVE_LDSCOPE_GLOBAL)# OR HAVE_VISIBILITY_DEFAULT
        SET(HAVE_LDSCOPE_HIDDEN 1)
    ENDIF()
    SET(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
ENDIF()

CHECK_C_SOURCE_COMPILES("int main(void) {__builtin_expect(0,0); return 0;}" HAVE___BUILTIN_EXPECT)

CHECK_C_SOURCE_COMPILES("static inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C_INLINE)
CHECK_C_SOURCE_COMPILES("static __inline__ int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE__)
CHECK_C_SOURCE_COMPILES("static __inline int static_foo() {return 0;}
                         int main(void) {return 0;}" HAVE_C___INLINE)

CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

SET(AUDIODRV_ALSA)
SET(AUDIODRV_OSS)
SET(AUDIODRV_OPENAL)

# UNIX-like environments
IF(UNIX AND NOT APPLE)

    # Go looking for available sound packages for WildMIDI player
    IF(WANT_PLAYER OR WANT_PLAYERSTATIC)
        FIND_PACKAGE(ALSA)
        FIND_PACKAGE(OpenAL)
        FIND_PACKAGE(OSS)

        # Set preferred output
        IF(WANT_ALSA)
            IF(NOT ALSA_FOUND)
                MESSAGE(FATAL_ERROR "ALSA required but not found.")
            ENDIF()
            SET(AUDIODRV_ALSA 1)
            SET(AUDIO_LIBRARY ${ALSA_LIBRARY})

        ELSEIF(WANT_OSS)
            IF(NOT OSS_FOUND)
                MESSAGE(FATAL_ERROR "OSS required but not found.")
            ENDIF()
            # no special header paths
            SET(AUDIODRV_OSS 1)
            SET(AUDIO_LIBRARY ${OSS_LIBRARY})

        ELSEIF(WANT_OPENAL)
            IF(NOT OPENAL_FOUND)
                MESSAGE(FATAL_ERROR "OpenAL required but not found.")
            ENDIF()
            SET(AUDIODRV_OPENAL 1)
            SET(AUDIO_LIBRARY ${OPENAL_LIBRARY})

        ELSE() # Try to auto-detect

            IF(ALSA_FOUND)
                SET(AUDIO_LIBRARY ${ALSA_LIBRARY})
                SET(AUDIODRV_ALSA 1)

            ELSEIF(OSS_FOUND)
                # no special header paths
                SET(AUDIO_LIBRARY ${OSS_LIBRARY})
                SET(AUDIODRV_OSS 1)

            ELSEIF(OPENAL_FOUND)
                SET(AUDIO_LIBRARY ${OPENAL_LIBRARY})
                SET(AUDIODRV_OPENAL 1)

            ELSE()
                MESSAGE(WARNING "Could not find an audio sub-system!")
                SET(AUDIO_LIBRARY "")
            ENDIF()

        ENDIF()
    ENDIF()

    # find our math lib
    FIND_LIBRARY(M_LIBRARY m REQUIRED)

    ## Debian and non debian Linux building
    IF (DPKG_PROGRAM)
        ## Debian specific
    ELSE ()
        ## Non debian specific
    ENDIF (DPKG_PROGRAM)
ENDIF(UNIX AND NOT APPLE)

IF (APPLE AND (WANT_PLAYER OR WANT_PLAYERSTATIC))
    FIND_PACKAGE(OpenAL)

    IF(WANT_OPENAL)
        IF(NOT OPENAL_FOUND)
            MESSAGE(FATAL_ERROR "OpenAL required but not found.")
        ENDIF()
        SET(AUDIO_LIBRARY ${OPENAL_LIBRARY})

    ELSE()
        IF(OPENAL_FOUND)
            SET(AUDIO_LIBRARY ${OPENAL_LIBRARY})
            SET(AUDIODRV_OPENAL 1)

        ELSE()
            MESSAGE(WARNING "Could not find an audio sub-system!")
            SET(AUDIO_LIBRARY "")
        ENDIF()

    ENDIF()
ENDIF()

IF(WIN32)
    IF (WANT_PLAYER OR WANT_PLAYERSTATIC)
        LINK_LIBRARIES(winmm)
    ENDIF()
ENDIF(WIN32)

# ######### General setup ##########
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_SOURCE_DIR}/include")
FILE (MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")
INCLUDE_DIRECTORIES(BEFORE "${CMAKE_BINARY_DIR}/include")
IF(NOT HAVE_STDINT_H) # AND NOT HAVE_INTTYPES_H
   INCLUDE_DIRECTORIES(BEFORE "${CMAKE_SOURCE_DIR}/include/stdint")
ENDIF()

IF(APPLE)
    SET(APP_BUNDLE_NAME "${CMAKE_PROJECT_NAME}.app")
    SET(APP_BUNDLE_DIR "${wildmidi_BINARY_DIR}/${APP_BUNDLE_NAME}")
ENDIF(APPLE)

IF(APPLE)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APP_BUNDLE_DIR}/Contents/MacOS")
ELSE(APPLE)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${wildmidi_BINARY_DIR}")
ENDIF(APPLE)

# Setup up our config file
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/include/config.h.cmake" "${CMAKE_BINARY_DIR}/include/config.h")

ADD_SUBDIRECTORY(src)
