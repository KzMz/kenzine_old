#include "event.h"
#include "memory.h"
#include "lib/containers/dyn_array.h"

#define MAX_MESSAGE_CODES 16 * 1024

typedef struct EventSubscription
{
    void* listener;
    EventCallback callback;
} EventSubscription;

typedef struct EventCodeEntry 
{
    EventSubscription* subscriptions;
}  EventCodeEntry;

typedef struct EventSystem
{
    EventCodeEntry event_entries[MAX_MESSAGE_CODES];
} EventSystem;

static bool initialized = false;
static EventSystem event_system = {0};

bool event_system_init(void)
{
    if (initialized)
    {
        return false;
    }

    initialized = false;
    memory_zero(&event_system, sizeof(EventSystem));

    initialized = true;
    return true;
}

void event_system_shutdown(void)
{
    if (!initialized)
    {
        return;
    }

    for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i)
    {
        EventCodeEntry* entry = &event_system.event_entries[i];
        if (entry->subscriptions)
        {
            dynarray_destroy(entry->subscriptions);
            entry->subscriptions = (void*) 0;
        }
    }

    memory_zero(&event_system, sizeof(EventSystem));
    initialized = false;
}

bool event_subscribe(u16 code, void* listener, EventCallback callback)
{
    if (!initialized)
    {
        return false;
    }

    EventCodeEntry* entry = &event_system.event_entries[code];
    if (!entry->subscriptions)
    {
        entry->subscriptions = dynarray_create(EventSubscription);
    }

    u64 count = dynarray_length(entry->subscriptions);
    for (u64 i = 0; i < count; ++i)
    {
        if (entry->subscriptions[i].listener == listener)
        {
            // TODO: warning already registered listener
            return false;
        }
    }

    EventSubscription new_subscription = { 
        .listener = listener,
        .callback = callback
    };
    dynarray_push(entry->subscriptions, new_subscription);

    return true;
}

bool event_unsubscribe(u16 code, void* listener, EventCallback callback)
{
    if (!initialized)
    {
        return false;
    }

    EventCodeEntry* entry = &event_system.event_entries[code];
    if (!entry->subscriptions)
    {
        return false;
    }

    u64 count = dynarray_length(entry->subscriptions);
    for (u64 i = 0; i < count; ++i)
    {
        EventSubscription subscription = entry->subscriptions[i];
        if (subscription.listener == listener && subscription.callback == callback)
        {
            EventSubscription popped = {0};
            dynarray_remove(entry->subscriptions, i, &popped);
            return true;
        }
    }

    return false;
}

bool event_trigger(u16 code, void* sender, EventContext context)
{
    if (!initialized)
    {
        return false;
    }

    EventCodeEntry* entry = &event_system.event_entries[code];
    if (!entry->subscriptions)
    {
        return false;
    }

    u64 count = dynarray_length(entry->subscriptions);
    for (u64 i = 0; i < count; ++i)
    {
        EventSubscription subscription = entry->subscriptions[i];
        if (subscription.callback(code, sender, subscription.listener, context))
        {
            return true;
        }
    }

    return false;
}