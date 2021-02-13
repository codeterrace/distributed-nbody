/**
 * \file taskqueue.h
 * \brief A concurrent task queue for delegation of generic tasks to worker
 * threads.
 *
 * This header declares a struct to represent an abstract task, and a struct to
 * represent a FIFO of tasks awaiting execution by one or more worker threads.
 */

#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "queue.h"

#include <pthread.h>
#include <stddef.h>

/**
 * \brief Generic task that can be scheduled for execution.
 *
 * \class task
 *
 * A task is a function pointer and argument pair.
 * Execution of a task corresponds to calling func(arg)
 *
 * Each task owns the memory associated with its argument, and frees this
 * memory after execution. As a result, a task may only be executed once!
 */
struct task {
  void (*func)(void *);
  void *arg;
  size_t arg_size;
};

/**
 * \brief Execute a task.
 * \memberof task
 *
 * \param t Task to execute.
 */
void task_execute(struct task *t);

/**
 * \brief Free any resources associated with task t, leaving t in an
 * uninitialized state.
 * \memberof task
 *
 * \param t Task to destroy.
 */
void task_destroy(struct task *t);

/**
 * \brief Create local copy of function argument for task.
 * \memberof task
 *
 * \param t Task to freeze.
 * \return 0 on success, non-zero on failure.
 */
int task_freeze(struct task *t);

/**
 * \brief Task queue FIFO for assigning tasks to worker threads.
 *
 * \class taskqueue
 *
 * This struct represents a task queue instance, and all of its associated
 * state.
 */
struct taskqueue {
  struct queue queue;

  size_t num_running;   /**< Number of tasks currently running. */
  pthread_mutex_t lock; /**< Mutex for concurrent access. */

  /** Condition variable for notification of waiting threads. */
  pthread_cond_t notify;
};

/**
 * \brief Initialize a task queue.
 * \memberof taskqueue
 *
 * \param q Pointer to task queue to initialize.
 * \return 0 on success, non-zero on failure.
 */
int taskqueue_init(struct taskqueue *q);

/**
 * \brief Destroy taskqueue referred to by q, leaving it uninitialized.
 * \memberof taskqueue
 *
 * \param q Pointer to task queue to destroy.
 * \return 0 on success, non-zero on error.
 */
int taskqueue_destroy(struct taskqueue *q);

/**
 * \brief Push a new task into the task queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \param t Task to push.
 * \return 0 on success, non-zero on failure
 */
int taskqueue_push(struct taskqueue *q, struct task t);

/**
 * \brief Push a N identical copies of task into the task queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \param t Task to push.
 * \param n Number of copies to push.
 * \return 0 on success, non-zero on failure
 */
int taskqueue_push_n(struct taskqueue *q, struct task t, size_t n);

/**
 * \brief Retrieve and pop a task from the task queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \param t Pointer at which to store retrieved task.
 * \return 0 on success, non-zero on failure
 */
int taskqueue_pop(struct taskqueue *q, struct task *t);

/**
 * \brief Get number of pending tasks in the queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \return Number of tasks in the queue.
 */
size_t taskqueue_count(struct taskqueue *q);

/**
 * \brief Block and wait for work to appear on the queue.
 * \memberof taskqueue
 *
 * If there is a pending task on the queue, this function pops the task, and
 * stores it at the location pointed to by t immediately. If no task is pending,
 * this function blocks until there is a pending task and the queue is notified.
 *
 * \param q The task queue.
 * \param t Pointer at which to store the popped task.
 * \return 0 on success, non-zero on failure.
 */
int taskqueue_wait_for_work(struct taskqueue *q, struct task *t);

/**
 * \brief Block and wait for all queued and running tasks to complete.
 * \memberof taskqueue
 *
 * \param q The task queue.
 */
void taskqueue_wait_for_complete(struct taskqueue *q);

/**
 * \brief Signal to the queue that a running task has completed.
 * \memberof taskqueue
 *
 * \param q The task queue.
 */
void taskqueue_task_complete(struct taskqueue *q);

/**
 * \brief Notify any blocked processes that the queue state has changed.
 * \memberof taskqueue
 *
 * Calling taskqueue_notify() wakes up any processes that are blocked in calls
 * to taskqueue_wait_for_work() or taskqueue_wait_for_complete(). These
 * processes then re-evaluate the queue state, going back to sleep, or returning
 * depending on the outcome.
 *
 * \param q The task queue.
 */
int taskqueue_notify(struct taskqueue *q);

/**
 * \brief Basic worker thread function for use with task queue.
 *
 * This function loops forever, consuming and executing tasks on the queue.
 * When no work is available, this function will block on
 * taskqueue_wait_for_work(). Call taskqueue_notify() to wake up all blocked
 * threads and resume task execution.
 *
 * \param qp Task queue on which worker thread operates (casted to void *)
 */
void *taskqueue_basic_worker_func(void *qp);

#endif // TASKQUEUE_H
