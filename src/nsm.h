#ifndef _NSM_H
#define _NSM_H

#include "structure.h"

/* name string of action, event and state */
extern const char *action_str[];
extern const char *event_str[];
extern const char *state_str[];

/* neighbor state machine */
extern const transition nsm[];

void add_event(interface *iface, neighbor *nbr, nbr_event event);

#endif
