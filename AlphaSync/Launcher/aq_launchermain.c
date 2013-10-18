/** @file       aq_launchermain.c
 *  @brief      Launcher service library.
 *
 *  Copyright (c) 2008-2013, tSoniq. http://tsoniq.com
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  	*	Redistributions of source code must retain the above copyright notice, this list of
 *  	    conditions and the following disclaimer.
 *  	*	Redistributions in binary form must reproduce the above copyright notice, this list
 *  	    of conditions and the following disclaimer in the documentation and/or other materials
 *  	    provided with the distribution.
 *  	*	Neither the name of tSoniq nor the names of its contributors may be used to endorse
 *  	    or promote products derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <unistd.h>
#include <stdint.h>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <mach/mach.h>
#include <time.h>

#include "aq_launcherconfig.h"
#include "aq_launcherprivate.h"


/** Debug log macro.
 */
#define LOG(...)        do { fprintf(stderr, "AlphaSyncLauncher:: "); fprintf(stderr, __VA_ARGS__); } while (0)
#define LOG_ERROR(...)  do { fprintf(stderr, "AlphaSyncLauncher:: FAIL:: "); fprintf(stderr, __VA_ARGS__); } while (0)


/** Structure describing device matching criteria and an application to open on detection.
 */
struct AQLaunchControl
{
    CFStringRef m_applicationID;            /**< Application to launch (bundle ID). */
    SInt32 m_vendorID;                      /**< USB device's vendor ID, or -1 to match any vendor. */
    SInt32 m_productID;                     /**< USB device's product ID, or -1 to match any product. */
    struct AQLaunchControl* m_next;         /**< Next in list, or zero at end of list. */
};


/** Application global data.
 */
static struct
{
    IONotificationPortRef m_notifyPort;
    io_iterator_t m_deviceAddedIter;
    struct AQLaunchControl* m_control;
    int m_isStopped;
    int m_isPaused;
    double m_pauseTime;
    time_t m_pauseStartTime;

} globals;


/** Start the launcher.
 */
static void start()
{
    globals.m_isStopped = 0;
    globals.m_isPaused = 0;
}


/** Stop the launcher.
 */
static void stop()
{
    globals.m_isStopped = 1;
    globals.m_isPaused = 0;
}


/** Pause the launcher.
 */
static void pauseThenStart(float seconds)
{
    globals.m_isStopped = 0;
    globals.m_isPaused = 1;
    globals.m_pauseTime = (double)seconds;
    time(&globals.m_pauseStartTime);
}


/** Create a control record and add it to the list.
 *
 *  @param  app     The application path.
 *  @param  vendor  The USB vendor ID, or -1 for any vendor.
 *  @param  product The USB product ID, or -1 for any product.
 *  @return         The control structure, or zero if failed. The struction will be added
 *                  to the global list if successful.
 */
static struct AQLaunchControl* makeControl(CFStringRef app, SInt32 vendor, SInt32 product)
{
    struct AQLaunchControl* control = 0;
    if (0 != app)
    {
        control = (struct AQLaunchControl*)malloc(sizeof (struct AQLaunchControl));
        if (control)
        {
            CFRetain(app);
            control->m_applicationID = app;
            control->m_vendorID = vendor;
            control->m_productID = product;
            control->m_next = 0;
            if (0 == globals.m_control)
            {
                globals.m_control = control;
            }
            else
            {
                struct AQLaunchControl* c = globals.m_control;
                while (c->m_next) c = c->m_next;
                c->m_next = control;
            }
        }
    }
    return control;
}


/** Load preference data.
 */
static void loadPreferences()
{
    CFArrayRef launchList = CFPreferencesCopyAppValue(kPreferenceKeyLaunchList, kAQLauncherPreferenceID);
    if (0 != launchList && CFArrayGetTypeID() == CFGetTypeID(launchList))
    {
        int idx = 0;
        while (idx < CFArrayGetCount(launchList))
        {
            CFDictionaryRef dict = CFArrayGetValueAtIndex(launchList, idx);
            if (dict && CFDictionaryGetTypeID() == CFGetTypeID(dict))
            {
                CFStringRef app = CFDictionaryGetValue(dict, kPreferenceKeyAppID);
                CFNumberRef nvendor = CFDictionaryGetValue(dict, kPreferenceKeyUsbVendorID);
                CFNumberRef nproduct = CFDictionaryGetValue(dict, kPreferenceKeyUsbProductID);
                SInt32 vendor;
                SInt32 product;
                if (CFNumberGetValue(nvendor, kCFNumberSInt32Type, &vendor) &&
                    CFNumberGetValue(nproduct, kCFNumberSInt32Type, &product))
                {
                    makeControl(app, vendor, product);
                }
            }
            idx ++;
        }
    }
    if (0 != launchList) CFRelease(launchList);
}



