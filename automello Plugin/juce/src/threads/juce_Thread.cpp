/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_Thread.h"
#include "juce_SpinLock.h"
#include "../core/juce_Time.h"
#include "../containers/juce_Array.h"


//==============================================================================
class RunningThreadsList
{
public:
    RunningThreadsList()
    {
    }

    ~RunningThreadsList()
    {
        // Some threads are still running! Make sure you stop all your
        // threads cleanly before your app quits!
        jassert (threads.size() == 0);
    }

    void add (Thread* const thread)
    {
        const SpinLock::ScopedLockType sl (lock);
        jassert (! threads.contains (thread));
        threads.add (thread);
    }

    void remove (Thread* const thread)
    {
        const SpinLock::ScopedLockType sl (lock);
        jassert (threads.contains (thread));
        threads.removeValue (thread);
    }

    int size() const noexcept
    {
        return threads.size();
    }

    Thread* getThreadWithID (const Thread::ThreadID targetID) const noexcept
    {
        const SpinLock::ScopedLockType sl (lock);

        for (int i = threads.size(); --i >= 0;)
        {
            Thread* const t = threads.getUnchecked(i);

            if (t->getThreadId() == targetID)
                return t;
        }

        return nullptr;
    }

    void stopAll (const int timeOutMilliseconds)
    {
        signalAllThreadsToStop();

        for (;;)
        {
            Thread* firstThread = getFirstThread();

            if (firstThread != nullptr)
                firstThread->stopThread (timeOutMilliseconds);
            else
                break;
        }
    }

    static RunningThreadsList& getInstance()
    {
        static RunningThreadsList runningThreads;
        return runningThreads;
    }

private:
    Array<Thread*> threads;
    SpinLock lock;

    void signalAllThreadsToStop()
    {
        const SpinLock::ScopedLockType sl (lock);

        for (int i = threads.size(); --i >= 0;)
            threads.getUnchecked(i)->signalThreadShouldExit();
    }

    Thread* getFirstThread() const
    {
        const SpinLock::ScopedLockType sl (lock);
        return threads.getFirst();
    }
};


//==============================================================================
void Thread::threadEntryPoint()
{
    RunningThreadsList::getInstance().add (this);

    JUCE_TRY
    {
        if (threadName_.isNotEmpty())
            setCurrentThreadName (threadName_);

        if (startSuspensionEvent_.wait (10000))
        {
            jassert (getCurrentThreadId() == threadId_);

            if (affinityMask_ != 0)
                setCurrentThreadAffinityMask (affinityMask_);

            run();
        }
    }
    JUCE_CATCH_ALL_ASSERT

    RunningThreadsList::getInstance().remove (this);
    closeThreadHandle();
}

// used to wrap the incoming call from the platform-specific code
void JUCE_API juce_threadEntryPoint (void* userData)
{
    static_cast <Thread*> (userData)->threadEntryPoint();
}


//==============================================================================
Thread::Thread (const String& threadName)
    : threadName_ (threadName),
      threadHandle_ (nullptr),
      threadId_ (0),
      threadPriority_ (5),
      affinityMask_ (0),
      threadShouldExit_ (false)
{
}

Thread::~Thread()
{
    /* If your thread class's destructor has been called without first stopping the thread, that
       means that this partially destructed object is still performing some work - and that's
       probably a Bad Thing!

       To avoid this type of nastiness, always make sure you call stopThread() before or during
       your subclass's destructor.
    */
    jassert (! isThreadRunning());

    stopThread (100);
}

//==============================================================================
void Thread::startThread()
{
    const ScopedLock sl (startStopLock);

    threadShouldExit_ = false;

    if (threadHandle_ == nullptr)
    {
        launchThread();
        setThreadPriority (threadHandle_, threadPriority_);
        startSuspensionEvent_.signal();
    }
}

void Thread::startThread (const int priority)
{
    const ScopedLock sl (startStopLock);

    if (threadHandle_ == nullptr)
    {
        threadPriority_ = priority;
        startThread();
    }
    else
    {
        setPriority (priority);
    }
}

bool Thread::isThreadRunning() const
{
    return threadHandle_ != nullptr;
}

//==============================================================================
void Thread::signalThreadShouldExit()
{
    threadShouldExit_ = true;
}

bool Thread::waitForThreadToExit (const int timeOutMilliseconds) const
{
    // Doh! So how exactly do you expect this thread to wait for itself to stop??
    jassert (getThreadId() != getCurrentThreadId());

    const int sleepMsPerIteration = 5;
    int count = timeOutMilliseconds / sleepMsPerIteration;

    while (isThreadRunning())
    {
        if (timeOutMilliseconds > 0 && --count < 0)
            return false;

        sleep (sleepMsPerIteration);
    }

    return true;
}

void Thread::stopThread (const int timeOutMilliseconds)
{
    // agh! You can't stop the thread that's calling this method! How on earth
    // would that work??
    jassert (getCurrentThreadId() != getThreadId());

    const ScopedLock sl (startStopLock);

    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();

        if (timeOutMilliseconds != 0)
            waitForThreadToExit (timeOutMilliseconds);

        if (isThreadRunning())
        {
            // very bad karma if this point is reached, as there are bound to be
            // locks and events left in silly states when a thread is killed by force..
            jassertfalse;
            Logger::writeToLog ("!! killing thread by force !!");

            killThread();

            RunningThreadsList::getInstance().remove (this);
            threadHandle_ = nullptr;
            threadId_ = 0;
        }
    }
}

//==============================================================================
bool Thread::setPriority (const int priority)
{
    const ScopedLock sl (startStopLock);

    if (setThreadPriority (threadHandle_, priority))
    {
        threadPriority_ = priority;
        return true;
    }

    return false;
}

bool Thread::setCurrentThreadPriority (const int priority)
{
    return setThreadPriority (0, priority);
}

void Thread::setAffinityMask (const uint32 affinityMask)
{
    affinityMask_ = affinityMask;
}

//==============================================================================
bool Thread::wait (const int timeOutMilliseconds) const
{
    return defaultEvent_.wait (timeOutMilliseconds);
}

void Thread::notify() const
{
    defaultEvent_.signal();
}

//==============================================================================
int Thread::getNumRunningThreads()
{
    return RunningThreadsList::getInstance().size();
}

Thread* Thread::getCurrentThread()
{
    return RunningThreadsList::getInstance().getThreadWithID (getCurrentThreadId());
}

void Thread::stopAllThreads (const int timeOutMilliseconds)
{
    RunningThreadsList::getInstance().stopAll (timeOutMilliseconds);
}

//==============================================================================
void SpinLock::enter() const noexcept
{
    if (! tryEnter())
    {
        for (int i = 20; --i >= 0;)
            if (tryEnter())
                return;

        while (! tryEnter())
            Thread::yield();
    }
}


END_JUCE_NAMESPACE
