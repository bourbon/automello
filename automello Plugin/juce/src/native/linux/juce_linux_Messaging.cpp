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

// (This file gets included by juce_linux_NativeCode.cpp, rather than being
// compiled on its own).
#if JUCE_INCLUDED_FILE

//==============================================================================
#if JUCE_DEBUG && ! defined (JUCE_DEBUG_XERRORS)
 #define JUCE_DEBUG_XERRORS 1
#endif

Display* display = nullptr;
Window juce_messageWindowHandle = None;
XContext windowHandleXContext;   // This is referenced from Windowing.cpp

extern void juce_windowMessageReceive (XEvent* event);  // Defined in Windowing.cpp
extern void juce_handleSelectionRequest (XSelectionRequestEvent &evt);  // Defined in Clipboard.cpp

//==============================================================================
ScopedXLock::ScopedXLock()       { XLockDisplay (display); }
ScopedXLock::~ScopedXLock()      { XUnlockDisplay (display); }

//==============================================================================
class InternalMessageQueue
{
public:
    InternalMessageQueue()
        : bytesInSocket (0),
          totalEventCount (0)
    {
        int ret = ::socketpair (AF_LOCAL, SOCK_STREAM, 0, fd);
        (void) ret; jassert (ret == 0);

        //setNonBlocking (fd[0]);
        //setNonBlocking (fd[1]);
    }

    ~InternalMessageQueue()
    {
        close (fd[0]);
        close (fd[1]);

        clearSingletonInstance();
    }

    //==============================================================================
    void postMessage (Message* msg)
    {
        const int maxBytesInSocketQueue = 128;

        ScopedLock sl (lock);
        queue.add (msg);

        if (bytesInSocket < maxBytesInSocketQueue)
        {
            ++bytesInSocket;

            ScopedUnlock ul (lock);
            const unsigned char x = 0xff;
            size_t bytesWritten = write (fd[0], &x, 1);
            (void) bytesWritten;
        }
    }

    bool isEmpty() const
    {
        ScopedLock sl (lock);
        return queue.size() == 0;
    }

    bool dispatchNextEvent()
    {
        // This alternates between giving priority to XEvents or internal messages,
        // to keep everything running smoothly..
        if ((++totalEventCount & 1) != 0)
            return dispatchNextXEvent() || dispatchNextInternalMessage();
        else
            return dispatchNextInternalMessage() || dispatchNextXEvent();
    }

