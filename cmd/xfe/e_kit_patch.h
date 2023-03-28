/**********************************************************************
 e_kit_patch.h
 By Daniel Malmer
 March 11, 1996

 This file is used to allow customers to patch the Navigator binary
 in the field.
 The unique signature should only appear once in the Navigator.  This
 can be verified at Q/A time, but since it's 32 bytes long, that should
 be sufficiently unique.
 It works like this:  e-kit is shipped with a patcher that recognizes
 the signature below.  Once it finds it, it knows that it's dealing with
 the structure that holds the 'whacked' field, and can set it to '1'.

 The SIGNATURE string should change any time there is a change made
 to the data structure that is being touched.  This way, lockers can't
 patch Navigators that have different data structures.
**********************************************************************/

#include <limits.h>

/*
 * HP-UX only defines this if XPG2 is defined.
 * Until we figure out what the right thing to
 * do is, 1024 looks like it will do.
 * As long as the locker program and Navigator
 * both use the same value, it should be fine.
 */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define SIGNATURE "\177\222\223\235\112\223\235\112"\
                  "\213\230\112\217\127\225\223\236"\
                  "\112\237\230\223\233\237\217\112"\
                  "\235\236\234\223\230\221\113\0"

#if defined(__sun)
#define DEFAULT_ROOT "/usr/openwin/lib"
#else
#define DEFAULT_ROOT "/usr/lib/X11"
#endif

struct patch_struct {
  unsigned char signature[32];
  char version[8];
  char root[PATH_MAX+1];
  unsigned char whacked;
} ekit_patch = {SIGNATURE, "", DEFAULT_ROOT, '\0'};
 
#define ekit_enabled()  (ekit_patch.whacked & 0x01)
#define ekit_required() (ekit_patch.whacked & 0x02)
#define ekit_version()  ekit_patch.version
#define ekit_root()     ekit_patch.root
