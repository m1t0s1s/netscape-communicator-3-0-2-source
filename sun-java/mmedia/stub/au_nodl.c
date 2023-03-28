/* Automatically generated file; do not edit */

#if defined(AIX) || defined(BSDI) || defined(HPUX) || defined(LINUX) || defined(XP_MAC) || defined(SCO) || defined(UNIXWARE)
#include "prtypes.h"
#include "prlink.h"

extern void Java_sun_audio_AudioDevice_audioOpen_stub();
extern void Java_sun_audio_AudioDevice_audioClose_stub();
extern void Java_sun_audio_AudioDevice_audioWrite_stub();

PRStaticLinkTable au_nodl_tab[] = {
  { "Java_sun_audio_AudioDevice_audioOpen_stub", Java_sun_audio_AudioDevice_audioOpen_stub },
  { "Java_sun_audio_AudioDevice_audioClose_stub", Java_sun_audio_AudioDevice_audioClose_stub },
  { "Java_sun_audio_AudioDevice_audioWrite_stub", Java_sun_audio_AudioDevice_audioWrite_stub },
  { 0, 0, },
};
#endif