/** Device added notification callback.
 */
static void deviceAdded(void* refCon, io_iterator_t iterator)
{
    (void)refCon;

    kern_return_t kr;
    io_service_t usbDevice;
    IOCFPlugInInterface **plugInInterface=NULL;
    IOUSBDeviceInterface **dev=NULL;
    HRESULT res;
    SInt32 score;
    UInt16 vendor;
    UInt16 product;

    if (globals.m_isStopped)
    {
        while ((usbDevice = IOIteratorNext(iterator))) IOObjectRelease(usbDevice);
        return;
    }

    if (globals.m_isPaused)
    {
        time_t timeNow;
        time(&timeNow);
        double elapsedTime = difftime(timeNow, globals.m_pauseStartTime);
        if (elapsedTime < globals.m_pauseTime)
        {
            while ((usbDevice = IOIteratorNext(iterator))) IOObjectRelease(usbDevice);
            return;      // still paused...
        }
        globals.m_isPaused = 0;
    }

    while ((usbDevice = IOIteratorNext(iterator)))
    {
        kr = IOCreatePlugInInterfaceForService(usbDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        IOObjectRelease(usbDevice);
        if ((kIOReturnSuccess != kr) || !plugInInterface)
        {
            LOG_ERROR("unable to create a plugin (%08x)\n", kr);
            continue;
        }

        // I have the device plugin, I need the device interface
        res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
        IODestroyPlugInInterface(plugInInterface);

        if (res || !dev)
        {
            LOG_ERROR("couldn't create a device interface (%08x)\n", (int) res);
            continue;
        }

        // REVIEW: check these kr values
        if (kIOReturnSuccess == (*dev)->GetDeviceVendor(dev, &vendor) &&
            kIOReturnSuccess == (*dev)->GetDeviceProduct(dev, &product))
        {
            struct AQLaunchControl* lc = globals.m_control;
            while (lc)
            {
                if ((-1 == lc->m_vendorID || vendor == lc->m_vendorID) &&
                    (-1 == lc->m_productID || product == lc->m_productID))
                {
                    FSRef appFSRef;
                    OSStatus oserr = LSFindApplicationForInfo(kLSUnknownCreator, lc->m_applicationID, NULL, &appFSRef, NULL);
                    if (!oserr) oserr = LSOpenFSRef(&appFSRef, NULL);
                    if (oserr) LOG_ERROR("Attempt to launch application for IDs %04x %04x failed with error %d\n", (int)lc->m_vendorID, (int)lc->m_productID, (int)oserr);
                }
                lc = lc->m_next;
            }
        }

        (*dev)->Release(dev);
    }
}


/** Signal trap handler. SIGUSR1 is trapped and used as a hint to re-read the preference data.
 */
static void signalHandler(int sigraised)
{
    if (SIGUSR1 == sigraised)
    {
        // Re-load preference data.
        loadPreferences();
    }
    else
    {
        // Exit.
        IONotificationPortDestroy(globals.m_notifyPort);
        if (globals.m_deviceAddedIter)
        {
            IOObjectRelease(globals.m_deviceAddedIter);
            globals.m_deviceAddedIter = 0;
        }

        _exit(0);   // exit(0) should not be called from a signal handler. Use _exit(0) instead
    }
}


/** IPC Message handler callback.
 */
static CFDataRef ipcMessageHandlerCopyReply(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void* info)
{
    (void)local;
    (void)data;
    (void)info;

    CFDataRef replyData = 0;

    switch (msgid)
    {
        case kAQLauncherControlMsgID_Version:
        {
            uint32_t version = kAQLauncherVersion;
            replyData = CFDataCreate(NULL, (UInt8*)&version, 4);
            break;
        }

        case kAQLauncherControlMsgID_LoadPreferences:
            loadPreferences();
            break;

        case kAQLauncherControlMsgID_Stop:
            stop();
            break;

        case kAQLauncherControlMsgID_Start:
            start();
            break;

        case kAQLauncherControlMsgID_PauseThenStart:
        {
            float seconds = 4.0;
            assert(CFDataGetLength(data) == sizeof (float));
            if (CFDataGetLength(data) == sizeof (float)) CFDataGetBytes(data, CFRangeMake(0, sizeof (float)), (UInt8*)&seconds);
            pauseThenStart(seconds);
            break;
        }

        case kAQLauncherControlMsgID_Quit:
            exit(0);    // REVIEW: is it safe to quit here???
            break;

        default:
            LOG_ERROR("Unrecognised IPC message, code %02x\n", (unsigned)msgid);
            break;
    }

    return replyData;
}


/** Main entry.
 */
int main (int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    mach_port_t masterPort;
    CFMutableDictionaryRef matchingDict;
    CFRunLoopSourceRef runLoopSource;
    kern_return_t kr;
    sig_t oldHandler;

    // Initialise global data.
    globals.m_notifyPort = 0;
    globals.m_deviceAddedIter = 0;
    globals.m_control = 0;
    globals.m_isStopped = 0;
    globals.m_isPaused = 0;
    globals.m_pauseTime = 0.0;

    // Set up a signal handler so we can clean up when we're interrupted from the command line
    // Otherwise we stay in our run loop forever.
    oldHandler = signal(SIGINT, signalHandler);
    if (oldHandler == SIG_ERR) LOG_ERROR("Could not establish new signal handler");

    // first create a master_port for my task
    kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (kr || !masterPort)
    {
        LOG_ERROR("Couldn't create a master IOKit Port(%08x)\n", kr);
        return -1;
    }

    // Set up the matching criteria for the devices we're interested in
    matchingDict = IOServiceMatching(kIOUSBDeviceClassName);    // Interested in instances of class IOUSBDevice and its subclasses
    if (!matchingDict)
    {
        LOG_ERROR("Can't create a USB matching dictionary\n");
        mach_port_deallocate(mach_task_self(), masterPort);
        return -1;
    }

    // Create a notification port and add its run loop event source to our run loop
    // This is how async notifications get set up. By using an empty dictionary we
    // get to see all devices that are connected.
    globals.m_notifyPort = IONotificationPortCreate(masterPort);
    runLoopSource = IONotificationPortGetRunLoopSource(globals.m_notifyPort);

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

    // Set up the notification to be called when a raw device is first matched by I/O Kit
    IOServiceAddMatchingNotification(globals.m_notifyPort,
                                     kIOFirstMatchNotification,
                                     matchingDict,
                                     deviceAdded,
                                     NULL,
                                     &globals.m_deviceAddedIter);

    // Create the mach port used to receive messages from client applications.
    CFMessagePortContext context = {0, NULL, NULL, NULL, NULL};
    Boolean shouldFreeInfo;
    CFMessagePortRef controlPort = CFMessagePortCreateLocal(kCFAllocatorDefault, kAQLauncherControlPort, ipcMessageHandlerCopyReply, &context, &shouldFreeInfo);
    CFRunLoopSourceRef rlSource = CFMessagePortCreateRunLoopSource(NULL, controlPort, 0);
    if (!controlPort || !rlSource)
    {
        LOG_ERROR("Failed to create control port");
        exit(1);
    }
    CFRunLoopAddSource(CFRunLoopGetCurrent(), rlSource, kCFRunLoopDefaultMode);
    CFRelease(rlSource);
    CFRelease(controlPort);


    // Parse preference data (leave this until the mach port is set up, so that
    // we do not miss change requests).
    loadPreferences();


    // Iterate once to get already-present devices and arm the notification
    deviceAdded(NULL, globals.m_deviceAddedIter);


    // Now done with the master_port
    mach_port_deallocate(mach_task_self(), masterPort);
    masterPort = 0;

    // Start the run loop. Now we'll receive notifications.
    CFRunLoopRun();

    // We should never get here
    return 0;
}
