#pragma once

#include "mcu-system.h"

// Currently under development
#define OPT_USE_WATCHDOG	0
// Currently under development
#define OPT_USE_ISR			0
// Currently under development
#define OPT_SIMPLE_SUPERVISOR 0


/// Expected return values for TaskProc_t procedure
/*!
This return values are valid for normal tasks. For ISR and tick tasks there are special notes to consider.
The following rules shall be noted:
- Normal task: See description as noted on each enum value.
- Tick Task: Tick tasks are run once per tick until a kTaskStop is returned when they will stop until a they
  are reactivated with an appropriate StartTickTask() call.
- ISR Task: These are high priority tasks and will be called ASAP but only once, regardless of the return 
  value. This has the same effect as always returning kTaskStop.
*/
enum TaskResult
{
	/// Indicates the Job can be removed from the queue
	/**	Return this value for one-shot jobs. */
	kTaskStop,
	/// Indicates the job should be called later again (after a sleep cycle)
	/** This is the preferred way for looping jobs to terminate, since it
	lets processor sleep until the next interrupt, saving battery
	power.
	Exception to this rule: This value is ignored for ISR task, which will always behave as
	kTaskStop.*/
	kTaskSleep,
	/// Indicates the job is giving a chance to other concurrent jobs.
	/** Return this value if you are still requiring processor time but
	wants to cooperate with other scheduled tasks. As soon as possible
	(ASAP) OnRunJob() will be called again.
	Exceptions to this rule: This value has equivalent behavior as kTaskSleep for "tick tasks" and
	is kTaskStop for ISR tasks. */
	kTaskRunning
};

// Forward declare
class Task;
class TaskQueueProxy;

/// Datatype for task procedures
typedef TaskResult (*TaskProc_t)(volatile Task*);
/// Datatype for ISR task procedures
typedef void (*IsrProc_t)(void *pParam);


/// Structure used to define a task
/** You should associate define any schedulable task with an instance of this
structure.

The structure can be queued into the main process queue and executed as
soon as the processor awakes, usually as a consequence of an interrupt.

Routines here belongs to the main process and are executed outside from an
interrupt handler. They can be interrupted at anytime.
If you enter a critical section you should suspend interrupts. This must
be done briefly, so you won't compromise system services response time.

Tasks interacts cooperatively. So as long as one task is executing the
other pending ones will be waiting. This is because they are called
sequentially.

You should dimension task duration according to your needs. Sometimes it
is necessary to split lengthy jobs into several pieces, resulting in a
smooth "multi-tasking" effect. */
class Task
{
public:
	friend class TaskQueue;
	friend class TaskQueueProxy;

	/// Value for Task::next_ that indicates end of list item
	static constexpr const uintptr_t kEol_ = 0;
	/// Value for Task::next_ that indicates unlinked state
	static constexpr const uintptr_t kUnlinked_ = 1;
	/// Value for Task::next_ that indicates running state
	static constexpr const uintptr_t kRunning_ = 3;

	union Flags
	{
		enum Raw : uint32_t
		{
			kClear = 0,
			kTaskVerify = 0x100,
			kDelete = 0x200,
		};
		Raw raw_;
		struct  
		{
			uint32_t counter : 8;		//!< A counter used by the supervisor to detect orphan tasks
			uint32_t task_verify : 1;	//!< Verify flag for integrity checker
			uint32_t del_flag : 1;		//!< Task deletion is pending and will be cleared when possible
		};
	};

	/// Points to the next job in queue
	volatile Task* next_;
	/// Function to be executed (cannot be NULL)
	TaskProc_t proc_;
	/// Tick time when task was scheduled
	uint32_t time_;
	/// Status fields and integrity check redundancy
	Flags flags_;
	//! Task private data, free to use
	uint32_t param_;

	//! Simple execute the associated procedure (warning: outside scheduler!!!)
	ALWAYS_INLINE TaskResult Execute() volatile
	{
		_TASK_SetVerifyFlag();
		return (*proc_)(this);
	}

