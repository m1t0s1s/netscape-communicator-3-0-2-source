#if defined(__STDC__) || defined(__cplusplus)
# define __stdcargs(s) s
#else
# define __stdcargs(s) ()
#endif

/* malloc.c */
DATATYPE *malloc __stdcargs((SIZETYPE size));
DATATYPE *debug_malloc __stdcargs((CONST char *file, int line, SIZETYPE size));
char *DBFmalloc __stdcargs((CONST char *func, int type, unsigned long call_counter, CONST char *file, int line, SIZETYPE size));
VOIDTYPE malloc_split __stdcargs((struct mlist *ptr));
VOIDTYPE malloc_join __stdcargs((struct mlist *ptr, struct mlist *nextptr, int inuse_override, int fill_flag));
VOIDTYPE malloc_fatal __stdcargs((CONST char *funcname, CONST char *file, int line, CONST struct mlist *mptr));
VOIDTYPE malloc_warning __stdcargs((CONST char *funcname, CONST char *file, int line, CONST struct mlist *mptr));
VOIDTYPE malloc_dump_info_block __stdcargs((CONST struct mlist *mptr, int id));
VOIDTYPE malloc_err_handler __stdcargs((int level));
CONST char *malloc_int_suffix __stdcargs((unsigned long i));
VOIDTYPE malloc_freeseg __stdcargs((int op, struct mlist *ptr));
CONST char *MallocFuncName __stdcargs((CONST struct mlist *mptr));
CONST char *FreeFuncName __stdcargs((CONST struct mlist *mptr));
void InitMlist __stdcargs((struct mlist *mptr, int type));
/* datamc.c */
void DataMC __stdcargs((MEMDATA *ptr1, CONST MEMDATA *ptr2, MEMSIZE len));
/* datams.c */
void DataMS __stdcargs((MEMDATA *ptr1, int ch, register MEMSIZE len));
/* dgmalloc.c */
DATATYPE *_malloc __stdcargs((SIZETYPE size));
DATATYPE *_realloc __stdcargs((DATATYPE *cptr, SIZETYPE size));
DATATYPE *_calloc __stdcargs((SIZETYPE nelem, SIZETYPE elsize));
void _free __stdcargs((DATATYPE *cptr));
int _mallopt __stdcargs((int cmd, union dbmalloptarg value));
MEMDATA *_bcopy __stdcargs((CONST MEMDATA *ptr2, MEMDATA *ptr1, MEMSIZE len));
MEMDATA *_bzero __stdcargs((MEMDATA *ptr1, MEMSIZE len));
int _bcmp __stdcargs((CONST MEMDATA *ptr2, CONST MEMDATA *ptr1, MEMSIZE len));
MEMDATA *__dg_bcopy __stdcargs((CONST MEMDATA *ptr2, MEMDATA *ptr1, MEMSIZE len));
MEMDATA *__dg_bzero __stdcargs((MEMDATA *ptr1, MEMSIZE len));
int __dg_bcmp __stdcargs((CONST MEMDATA *ptr2, CONST MEMDATA *ptr1, MEMSIZE len));
/* fill.c */
VOIDTYPE FillInit __stdcargs((void));
int FillCheck __stdcargs((CONST char *func, CONST char *file, int line, struct mlist *ptr, int showerrors));
int FillCheckData __stdcargs((CONST char *func, CONST char *file, int line, struct mlist *ptr, int checktype, int showerrors));
void FillData __stdcargs((register struct mlist *ptr, int alloctype, SIZETYPE start, struct mlist *nextptr));
/* free.c */
FREETYPE free __stdcargs((DATATYPE *cptr));
FREETYPE debug_free __stdcargs((CONST char *file, int line, DATATYPE *cptr));
FREETYPE DBFfree __stdcargs((CONST char *func, int type, unsigned long counter, CONST char *file, int line, DATATYPE *cptr));
/* realloc.c */
DATATYPE *realloc __stdcargs((DATATYPE *cptr, SIZETYPE size));
DATATYPE *debug_realloc __stdcargs((CONST char *file, int line, DATATYPE *cptr, SIZETYPE size));
DATATYPE *DBFrealloc __stdcargs((CONST char *func, int type, unsigned long call_counter, CONST char *file, int line, DATATYPE *cptr, SIZETYPE size));
/* calloc.c */
DATATYPE *calloc __stdcargs((SIZETYPE nelem, SIZETYPE elsize));
DATATYPE *debug_calloc __stdcargs((CONST char *file, int line, SIZETYPE nelem, SIZETYPE elsize));
char *DBFcalloc __stdcargs((CONST char *func, int type, unsigned long call_counter, CONST char *file, int line, SIZETYPE nelem, SIZETYPE elsize));
FREETYPE cfree __stdcargs((DATATYPE *cptr));
FREETYPE debug_cfree __stdcargs((CONST char *file, int line, DATATYPE *cptr));
/* string.c */
char *strcat __stdcargs((char *str1, CONST char *str2));
char *DBstrcat __stdcargs((CONST char *file, int line, register char *str1, register CONST char *str2));
char *strdup __stdcargs((CONST char *str1));
char *DBstrdup __stdcargs((CONST char *file, int line, register CONST char *str1));
char *strncat __stdcargs((char *str1, CONST char *str2, STRSIZE len));
char *DBstrncat __stdcargs((CONST char *file, int line, register char *str1, register CONST char *str2, register STRSIZE len));
int strcmp __stdcargs((register CONST char *str1, register CONST char *str2));
int DBstrcmp __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2));
int strncmp __stdcargs((register CONST char *str1, register CONST char *str2, register STRSIZE len));
int DBstrncmp __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2, register STRSIZE len));
int stricmp __stdcargs((register CONST char *str1, register CONST char *str2));
int DBstricmp __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2));
int strincmp __stdcargs((register CONST char *str1, register CONST char *str2, register STRSIZE len));
int DBstrincmp __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2, register STRSIZE len));
char *strcpy __stdcargs((register char *str1, register CONST char *str2));
char *DBstrcpy __stdcargs((CONST char *file, int line, register char *str1, register CONST char *str2));
char *strncpy __stdcargs((register char *str1, register CONST char *str2, register STRSIZE len));
char *DBstrncpy __stdcargs((CONST char *file, int line, register char *str1, register CONST char *str2, STRSIZE len));
STRSIZE strlen __stdcargs((CONST char *str1));
STRSIZE DBstrlen __stdcargs((CONST char *file, int line, register CONST char *str1));
char *strchr __stdcargs((CONST char *str1, int c));
char *DBstrchr __stdcargs((CONST char *file, int line, CONST char *str1, int c));
char *DBFstrchr __stdcargs((CONST char *func, CONST char *file, int line, register CONST char *str1, register int c));
char *strrchr __stdcargs((CONST char *str1, int c));
char *DBstrrchr __stdcargs((CONST char *file, int line, CONST char *str1, int c));
char *DBFstrrchr __stdcargs((CONST char *func, CONST char *file, int line, register CONST char *str1, register int c));
char *index __stdcargs((CONST char *str1, int c));
char *DBindex __stdcargs((CONST char *file, int line, CONST char *str1, int c));
char *rindex __stdcargs((CONST char *str1, int c));
char *DBrindex __stdcargs((CONST char *file, int line, CONST char *str1, int c));
char *strpbrk __stdcargs((CONST char *str1, CONST char *str2));
char *DBstrpbrk __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2));
STRSIZE strspn __stdcargs((CONST char *str1, CONST char *str2));
STRSIZE DBstrspn __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2));
STRSIZE strcspn __stdcargs((CONST char *str1, CONST char *str2));
STRSIZE DBstrcspn __stdcargs((CONST char *file, int line, register CONST char *str1, register CONST char *str2));
char *strstr __stdcargs((CONST char *str1, CONST char *str2));
char *DBstrstr __stdcargs((CONST char *file, int line, CONST char *str1, CONST char *str2));
char *strtok __stdcargs((char *str1, CONST char *str2));
char *DBstrtok __stdcargs((CONST char *file, int line, char *str1, CONST char *str2));
char *strtoken __stdcargs((register char **stringp, register CONST char *delim, int skip));
/* mcheck.c */
VOIDTYPE malloc_check_str __stdcargs((CONST char *func, CONST char *file, int line, CONST char *str));
VOIDTYPE malloc_check_strn __stdcargs((CONST char *func, CONST char *file, int line, CONST char *str, STRSIZE len));
VOIDTYPE malloc_check_data __stdcargs((CONST char *func, CONST char *file, int line, CONST DATATYPE *ptr, SIZETYPE len));
VOIDTYPE malloc_verify __stdcargs((CONST char *func, CONST char *file, int line, register CONST DATATYPE *ptr, register SIZETYPE len));
int GoodMlist __stdcargs((register CONST struct mlist *mptr));
/* mchain.c */
int malloc_chain_check __stdcargs((int todo));
int DBmalloc_chain_check __stdcargs((CONST char *file, int line, int todo));
int DBFmalloc_chain_check __stdcargs((CONST char *func, CONST char *file, int line, int todo));
/* memory.c */
MEMDATA *memccpy __stdcargs((MEMDATA *ptr1, CONST MEMDATA *ptr2, int ch, MEMSIZE len));
MEMDATA *DBmemccpy __stdcargs((CONST char *file, int line, MEMDATA *ptr1, CONST MEMDATA *ptr2, int ch, MEMSIZE len));
MEMDATA *memchr __stdcargs((CONST MEMDATA *ptr1, register int ch, MEMSIZE len));
MEMDATA *DBmemchr __stdcargs((CONST char *file, int line, CONST MEMDATA *ptr1, register int ch, MEMSIZE len));
MEMDATA *memmove __stdcargs((MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
MEMDATA *DBmemmove __stdcargs((CONST char *file, int line, MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
MEMDATA *memcpy __stdcargs((MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
MEMDATA *DBmemcpy __stdcargs((CONST char *file, int line, MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
MEMDATA *DBFmemcpy __stdcargs((CONST char *func, CONST char *file, int line, MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
int memcmp __stdcargs((CONST MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
int DBmemcmp __stdcargs((CONST char *file, int line, CONST MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
int DBFmemcmp __stdcargs((CONST char *func, CONST char *file, int line, CONST MEMDATA *ptr1, CONST MEMDATA *ptr2, register MEMSIZE len));
MEMDATA *memset __stdcargs((MEMDATA *ptr1, register int ch, register MEMSIZE len));
MEMDATA *DBmemset __stdcargs((CONST char *file, int line, MEMDATA *ptr1, register int ch, register MEMSIZE len));
MEMDATA *DBFmemset __stdcargs((CONST char *func, CONST char *file, int line, MEMDATA *ptr1, register int ch, register MEMSIZE len));
MEMDATA *bcopy __stdcargs((CONST MEMDATA *ptr2, MEMDATA *ptr1, MEMSIZE len));
MEMDATA *DBbcopy __stdcargs((CONST char *file, int line, CONST MEMDATA *ptr2, MEMDATA *ptr1, MEMSIZE len));
MEMDATA *bzero __stdcargs((MEMDATA *ptr1, MEMSIZE len));
MEMDATA *DBbzero __stdcargs((CONST char *file, int line, MEMDATA *ptr1, MEMSIZE len));
int bcmp __stdcargs((CONST MEMDATA *ptr2, CONST MEMDATA *ptr1, MEMSIZE len));
int DBbcmp __stdcargs((CONST char *file, int line, CONST MEMDATA *ptr2, CONST MEMDATA *ptr1, MEMSIZE len));
/* tostring.c */
int tostring __stdcargs((char *buf, unsigned long val, int len, int base, int fill));
/* m_perror.c */
VOIDTYPE malloc_perror __stdcargs((CONST char *str));
/* m_init.c */
VOIDTYPE malloc_init __stdcargs((void));
/* mallopt.c */
int dbmallopt __stdcargs((int cmd, union dbmalloptarg *value));
/* dump.c */
VOIDTYPE malloc_dump __stdcargs((int fd));
VOIDTYPE malloc_list __stdcargs((int fd, unsigned long histid1, unsigned long histid2));
VOIDTYPE malloc_list_items __stdcargs((int fd, int list_type, unsigned long histid1, unsigned long histid2));
/* stack.c */
void StackEnter __stdcargs((CONST char *func, CONST char *file, int line));
struct stack *StackNew __stdcargs((CONST char *func, CONST char *file, int line));
int StackMatch __stdcargs((struct stack *this, CONST char *func, CONST char *file, int line));
void StackLeave __stdcargs((CONST char *func, CONST char *file, int line));
struct stack *StackCurrent __stdcargs((void));
void StackDump __stdcargs((int fd, CONST char *msg, struct stack *node));
/* xmalloc.c */
void _XtAllocError __stdcargs((CONST char *type));
void _XtBCopy __stdcargs((char *b1, char *b2, int length));
void debug_XtBcopy __stdcargs((char *file, int line, char *b1, char *b2, int length));
char *XtMalloc __stdcargs((unsigned int size));
char *debug_XtMalloc __stdcargs((CONST char *file, int line, unsigned int size));
char *XtRealloc __stdcargs((char *ptr, unsigned int size));
char *debug_XtRealloc __stdcargs((CONST char *file, int line, char *ptr, unsigned int size));
char *XtCalloc __stdcargs((unsigned int num, unsigned int size));
char *debug_XtCalloc __stdcargs((CONST char *file, int line, unsigned int num, unsigned int size));
void XtFree __stdcargs((char *ptr));
void debug_XtFree __stdcargs((CONST char *file, int line, char *ptr));
/* xheap.c */
void _XtHeapInit __stdcargs((Heap *heap));
char *_XtHeapAlloc __stdcargs((Heap *heap, Cardinal bytes));
void _XtHeapFree __stdcargs((Heap *heap));
/* malign.c */
DATATYPE *memalign __stdcargs((SIZETYPE align, SIZETYPE size));
DATATYPE *DBmemalign __stdcargs((CONST char *file, int line, SIZETYPE align, SIZETYPE size));
int AlignedFit __stdcargs((struct mlist *mptr, SIZETYPE align, SIZETYPE size));
struct mlist *AlignedMakeSeg __stdcargs((struct mlist *mptr, SIZETYPE align));
SIZETYPE AlignedOffset __stdcargs((struct mlist *mptr, SIZETYPE align));
/* size.c */
SIZETYPE malloc_size __stdcargs((CONST DATATYPE *cptr));
SIZETYPE DBmalloc_size __stdcargs((CONST char *file, int line, CONST DATATYPE *cptr));
/* abort.c */
VOIDTYPE malloc_abort __stdcargs((void));
/* leak.c */
unsigned long malloc_inuse __stdcargs((unsigned long *histptr));
unsigned long DBmalloc_inuse __stdcargs((CONST char *file, int line, unsigned long *histptr));
VOIDTYPE malloc_mark __stdcargs((DATATYPE *cptr));
VOIDTYPE DBmalloc_mark __stdcargs((CONST char *file, int line, DATATYPE *cptr));

#undef __stdcargs