    // Wait for an event (either XEvent, or an internal Message)
    bool sleepUntilEvent (const int timeoutMs)
    {
        if (! isEmpty())
            return true;

        if (display != 0)
        {
            ScopedXLock xlock;
            if (XPending (display))
                return true;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = timeoutMs * 1000;
        int fd0 = getWaitHandle();
        int fdmax = fd0;

        fd_set readset;
        FD_ZERO (&readset);
        FD_SET (fd0, &readset);

        if (display != 0)
        {
            ScopedXLock xlock;
            int fd1 = XConnectionNumber (display);
            FD_SET (fd1, &readset);
            fdmax = jmax (fd0, fd1);
        }

        const int ret = select (fdmax + 1, &readset, 0, 0, &tv);
        return (ret > 0); // ret <= 0 if error or timeout
    }

    //==============================================================================
    struct MessageThreadFuncCall
    {
        enum { uniqueID = 0x73774623 };

        MessageCallbackFunction* func;
        void* parameter;
        void* result;
        CriticalSection lock;
        WaitableEvent event;
    };

    //==============================================================================
    juce_DeclareSingleton_SingleThreaded_Minimal (InternalMessageQueue);

private:
    CriticalSection lock;
    ReferenceCountedArray <Message> queue;
    int fd[2];
    int bytesInSocket;
    int totalEventCount;

    int getWaitHandle() const noexcept      { return fd[1]; }

    static bool setNonBlocking (int handle)
    {
        int socketFlags = fcntl (handle, F_GETFL, 0);
        if (socketFlags == -1)
            return false;

        socketFlags |= O_NONBLOCK;
        return fcntl (handle, F_SETFL, socketFlags) == 0;
    }

    static bool dispatchNextXEvent()
    {
        if (display == 0)
            return false;

        XEvent evt;

        {
            ScopedXLock xlock;
            if (! XPending (display))
                return false;

            XNextEvent (display, &evt);
        }

        if (evt.type == SelectionRequest && evt.xany.window == juce_messageWindowHandle)
            juce_handleSelectionRequest (evt.xselectionrequest);
        else if (evt.xany.window != juce_messageWindowHandle)
            juce_windowMessageReceive (&evt);

        return true;
    }

    Message::Ptr popNextMessage()
    {
        const ScopedLock sl (lock);

        if (bytesInSocket > 0)
        {
            --bytesInSocket;

            const ScopedUnlock ul (lock);
            unsigned char x;
            size_t numBytes = read (fd[1], &x, 1);
            (void) numBytes;
        }

        return queue.removeAndReturn (0);
    }

    bool dispatchNextInternalMessage()
    {
        const Message::Ptr msg (popNextMessage());

        if (msg == nullptr)
            return false;

        if (msg->intParameter1 == MessageThreadFuncCall::uniqueID)
        {
            // Handle callback message
            MessageThreadFuncCall* const call = (MessageThreadFuncCall*) msg->pointerParameter;

            call->result = (*(call->func)) (call->parameter);
            call->event.signal();
        }
        else
        {
            // Handle "normal" messages
            MessageManager::getInstance()->deliverMessage (msg);
        }

        return true;
    }
};

juce_ImplementSingleton_SingleThreaded (InternalMessageQueue);


//==============================================================================
namespace LinuxErrorHandling
{
    //==============================================================================
    static bool errorOccurred = false;
    static bool keyboardBreakOccurred = false;
    static XErrorHandler oldErrorHandler = (XErrorHandler) 0;
    static XIOErrorHandler oldIOErrorHandler = (XIOErrorHandler) 0;

    //==============================================================================
    // Usually happens when client-server connection is broken
    static int ioErrorHandler (Display* display)
    {
        DBG ("ERROR: connection to X server broken.. terminating.");

        if (JUCEApplication::isStandaloneApp())
            MessageManager::getInstance()->stopDispatchLoop();

        errorOccurred = true;
        return 0;
    }

    // A protocol error has occurred
    static int juce_XErrorHandler (Display* display, XErrorEvent* event)
    {
       #if JUCE_DEBUG_XERRORS
        char errorStr[64] = { 0 };
        char requestStr[64] = { 0 };

        XGetErrorText (display, event->error_code, errorStr, 64);
        XGetErrorDatabaseText (display, "XRequest", String (event->request_code).toUTF8(), "Unknown", requestStr, 64);
        DBG ("ERROR: X returned " + String (errorStr) + " for operation " + String (requestStr));
       #endif

        return 0;
    }

    static void installXErrorHandlers()
    {
        oldIOErrorHandler = XSetIOErrorHandler (ioErrorHandler);
        oldErrorHandler = XSetErrorHandler (juce_XErrorHandler);
    }

    static void removeXErrorHandlers()
    {
        if (JUCEApplication::isStandaloneApp())
        {
            XSetIOErrorHandler (oldIOErrorHandler);
            oldIOErrorHandler = 0;

            XSetErrorHandler (oldErrorHandler);
            oldErrorHandler = 0;
        }
    }

    //==============================================================================
    static void keyboardBreakSignalHandler (int sig)
    {
        if (sig == SIGINT)
            keyboardBreakOccurred = true;
    }