	/// Constructs a task object (call it to initialize any instance)
	ALWAYS_INLINE void Init(TaskProc_t pFunc) volatile
	{
		next_ = (Task*)kUnlinked_;
		proc_ = pFunc;
		flags_.raw_ = Flags::kClear;
		param_ = 0;
	}
	//! Constructs a task object to run outside the task scheduler using Execute() method directly (not recommended!)
	ALWAYS_INLINE void InitEx(TaskProc_t pFunc, uint32_t tick) volatile
	{
		next_ = (Task*)kUnlinked_;
		proc_ = pFunc;
		flags_.raw_ = Flags::kTaskVerify;
		param_ = 0;
		InitTimer(tick);
	}
	/// Overrides current function pointer and restarts timer
	ALWAYS_INLINE void Override(TaskProc_t pFunc, uint32_t tick) volatile
	{
		proc_ = pFunc;
		InitTimer(tick);
	}
	/// Restarts TASK timer value; causes GetDuration() to be zeroed
	ALWAYS_INLINE void InitTimer(uint32_t tick) volatile { time_ = tick; }

	/// This negates IsInSchedule() and indicates task object is free
	ALWAYS_INLINE bool IsFree() volatile const { return (uintptr_t)next_ == kUnlinked_; }
	/// Return true if object is linked to the task list
	ALWAYS_INLINE bool IsInSchedule() volatile const { return IsFree() == false; }
	/// Return true if object is currently running
	ALWAYS_INLINE bool IsRunning() volatile const { return ((uintptr_t)next_ == kRunning_); }
	/// Return true if object is the end of the list
	ALWAYS_INLINE bool IsEOL() volatile const { return ((uintptr_t)next_ == kEol_); }
	/// Returns the duration of task since it was scheduled.
	/** Use this macro to obtain the duration of a task in units of 5 ms. You will
	typically use this result to check if any specified time interval was
	elapsed, enabling you to return "jobSleep" from your task while the
	desired time amount has not passed, without blocking other tasks. This
	feature avoids the use of System::Sleep(), which blocks other tasks.
	Beware to note that this variables are computed in 16 bits arithmetics,
	so you should not consider intervals beyond 5 minutes.
	HINT: Use the TM_MSEC macro for easy ms conversion. */
	ALWAYS_INLINE uint32_t GetDuration(uint32_t tick) volatile const { return ((uint32_t)(tick - time_)); }

protected:
	/**
	This method removes the given tick task from the tick queue. Tick task
	are executed once every 200 ms and are designed to replace tick-interrupt
	procedures (TM_RegisterProc200()/TM_StopProc200() methods).
	Note that these approach reduces interrupt latencies, since the tasks are
	executed in the main process.
	*/
	void StopTickTask() volatile
	{
		if (IsInSchedule())
			MarkForDelete_();
	}

protected:
	/// This macro is used to mark task to be automatically deleted
	ALWAYS_INLINE void MarkForDelete_() volatile { flags_.del_flag = true; }
	/// Reset deleted flag of a task
	ALWAYS_INLINE void ResetDeleteFlag_() volatile { flags_.del_flag = false; }
	/// Returns true if a task is marked for delete
	ALWAYS_INLINE bool IsMarkedForDelete_() volatile const { return flags_.del_flag != 0; }

// Supervisor
protected:
	/// Unpacks Retry counter for task supervisor
	ALWAYS_INLINE uint8_t GetCounter_() volatile const { return (uint8_t)flags_.counter; }
	/// Clear retry counter
	ALWAYS_INLINE void ClrCounter_() volatile { flags_.counter = 0; }
	/// Increment retry counter
	ALWAYS_INLINE void IncCounter_() volatile { ++flags_.counter; }
	/// Checks for verify flag
	ALWAYS_INLINE bool IsVerifyFlag_() volatile const { return flags_.task_verify != 0; }
	/// Clears the verify flag
	ALWAYS_INLINE void ClearVerifyFlag_() volatile { flags_.task_verify = false; }
	/// Mark the verify flag
	ALWAYS_INLINE void _TASK_SetVerifyFlag() volatile { flags_.task_verify = true; }
};


