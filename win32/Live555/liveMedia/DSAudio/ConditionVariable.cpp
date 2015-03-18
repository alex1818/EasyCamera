#include "General.h"

#include "ConditionVariable.h"

#include <stack>

namespace base {

ConditionVariable::ConditionVariable(Lock* user_lock)
    : user_lock_(*user_lock),
    run_state_(RUNNING),
    allocation_counter_(0),
    recycling_list_size_(0) {
        //DCHECK(user_lock);
}

ConditionVariable::~ConditionVariable() {
    AutoLock auto_lock(internal_lock_);
    run_state_ = SHUTDOWN;  // Prevent any more waiting.

    //DCHECK_EQ(recycling_list_size_, allocation_counter_);
    if (recycling_list_size_ != allocation_counter_) {  // Rare shutdown problem.
        // There are threads of execution still in this->TimedWait() and yet the
        // caller has instigated the destruction of this instance :-/.
        // A common reason for such "overly hasty" destruction is that the caller
        // was not willing to wait for all the threads to terminate.  Such hasty
        // actions are a violation of our usage contract, but we'll give the
        // waiting thread(s) one last chance to exit gracefully (prior to our
        // destruction).
        // Note: waiting_list_ *might* be empty, but recycling is still pending.
        AutoUnlock auto_unlock(internal_lock_);
        Broadcast();  // Make sure all waiting threads have been signaled.
        Sleep(10);  // Give threads a chance to grab internal_lock_.
        // All contained threads should be blocked on user_lock_ by now :-).
    }  // Reacquire internal_lock_.

    //DCHECK_EQ(recycling_list_size_, allocation_counter_);
}

void ConditionVariable::Wait() {
    // Default to "wait forever" timing, which means have to get a Signal()
    // or Broadcast() to come out of this wait state.
    TimedWait(INFINITE);
}

void ConditionVariable::TimedWait(int millSeconds) {
    Event* waiting_event;
    HANDLE handle;
    {
        AutoLock auto_lock(internal_lock_);
        if (RUNNING != run_state_) return;  // Destruction in progress.
        waiting_event = GetEventForWaiting();
        handle = waiting_event->handle();
        //DCHECK(handle);
    }  // Release internal_lock.

    {
        AutoUnlock unlock(user_lock_);  // Release caller's lock
        WaitForSingleObject(handle, static_cast<DWORD>(millSeconds));
        // Minimize spurious signal creation window by recycling asap.
        AutoLock auto_lock(internal_lock_);
        RecycleEvent(waiting_event);
        // Release internal_lock_
    }  // Reacquire callers lock to depth at entry.
}

// Broadcast() is guaranteed to signal all threads that were waiting (i.e., had
// a cv_event internally allocated for them) before Broadcast() was called.
void ConditionVariable::Broadcast() {
    std::stack<HANDLE> handles;  // See FAQ-question-10.
    {
        AutoLock auto_lock(internal_lock_);
        if (waiting_list_.IsEmpty())
            return;
        while (!waiting_list_.IsEmpty())
            // This is not a leak from waiting_list_.  See FAQ-question 12.
            handles.push(waiting_list_.PopBack()->handle());
    }  // Release internal_lock_.
    while (!handles.empty()) {
        SetEvent(handles.top());
        handles.pop();
    }
}

// Signal() will select one of the waiting threads, and signal it (signal its
// cv_event).  For better performance we signal the thread that went to sleep
// most recently (LIFO).  If we want fairness, then we wake the thread that has
// been sleeping the longest (FIFO).
void ConditionVariable::Signal() {
    HANDLE handle;
    {
        AutoLock auto_lock(internal_lock_);
        if (waiting_list_.IsEmpty())
            return;  // No one to signal.
        // Only performance option should be used.
        // This is not a leak from waiting_list.  See FAQ-question 12.
        handle = waiting_list_.PopBack()->handle();  // LIFO.
    }  // Release internal_lock_.
    SetEvent(handle);
}

// GetEventForWaiting() provides a unique cv_event for any caller that needs to
// wait.  This means that (worst case) we may over time create as many cv_event
// objects as there are threads simultaneously using this instance's Wait()
// functionality.
ConditionVariable::Event* ConditionVariable::GetEventForWaiting() {
    // We hold internal_lock, courtesy of Wait().
    Event* cv_event;
    if (0 == recycling_list_size_) {
        //DCHECK(recycling_list_.IsEmpty());
        cv_event = new Event();
        cv_event->InitListElement();
        allocation_counter_++;
        //DCHECK(cv_event->handle());
    } else {
        cv_event = recycling_list_.PopFront();
        recycling_list_size_--;
    }
    waiting_list_.PushBack(cv_event);
    return cv_event;
}

// RecycleEvent() takes a cv_event that was previously used for Wait()ing, and
// recycles it for use in future Wait() calls for this or other threads.
// Note that there is a tiny chance that the cv_event is still signaled when we
// obtain it, and that can cause spurious signals (if/when we re-use the
// cv_event), but such is quite rare (see FAQ-question-5).
void ConditionVariable::RecycleEvent(Event* used_event) {
    // We hold internal_lock, courtesy of Wait().
    // If the cv_event timed out, then it is necessary to remove it from
    // waiting_list_.  If it was selected by Broadcast() or Signal(), then it is
    // already gone.
    used_event->Extract();  // Possibly redundant
    recycling_list_.PushBack(used_event);
    recycling_list_size_++;
}
//------------------------------------------------------------------------------
// The next section provides the implementation for the private Event class.
//------------------------------------------------------------------------------

// Event provides a doubly-linked-list of events for use exclusively by the
// ConditionVariable class.

// This custom container was crafted because no simple combination of STL
// classes appeared to support the functionality required.  The specific
// unusual requirement for a linked-list-class is support for the Extract()
// method, which can remove an element from a list, potentially for insertion
// into a second list.  Most critically, the Extract() method is idempotent,
// turning the indicated element into an extracted singleton whether it was
// contained in a list or not.  This functionality allows one (or more) of
// threads to do the extraction.  The iterator that identifies this extractable
// element (in this case, a pointer to the list element) can be used after
// arbitrary manipulation of the (possibly) enclosing list container.  In
// general, STL containers do not provide iterators that can be used across
// modifications (insertions/extractions) of the enclosing containers, and
// certainly don't provide iterators that can be used if the identified
// element is *deleted* (removed) from the container.

// It is possible to use multiple redundant containers, such as an STL list,
// and an STL map, to achieve similar container semantics.  This container has
// only O(1) methods, while the corresponding (multiple) STL container approach
// would have more complex O(log(N)) methods (yeah... N isn't that large).
// Multiple containers also makes correctness more difficult to assert, as
// data is redundantly stored and maintained, which is generally evil.

ConditionVariable::Event::Event() : handle_(0) {
    next_ = prev_ = this;  // Self referencing circular.
}

ConditionVariable::Event::~Event() {
    if (0 == handle_) {
        // This is the list holder
        while (!IsEmpty()) {
            Event* cv_event = PopFront();
            //DCHECK(cv_event->ValidateAsItem());
            delete cv_event;
        }
    }
    //DCHECK(IsSingleton());
    if (0 != handle_) {
        int ret_val = CloseHandle(handle_);
        //DCHECK(ret_val);
    }
}

// Change a container instance permanently into an element of a list.
void ConditionVariable::Event::InitListElement() {
    //DCHECK(!handle_);
    handle_ = CreateEvent(NULL, false, false, NULL);
    //DCHECK(handle_);
}

// Methods for use on lists.
bool ConditionVariable::Event::IsEmpty() const {
    //DCHECK(ValidateAsList());
    return IsSingleton();
}

void ConditionVariable::Event::PushBack(Event* other) {
    //DCHECK(ValidateAsList());
    //DCHECK(other->ValidateAsItem());
    //DCHECK(other->IsSingleton());
    // Prepare other for insertion.
    other->prev_ = prev_;
    other->next_ = this;
    // Cut into list.
    prev_->next_ = other;
    prev_ = other;
    //DCHECK(ValidateAsDistinct(other));
}

ConditionVariable::Event* ConditionVariable::Event::PopFront() {
    //DCHECK(ValidateAsList());
    //DCHECK(!IsSingleton());
    return next_->Extract();
}

ConditionVariable::Event* ConditionVariable::Event::PopBack() {
    //DCHECK(ValidateAsList());
    //DCHECK(!IsSingleton());
    return prev_->Extract();
}

// Methods for use on list elements.
// Accessor method.
HANDLE ConditionVariable::Event::handle() const {
    //DCHECK(ValidateAsItem());
    return handle_;
}

// Pull an element from a list (if it's in one).
ConditionVariable::Event* ConditionVariable::Event::Extract() {
    //DCHECK(ValidateAsItem());
    if (!IsSingleton()) {
        // Stitch neighbors together.
        next_->prev_ = prev_;
        prev_->next_ = next_;
        // Make extractee into a singleton.
        prev_ = next_ = this;
    }
    //DCHECK(IsSingleton());
    return this;
}

// Method for use on a list element or on a list.
bool ConditionVariable::Event::IsSingleton() const {
    //DCHECK(ValidateLinks());
    return next_ == this;
}

// Provide pre/post conditions to validate correct manipulations.
bool ConditionVariable::Event::ValidateAsDistinct(Event* other) const {
    return ValidateLinks() && other->ValidateLinks() && (this != other);
}

bool ConditionVariable::Event::ValidateAsItem() const {
    return (0 != handle_) && ValidateLinks();
}

bool ConditionVariable::Event::ValidateAsList() const {
    return (0 == handle_) && ValidateLinks();
}

bool ConditionVariable::Event::ValidateLinks() const {
    // Make sure both of our neighbors have links that point back to us.
    // We don't do the O(n) check and traverse the whole loop, and instead only
    // do a local check to (and returning from) our immediate neighbors.
    return (next_->prev_ == this) && (prev_->next_ == this);
}

} // namespace base
