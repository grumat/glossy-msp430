#include "Tasks.h"
#include "critical_section.h"

namespace Bmt
{

// These are defined on the linker script
extern Task _taskobjs_begin, _taskobjs_end;

void TaskQueue::Queue_(volatile Task& t) volatile
{
	// A new task is always the tail of the list
	t.next_ = (Task *)Task::kEol_;
	// Link at the head for empty list
	if (IsEmpty())
		m_pQueueStart = &t;
	// Or link at the tail of the list
	else
		m_pQueueEnd->next_ = &t;
	// Update end-of-queue
	m_pQueueEnd = &t;
}


/// Puts a job object to the tail of the list
bool TaskQueue::Queue(volatile Task& t, TaskProc_t proc, uint32_t timestamp) volatile
{
	bool fRes;

	/*
	** What to do when this asserts?
	** If this asserts, you have defined Task using DEFINE_TASK_OBJECT() macro!!
	** Although TaskProc objects are normal objects a reserved RAM segment was defined for all of them
	** so a task supervisor can control all system tasks if the are OK (typically an orphan state).
	*/
	ASSERT(&t >= &_taskobjs_begin && &t < &_taskobjs_end);

	// Never link a job twice
	fRes = t.IsFree();
	if (fRes)
	{
		// Parameter proc is optional
		if (proc)
			t.proc_ = proc;
		// We cannot queue a task without a procedure!
		if ((fRes = (t.proc_ != 0)))
		{
			t.InitTimer(timestamp);
			Queue_(t);
			t.flags_.raw_ = Task::Flags::kTaskVerify;
		}
	}
	return fRes;
}


/// Removes the head of the list and prepares the task for execution
/*!
Before a task is run, it is removed from it's queue and kept unlinked until it returns. The return value
of the task will determine if it will be relinked again or kept unlinked.

\return A pointer to the Task object in unlinked and in a "running state"; it is ready to be executed. A NULL 
	pointer will be returned if the queue is empty.
*/
volatile Task* TaskQueue::GetRunningTask_() volatile
{
	volatile Task* pJob = NULL;

	// Queue must have at least one node to start with
	if ((uintptr_t)m_pQueueStart != Task::kEol_)
	{
		// Get head object
		pJob = m_pQueueStart;
		// Empty list if this is the last
		if (pJob->IsEOL()
			|| m_pQueueStart == m_pQueueEnd)
		{
			// Empties the list
			m_pQueueStart = m_pQueueEnd = (Task *)Task::kEol_;
		}
		else
		{
			// Advance one element in the queue
			m_pQueueStart = pJob->next_;
		}
		// Remove link data
		pJob->next_ = (Task*)Task::kRunning_;
	}

	return pJob;
}



bool TaskQueue::Unlink_(volatile Task& t) volatile
{
	volatile Task* volatile* pObj;

	// Get head object
	pObj = &m_pQueueStart;
	// Scan all objects in this list
	while ((uintptr_t)*pObj > Task::kRunning_)
	{
		// Found the object in this list?
		if (*pObj == &t)
		{
			/*
			 * We must adjust end of queue pointer if it points to our
			 * object and it is our direct pLast value. This is valid when
			 * we have multiple items. In the case we have a single item,
			 * pLast points to the head and not a valid item. This
			 * situation will be corrected later, when we check for the
			 * empty list case.
			 */
			if (m_pQueueEnd == &t)
				m_pQueueEnd = (Task*)pObj;

			// Link our tail to our head
			*pObj = t.next_;

			// Now we check for the empty list case
			if (m_pQueueStart == (Task*)Task::kEol_)
				m_pQueueEnd = (Task *)Task::kEol_;

			// Object is now unlinked. We can stop!
			t.next_ = (Task*)Task::kUnlinked_;
			return true;
		}
		// Fetch next item to continue scanning
		pObj = &(*pObj)->next_;
	}
	// Be sure to let object unlinked or it will be lost for ever
	t.next_ = (Task*)Task::kUnlinked_;
	// Failed to locate object
	return false;
}





void TaskQueueProxy::ctor()
{
	m_TaskQueue.Reset();
	m_TickQueue.Reset();
#if OPT_USE_ISR
	m_IsrQueue.Reset();
	m_IsrCnt = 0;
	for (uint32_t i = 0; i < _countof(m_IsrTasks); ++i)
		m_IsrTasks[i].Init(NULL);
#endif
}


/**
This method verifies if the task is in schedule and removes it from the
task list. If it fails to locate the task in the task list it tries the
temporary task list. If everything fails it simply update the state from
the task object to "unlinked", to preserve object integrity.

Note that if the task is in execution, it is unlinked from any list and no
search in queues will succeed. Instead a flag will mark the Task to be 
removed and as soon as task execution cycle ends it will stop regardless 
from tasks return value. */
void TaskQueueProxy::StopTask(volatile Task& t)
{
	if (t.IsRunning())
		t.MarkForDelete_();
	else if (t.IsInSchedule()
			 && !m_TaskQueue.Unlink_(t))
	{
		m_Reschedule.Unlink_(t);
	}
}


#if OPT_USE_ISR
void TaskQueueSystem::RunISR_()
{
	volatile Task* pJob;

	// Loop while we have jobs to run
	while ((pJob = m_IsrQueue.GetRunningTask_()) != NULL)
	{
		// Rearm watchdog timer, so system keeps running
		WDT::CheckPoint();

		/*
		**	Get a copy so we avoid a deadlock caused by an interrupt that
		**	suddenly clears pJob->proc_ contents.
		*/
		register IsrProc_t pProc = (IsrProc_t)pJob->proc_;
		ASSERT(pProc != NULL);

		/*
		**	EXECUTES THE ISR TASK
		**
		**	The ISR task is a one shot task and should be use for fast and urgent data processing
		**	to avoid that the ISR overwrites data that could not be processed.
		**	The return value of this task type is ignored.
		*/
		(*pProc)((void *)pJob->m_lParam);

		// BEGIN CRITICAL SECTION
		{
			CriticalSectionTasks lock;
			pJob->next_ = Task::kUnlinked_;
		}
		// END CRITICAL SECTION
	}
}
#endif

}	// namespace Bmt