class ALIGNED TaskQueue
{
private:
	friend class TaskQueueProxy;
	/// Head of the list
	volatile Task* m_pQueueStart;
	/// Points to the tail of the list
	volatile Task* m_pQueueEnd;

public:
	/// Constructs a task queue
	ALWAYS_INLINE void Reset() volatile
	{
		m_pQueueStart = (Task *)Task::kEol_;
		m_pQueueEnd = (Task*)Task::kEol_;
	}
	/// Result valid when called from inside a task, about the queue that owns it
	ALWAYS_INLINE bool IsLastTask() const volatile
	{
		return IsEmpty();
	}
	//! Checks if this queue is empty
	ALWAYS_INLINE bool IsEmpty() const volatile
	{
		return (((uintptr_t)m_pQueueStart == Task::kEol_) && ((uintptr_t)m_pQueueEnd == Task::kEol_));
	}

protected:
	/// Puts a job object to the tail of the list
	bool Queue(volatile Task& t, TaskProc_t proc, uint32_t timestamp) volatile;
	/// Puts a ISR job at the tail of the list or mark it for rerun
	void QueueIsrTask(volatile Task& t, uint32_t timestamp) volatile;
	/// Removes the head of the list
	volatile Task* GetRunningTask_() volatile;
	/// Removes a task from the given task list
	bool Unlink_(volatile Task& t) volatile;

private:
	/// Queues an task object into the given queue
	void Queue_(volatile Task& t) volatile;
};



class TaskQueueProxy
{
protected:
	//! Constructor
	void ctor();

	/// Removes a task from the schedule
	void StopTask(volatile Task& t);

protected:
	static void StopTickTask(volatile Task& t) { t.StopTickTask(); }
	static volatile Task* GetRunningTask_(volatile TaskQueue& q) { return q.GetRunningTask_(); }

protected:
	/// Global process queue instance for Interrupt completion tasks
	//volatile TaskQueue m_IsrQueue;
	/// Global process queue instance for normal processes
	volatile TaskQueue m_TaskQueue;
	/// This is the temporary task list
	TaskQueue	m_Reschedule;
	/// This is the task list for "ticked" procedures
	volatile TaskQueue m_TickQueue;
	/// Variable used to control tick changes;
	uint32_t m_TickFollower;
	/// ISR handler counter
	//uint32_t m_IsrCnt;
};


/// This structure controls a task queue.
template <typename Tick, typename CritSect>
class TaskQueueSystem : public TaskQueueProxy
{
public:
	static TaskQueueSystem<Tick, CritSect> task_manager_;

	//! Constructor
	ALWAYS_INLINE void Init()
	{
		Tick::Init();
		TaskQueueProxy::ctor();
		m_TickFollower = Tick::soft_tick_;
	}

	/// Can be called from any task to check if there are other tasks to run
	ALWAYS_INLINE bool IsLastTask() volatile
	{
		return m_TaskQueue.IsLastTask();
	}
	/// Schedules a task to be executed with normal priority by the main process.
	ALWAYS_INLINE bool StartTask(volatile Task& t, TaskProc_t p = NULL)
	{
		CritSect lock;
		return m_TaskQueue.Queue(t, p, Tick::sys_tick_);
	}
	/// Schedules a task to be executed on the next tick cycle by the main process.
	/** A task tick task can only be unlinked by itself */
	ALWAYS_INLINE bool StartTickTask(volatile Task& t, TaskProc_t p = NULL)
	{
		CritSect lock;
		return m_TickQueue.Queue(t, p, Tick::sys_tick_);
	}

public:
	/// Starts looping for tasks execution
	ALWAYS_INLINE void BeginLooping()
	{
		m_TickFollower = Tick::soft_tick_;
	}

