#ifndef AWT_CLIST_H
#define AWT_CLIST_H

// -----------------------------------------------------------------------
//
// Simple circular linked-list implementation...
//
// -----------------------------------------------------------------------

// Foreward declarations...
struct AwtCList;


struct AwtCList {
    AwtCList *next;
    AwtCList *prev;

    AwtCList() {
        next = prev = this;
    }

    ~AwtCList() {
        Remove();
    }
        
    //
    // Append an element to the end of this list
    //
    void Append(AwtCList &element) {
        element.next = this;
        element.prev = prev;
        prev->next = &element;
        prev = &element;
    }

    //
    // Append this element to the end of a list
    //
    void AppendToList(AwtCList &list) {
        list.Append(*this);
    }

    //
    // Remove this element from the list and re-initialize
    //
    void Remove(void) {
        prev->next = next;
        next->prev = prev;
 
        next = prev = this;
    }

    //
    // Is this list empty ?
    //
    BOOL IsEmpty(void) {
        return (next == this);
    }
};    

#endif // AWT_CLIST_H