    static void installKeyboardBreakHandler()
    {
        struct sigaction saction;
        sigset_t maskSet;
        sigemptyset (&maskSet);
        saction.sa_handler = keyboardBreakSignalHandler;
        saction.sa_mask = maskSet;
        saction.sa_flags = 0;
        sigaction (SIGINT, &saction, 0);
    }
}

//==============================================================================
void MessageManager::doPlatformSpecificInitialisation()
{
    if (JUCEApplication::isStandaloneApp())
    {
        // Initialise xlib for multiple thread support
        static bool initThreadCalled = false;

        if (! initThreadCalled)
        {
            if (! XInitThreads())
            {
                // This is fatal!  Print error and closedown
                Logger::outputDebugString ("Failed to initialise xlib thread support.");
                Process::terminate();
                return;
            }

            initThreadCalled = true;
        }

        LinuxErrorHandling::installXErrorHandlers();
        LinuxErrorHandling::installKeyboardBreakHandler();
    }

    // Create the internal message queue
    InternalMessageQueue::getInstance();

    // Try to connect to a display
    String displayName (getenv ("DISPLAY"));
    if (displayName.isEmpty())
        displayName = ":0.0";

    display = XOpenDisplay (displayName.toUTF8());

    if (display != 0)  // This is not fatal! we can run headless.
    {
        // Create a context to store user data associated with Windows we create in WindowDriver
        windowHandleXContext = XUniqueContext();

        // We're only interested in client messages for this window, which are always sent
        XSetWindowAttributes swa;
        swa.event_mask = NoEventMask;

        // Create our message window (this will never be mapped)
        const int screen = DefaultScreen (display);
        juce_messageWindowHandle = XCreateWindow (display, RootWindow (display, screen),
                                                  0, 0, 1, 1, 0, 0, InputOnly,
                                                  DefaultVisual (display, screen),
                                                  CWEventMask, &swa);
    }
}

void MessageManager::doPlatformSpecificShutdown()
{
    InternalMessageQueue::deleteInstance();

    if (display != 0 && ! LinuxErrorHandling::errorOccurred)
    {
        XDestroyWindow (display, juce_messageWindowHandle);
        XCloseDisplay (display);

        juce_messageWindowHandle = 0;
        display = nullptr;

        LinuxErrorHandling::removeXErrorHandlers();
    }
}

bool MessageManager::postMessageToSystemQueue (Message* message)
{
    if (LinuxErrorHandling::errorOccurred)
        return false;

    InternalMessageQueue::getInstanceWithoutCreating()->postMessage (message);
    return true;
}

void MessageManager::broadcastMessage (const String& value)
{
    /* TODO */
}


//==============================================================================
class AsyncFunctionCaller   : public AsyncUpdater
{
public:
    static void* call (MessageCallbackFunction* func_, void* parameter_)
    {
        if (MessageManager::getInstance()->isThisTheMessageThread())
            return func_ (parameter_);

        AsyncFunctionCaller caller (func_, parameter_);
        caller.triggerAsyncUpdate();
        caller.finished.wait();
        return caller.result;
    }

    void handleAsyncUpdate()
    {
        result = (*func) (parameter);
        finished.signal();
    }

private:
    WaitableEvent finished;
    MessageCallbackFunction* func;
    void* parameter;
    void* volatile result;

    AsyncFunctionCaller (MessageCallbackFunction* func_, void* parameter_)
        : result (0), func (func_), parameter (parameter_)
    {}

    JUCE_DECLARE_NON_COPYABLE (AsyncFunctionCaller);
};

void* MessageManager::callFunctionOnMessageThread (MessageCallbackFunction* func, void* parameter)
{
    if (LinuxErrorHandling::errorOccurred)
        return nullptr;

    return AsyncFunctionCaller::call (func, parameter);
}

// this function expects that it will NEVER be called simultaneously for two concurrent threads
bool MessageManager::dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages)
{
    while (! LinuxErrorHandling::errorOccurred)
    {
        if (LinuxErrorHandling::keyboardBreakOccurred)
        {
            LinuxErrorHandling::errorOccurred = true;

            if (JUCEApplication::isStandaloneApp())
                Process::terminate();

            break;
        }

        if (InternalMessageQueue::getInstanceWithoutCreating()->dispatchNextEvent())
            return true;

        if (returnIfNoPendingMessages)
            break;

        InternalMessageQueue::getInstanceWithoutCreating()->sleepUntilEvent (2000);
    }

    return false;
}

#endif