	/// Executes scheduled tasks
/**
This method is the core of the task executer. It consists of the
following steps:
	-	Rearms the watchdog timer, confirming system main task is running
	properly.
	- Checks for the tick counter to identify tick-counter transitions,
	which are flagged by the State::g_Ctrl.bits.m_fTickAck bit.
	-	Then it is optimized to return as quickly as possible if no tasks are
	scheduled, skipping any unnecessary code, so the MSP may return
	faster to Low Power Mode.
	- Optionally this routine may test for clock stability using MSP
	oscillator fault logic, which is controlled by the OPT_OSC_FAULT
	configuration macro.
	-	All scheduled tasks are executed one by one. Depending on the return
	value of the task (see TaskResult) it will remove, reschedule or
	immediate reschedule it.

This scheduler implements the basis for different task requirements,
depending on the tasks return value and the analysis of some extra data.
The execution will start after any interrupt that causes CPU to waken and
the most common situations are described below:
	- One shot tasks: are tasks that are scheduled by an interrupt or other
	task, that implements a custom procedure and at its exit it is
	automatically unlinked and will not execute anymore until the
	interrupt or other task reschedules it. A task of this type has a
	'jobStop' return value;
	- Real-time looping tasks: are tasks that are executed using all
	available CPU power, but gives chance for other tasks to be executed,
	cooperatively. These type of task has a 'jobRunning' return value;
	- ASAP looping tasks: are tasks executed after the CPU waken, that
	requires immediate CPU attention, and short response time, but can
	then return to sleep state for a while. A typical case is a task which
	waits for an external resource to be available: it can monitor the
	resource and sleep a while before another retry. A task of this type
	has a 'jobSleep' return value;
	- A combination of the <B>ASAP looping task</B> with the
	State::g_Ctrl.bits.m_fTickAck flag to operate on a reliable 5ms rate.
	- Combining the <B>ASAP looping task</B> with the GetDuration(),
	to control events that depends on a time interval. This method can
	measure intervals of up to 5:27 minutes. As an example, imagine a
	routine to write a sector to the flash card. The card may be busy
	because of a previous write, so we return \ref jobSleep until it
	is ready. We can use GetDuration() to check for a 500 ms timeout,
	in which case we signal an error and abort the write operation by
	returning \ref jobStop. If data has been written, we also return
	\ref jobStop. */
	void Execute()
	{
		volatile Task* pJob, * pNext;

#if OPT_USE_WATCHDOG
		// Rearm watchdog timer, so system keeps running
		WDT::CheckPoint();
#endif

#if OPT_USE_ISR
		RunISR();
#endif

		/*
		 * Here we have two distinct situations: Tasks running in 5 ms intervals
		 * and ASAP tasks.
		 *
		 * For the tasks in 5 ms intervals we will use the m_TickFollower
		 * variable, which is maintained here. We will follow the timer tick
		 * counter. When the values aren't matching we increment the
		 * m_TickFollower member. It will be incremented until it matches the
		 * system tick count, producing an equivalent task execution of 5 ms,
		 * although time jitter may arise (If jitter is a problem, then it is
		 * recommended to use the timer B real-time interrupt procedures).
		 *
		 * ASAP tasks, on the other hand, are executed every time the processor
		 * ends an interrupt routine.
		 *
		 * Note: ASAP = As Soon As Possible
		 */

		bool fTickAck = false;
		/*
		 * At first instance, we will check quickly if something has to be done.
		 * if not return immediately, so we return to sleep quickly.
		 */
		if (m_TickFollower != Tick::soft_tick_)
		{
			// Do one tick per cycle. This grants we run tick tasks one per tick,
			// even on a busy environment.
			m_TickFollower++;
			fTickAck = true;
		}
		else if (m_TaskQueue.m_pQueueStart == NULL)
			return;

		// Tick tasks are of higher priority
		if (fTickAck)
		{
			/*
			**	This implements the task supervisor code. It is a redundant code
			**	that keeps track of dead-locked tasks objects and unlock them as
			**	necessary. A deadlock could occur if the Task::next_ member
			**	contains a value different of Task::kUnlinked_, but the object is
			**	not ready linked to any of out task list. In this case we can
			**	Queue() as many times we want but the node will never get
			**	reused, because it will never get called, since all methods
			**	handles it as an "already linked" task.
			**	All used task objects are laid in a reserved RAM space on a the
			**	.bss.scheduler segment, exactly as an array. (see *.ld files)
			**	This supervisor will run this array and clear the m_fVerify flag
			**	from all objects. After the call. It will look for all unused
			**	tasks and keep them in the unlinked state.
			**
			**	This code was implemented after hours an hours of code revision.
			**	Some customers complained that devices stopped recording all of a
			**	sudden. After many research all pointed out to a firmware bug that
			**	had a statistical nature and could be caused by the task
			**	scheduler.
			*/
			CritSect lock;
#if OPT_SIMPLE_SUPERVISOR
			for (pJob = &_taskobjs_begin; pJob < &_taskobjs_end; ++pJob)
				pJob->ClearVerifyFlag_();
#endif

			// Executes tick threads
			for (pJob = m_TickQueue.m_pQueueStart; pJob; pJob = pNext)
			{
#if OPT_USE_WATCHDOG
				// Rearm watchdog timer, so system keeps running
				WDT::CheckPoint();
#endif
#if OPT_USE_ISR
				RunISR();
#endif
				pJob->_TASK_SetVerifyFlag();
				pNext = pJob->next_;
				TaskProc_t pProc = pJob->proc_;
				pJob->next_ = Task::kRunning_;
				lock.EnableInts();
				bool fContinue = (pProc && !pJob->IsMarkedForDelete_() && (*pProc)(pJob) != kTaskStop);
				lock.DisableInts();
				pJob->next_ = pNext;
				if (fContinue == false)
					m_TickQueue.Unlink_(*pJob);	// unlink task
			}
			lock.EnableInts();
		}

		// Loop while we have jobs to run
		while ((pJob = m_TaskQueue.GetRunningTask_()) != NULL)
		{
#if OPT_USE_WATCHDOG
			// Rearm watchdog timer, so system keeps running
			WDT::CheckPoint();
#endif

#if OPT_USE_ISR
			RunISR();
#endif
			// Signals that the task object was handled
			pJob->_TASK_SetVerifyFlag();

			/*
			 *	Get a copy so we avoid a deadlock caused by an interrupt that
			 *	suddenly clears pJob->proc_ contents.
			 */
			TaskProc_t pProc = pJob->proc_;
			if (pProc && !pJob->IsMarkedForDelete_())
			{
				/*
				 *	EXECUTES THE TASK
				 *
				 *	The task can add new jobs, even reassign itself to a new
				 *	function pointer. Tasks must be designed to be cooperative,
				 *	which means, duration is a concern. Lengthy operations should
				 *	be split into parts to cooperate with other pending tasks
				 *	Its return value may be one of the following:
				 *		jobRunning : The task still needs CPU cycles. It will be
				 *				appended to the tail of the queue, so other tasks
				 *				may use the CPU, but it will be called at the end
				 *				of the current "run cycle".
				 *		jobStop	: Will unlink the job, except if a new function
				 *				pointer was assigned (pJob->proc_ changed!), and
				 *				in this case it will be changed to jobRunning
				 *				state, letting us handle a sequence of operations
				 *				in a single "run-cycle" and reusing the same task
				 *				object (pJob).
				 *		jobSleep : The task will be temporarily suspended and will
				 *				wait until the next "run-cycle"
				 */
				TaskResult nRes = (*pProc)(pJob);

				// BEGIN CRITICAL SECTION
				{
					CritSect lock;
					// Test if someone else requested a task unlink
					if (pJob->IsMarkedForDelete_())
						nRes = kTaskStop;

					// Reschedule job for immediate run
					// (but allows other tasks to interact)
					if (nRes == kTaskRunning)
					{
						m_TaskQueue.Queue_(*pJob);
					}
					// Reschedule jobs for rerun only after sleep on a temporary list
					else if (nRes == kTaskSleep)
						m_Reschedule.Queue_(*pJob);
					// else job is dismissed
					else
						pJob->next_ = Task::kUnlinked_;
				}
				// END CRITICAL SECTION
			}
			else
			{
				/*
				**	Got an empty PROC address or task was marked for delete.
				**	Since it is already unlinked, BUT it still contains the
				**	Task::kRunning_ value, we must ensure it will be ready for new
				**	schedules.
				*/
				CritSect lock;
				pJob->next_ = Task::kUnlinked_;
				pJob->ResetDeleteFlag_();
			}
		}

#if OPT_USE_ISR
		RunISR();
#endif
		/*
		 * Now we have a new job list in m_Reschedule member. This list
		 * contains all tasks that returned "jobSleep", meaning they have low
		 * execution priority and we can put processor into low-power state.
		 * This list is stored temporarily in m_Reschedule and must be copied
		 * back to the main list (m_Jobs_RUN).
		 */

		 // BEGIN CRITICAL SECTION
		{
			CritSect lock;
			if (m_TaskQueue.m_pQueueStart != (Task *)Task::kEol_)
			{
				/*
				 *	This handles the case where an interrupt appends a task
				 *	immediately after the previous 'while' loop.
				 *	Then, just append both queues.
				 */

				 // Can skip if reschedule queue is empty
				if (m_Reschedule.m_pQueueStart == (Task*)Task::kEol_)
					goto queue_ready;
				// Merge reschedule queue and the task that we've got
				m_Reschedule.m_pQueueEnd->next_ = m_TaskQueue.m_pQueueStart;
				m_Reschedule.m_pQueueEnd = m_TaskQueue.m_pQueueEnd;
			}
			// Copy loops
			(TaskQueue&)m_TaskQueue = m_Reschedule;
			// Clear temporary task list
			m_Reschedule.Reset();
queue_ready:
			;
		}
		// END CRITICAL SECTION

#if OPT_USE_ISR
		RunISR();
#endif

#if OPT_SIMPLE_SUPERVISOR
		if (fTickAck)
		{
			/*
			**	This is the supervisor code that keeps unused task objects
			**	unlocked.
			*/
			pNext = NULL;
			// BEGIN CRITICAL SECTION
			{
				CritSect lock;
				for (pJob = &_taskobjs_begin; pJob < &_taskobjs_end; ++pJob)
				{
					if (!pJob->IsFree() && !pJob->IsVerifyFlag_())
					{
						/*
						**	Got a task that seems to be locked, but no hurry, it must
						**	last for 10 ticks, before we unlock it.
						*/
						if (pJob->GetCounter_() > 10)
						{
							pJob->next_ = Task::kUnlinked_;
							pJob->ClrCounter_();
							pNext = pJob;
						}
						else
							pJob->IncCounter_();
					}
					else
						pJob->ClrCounter_();
				}
			}
			// END CRITICAL SECTION
			if (pNext)
				Mcu::Abort();
		}

#if OPT_USE_ISR
		RunISR();
#endif
#endif	// OPT_SIMPLE_SUPERVISOR

#if OPT_USE_WATCHDOG
		// Rearm watchdog timer, so system keeps running
		WDT::CheckPoint();
#endif
		/*
		 * This function returns to the caller, which should command a low-power
		 * processor state. When an interrupt awakes the processor, this caller
		 * routine should call us again.
		 * This loop repeats again and again until the end of the exam.
		 *
		 */
	}
	/// Put MCU to sleep while handling ISR
	ALWAYS_INLINE void Sleep()
	{
#if OPT_USE_ISR
		RunISR();
#endif
		McuCore::Sleep();
#if OPT_USE_ISR
		RunISR();
#endif
	}

	/// Starts looping for tasks execution
	ALWAYS_INLINE void EndLooping() {}

	/// Removes a task from the schedule
	void StopTask(volatile Task& t)
	{
		CritSect lock;
		TaskQueueProxy::StopTask(t);
	}

	/// Removes a task from the tick scheduler
	void StopTickTask(volatile Task& t)
	{
		CritSect lock;
		StopTickTask(t);
	}
	//! Returns true if any task is in schedule (tick tasks excluded)
	ALWAYS_INLINE bool HasTasks() const { return m_TaskQueue.IsEmpty() == false; }
	//! Returns true if any task is in schedule (tick tasks excluded)
	ALWAYS_INLINE bool HasTickTasks() const { return m_TickQueue.IsEmpty() == false; }
	//! Returns true if any task is in schedule (tick tasks excluded)
	//ALWAYS_INLINE bool HasIsrTasks() const { return m_IsrQueue.IsEmpty() == false; }

protected:
	static volatile Task* GetRunningTask_(volatile TaskQueue& q)
	{
		CritSect lock;
		return q.GetRunningTask_();
	}
};



//! A dummy class to handle special events
class DummyEvent
{
public:
	ALWAYS_INLINE DummyEvent(uintptr_t data=0) {}
};

