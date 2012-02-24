#include "squid.h"
#include "event.h"

#define STUB_API "event.cc"
#include "tests/STUB.h"

void eventAdd(const char *name, EVH * func, void *arg, double when, int, bool cbdata) STUB
void eventAddIsh(const char *name, EVH * func, void *arg, double delta_ish, int) STUB
void eventDelete(EVH * func, void *arg) STUB
void eventInit(void) STUB
void eventFreeMemory(void) STUB
int eventFind(EVH *, void *) STUB_RETVAL(-1)

// ev_entry::ev_entry(char const * name, EVH * func, void *arg, double when, int weight, bool cbdata) STUB
// ev_entry::~ev_entry() STUB
//    MEMPROXY_CLASS(ev_entry);
//    EVH *func;

//MEMPROXY_CLASS_INLINE(ev_entry);

EventScheduler::EventScheduler() STUB
EventScheduler::~EventScheduler() STUB
void EventScheduler::cancel(EVH * func, void * arg) STUB
void EventScheduler::clean() STUB
int EventScheduler::checkDelay() STUB_RETVAL(-1)
void EventScheduler::dump(StoreEntry *) STUB
bool EventScheduler::find(EVH * func, void * arg) STUB_RETVAL(false)
void EventScheduler::schedule(const char *name, EVH * func, void *arg, double when, int weight, bool cbdata) STUB
int EventScheduler::checkEvents(int timeout) STUB_RETVAL(-1)
EventScheduler *EventScheduler::GetInstance() STUB_RETVAL(NULL)
