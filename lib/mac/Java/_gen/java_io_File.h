/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class java_io_File */

#ifndef _Included_java_io_File
#define _Included_java_io_File
struct Hjava_lang_String;

typedef struct Classjava_io_File {
    struct Hjava_lang_String *path;
/* Inaccessible static: separator */
/* Inaccessible static: separatorChar */
/* Inaccessible static: pathSeparator */
/* Inaccessible static: pathSeparatorChar */
} Classjava_io_File;
HandleTo(java_io_File);

extern /*boolean*/ long java_io_File_exists0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_canWrite0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_canRead0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_isFile0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_isDirectory0(struct Hjava_io_File* self);
extern int64_t java_io_File_lastModified0(struct Hjava_io_File* self);
extern int64_t java_io_File_length0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_mkdir0(struct Hjava_io_File* self);
struct Hjava_io_File;
extern /*boolean*/ long java_io_File_renameTo0(struct Hjava_io_File* self,struct Hjava_io_File *);
extern /*boolean*/ long java_io_File_delete0(struct Hjava_io_File* self);
extern HArrayOfString *java_io_File_list0(struct Hjava_io_File* self);
extern /*boolean*/ long java_io_File_isAbsolute(struct Hjava_io_File* self);
#endif