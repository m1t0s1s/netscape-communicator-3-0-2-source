
#ifndef MKSORT_H
#define MKSORT_H

typedef struct _SortStruct {
    int     cur_size;
    int     num_entries;
    void ** list;
} SortStruct;


extern SortStruct * NET_SortInit (void);
extern Bool NET_SortAdd (SortStruct * sort_struct, void * add_object);
extern void NET_DoSort(SortStruct * sort_struct, int (*compar) (const void *, const void *));
extern void * NET_SortUnloadNext(SortStruct * sort_struct);
extern void * NET_SortRetrieveNumber(SortStruct * sort_struct, int number);
extern int    NET_SortCount(SortStruct * sort_struct);
extern Bool NET_SortInsert(SortStruct * sort_struct, void * insert_before, void * new_object);



extern void NET_SortFree(SortStruct * sort_struct);

#endif /* MKSORT_H */
