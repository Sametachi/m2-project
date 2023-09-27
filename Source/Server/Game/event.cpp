#include "stdafx.h"
#include "event_queue.h"

static CEventQueue cxx_q;

/* Create and return an event */
LPEVENT event_create_ex(TEVENTFUNC func, event_info_data* info, int32_t when)
{
	LPEVENT new_event = nullptr;

	/* Be sure to call it after more than the next pulse has passed. */
	if (when < 1)
		when = 1;

	new_event = M2_NEW event;

	assert(NULL != new_event);

	new_event->func = func;
	new_event->info	= info;
	new_event->q_el	= cxx_q.Enqueue(new_event, when, thecore_heart->pulse);
	new_event->is_processing = FALSE;
	new_event->is_force_to_end = FALSE;

	return (new_event);
}

/* Remove the event from the system */
void event_cancel(LPEVENT* ppevent)
{
	LPEVENT event;

	if (!ppevent)
	{
		SysLog("null pointer");
		return;
	}

	if (!(event =* ppevent))
		return;

	if (event->is_processing)
	{
		event->is_force_to_end = TRUE;

		if (event->q_el)
			event->q_el->bCancel = TRUE;

		*ppevent = nullptr;
		return;
	}

	if (!event->q_el)
	{
		*ppevent = nullptr;
		return;
	}

	if (event->q_el->bCancel)
	{
		*ppevent = nullptr;
		return;
	}

	event->q_el->bCancel = TRUE;

	*ppevent = nullptr;
}

void event_reset_time(LPEVENT event, int32_t when)
{
	if (!event->is_processing)
	{
		if (event->q_el)
			event->q_el->bCancel = TRUE;

		event->q_el = cxx_q.Enqueue(event, when, thecore_heart->pulse);
	}
}

/* Execute the events that have reached the time to execute the event */
int32_t event_process(int32_t pulse)
{
	int32_t	new_time;
	int32_t		num_events = 0;

	while (pulse >= cxx_q.GetTopKey())
	{
		TQueueElement* pElem = cxx_q.Dequeue();

		if (pElem->bCancel)
		{
			cxx_q.Delete(pElem);
			continue;
		}

		new_time = pElem->iKey;

		LPEVENT the_event = pElem->pvData;
		int32_t processing_time = event_processing_time(the_event);
		cxx_q.Delete(pElem);
		
		/*
		* The return value is a new time, and if the return value is greater than 0, the event is added again.
		* If the return value is set to 0 or more, do not delete the memory information allocated to the event.
		* Be careful.
		*/
		the_event->is_processing = TRUE;

		if (!the_event->info)
		{
			the_event->q_el = nullptr;
			SysLog("Can not find event info");
		}
		else
		{
			new_time = (the_event->func) (get_pointer(the_event), processing_time);
			
			if (new_time <= 0 || the_event->is_force_to_end)
			{
				the_event->q_el = nullptr;
			}
			else
			{
				the_event->q_el = cxx_q.Enqueue(the_event, new_time, pulse);
				the_event->is_processing = FALSE;
			}
		}

		++num_events;
	}

	return num_events;
}

/* Returns the execution time of the event in pulse units */
int32_t event_processing_time(LPEVENT event)
{
	int32_t start_time;

	if (!event->q_el)
		return 0;

	start_time = event->q_el->iStartTime;
	return (thecore_heart->pulse - start_time);
}

/* Returns the remaining time of the event in pulse units */
int32_t event_time(LPEVENT event)
{
	int32_t when;

	if (!event->q_el)
		return 0;

	when = event->q_el->iKey;
	return (when - thecore_heart->pulse);
}

/* Remove all events */
void event_destroy(void)
{
	TQueueElement* pElem;

	while ((pElem = cxx_q.Dequeue()))
	{
		LPEVENT the_event = (LPEVENT) pElem->pvData;

		if (!pElem->bCancel)
		{
			// no op here
		}

		cxx_q.Delete(pElem);
	}
}

int32_t event_count()
{
	return cxx_q.Size();
}

void intrusive_ptr_add_ref(EVENT* p) {
	++(p->ref_count);
}

void intrusive_ptr_release(EVENT* p) {
	if (--(p->ref_count) == 0) 
	{
		M2_DELETE(p);
	}
}
