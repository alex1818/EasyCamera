#ifndef _CONDITION_VARIABLE_H_
#define _CONDITION_VARIABLE_H_

#include "Lock.h"

namespace base {

class ConditionVariable {
public:
    // Construct a cv for use with ONLY one user lock.
    explicit ConditionVariable(Lock* user_lock);

    ~ConditionVariable();

    // Wait() releases the caller's critical section atomically as it starts to
    // sleep, and the reacquires it when it is signaled.
    void Wait();
    void TimedWait(int millSeconds);

    // Broadcast() revives all waiting threads.
    void Broadcast();
    // Signal() revives one waiting thread.
    void Signal();

private:

    // Define Event class that is used to form circularly linked lists.
    // The list container is an element with NULL as its handle_ value.
    // The actual list elements have a non-zero handle_ value.
    // All calls to methods MUST be done under protection of a lock so that links
    // can be validated.  Without the lock, some links might asynchronously
    // change, and the assertions would fail (as would list change operations).
    class Event {
    public:
        // Default constructor with no arguments creates a list container.
        Event();
        ~Event();

        // InitListElement transitions an instance from a container, to an element.
        void InitListElement();

        // Methods for use on lists.
        bool IsEmpty() const;
        void PushBack(Event* other);
        Event* PopFront();
        Event* PopBack();

        // Methods for use on list elements.
        // Accessor method.
        HANDLE handle() const;
        // Pull an element from a list (if it's in one).
        Event* Extract();

        // Method for use on a list element or on a list.
        bool IsSingleton() const;

    private:
        // Provide pre/post conditions to validate correct manipulations.
        bool ValidateAsDistinct(Event* other) const;
        bool ValidateAsItem() const;
        bool ValidateAsList() const;
        bool ValidateLinks() const;

        HANDLE handle_;
        Event* next_;
        Event* prev_;
        DISALLOW_COPY_AND_ASSIGN(Event);
    };

    // Note that RUNNING is an unlikely number to have in RAM by accident.
    // This helps with defensive destructor coding in the face of user error.
    enum RunState { SHUTDOWN = 0, RUNNING = 64213 };

    // Internal implementation methods supporting Wait().
    Event* GetEventForWaiting();
    void RecycleEvent(Event* used_event);

    RunState run_state_;

    // Private critical section for access to member data.
    base::Lock internal_lock_;

    // Lock that is acquired before calling Wait().
    base::Lock& user_lock_;

    // Events that threads are blocked on.
    Event waiting_list_;

    // Free list for old events.
    Event recycling_list_;
    int recycling_list_size_;

    // The number of allocated, but not yet deleted events.
    int allocation_counter_;


    DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

} // namespace base

#endif // _CONDITION_VARIABLE_H_
