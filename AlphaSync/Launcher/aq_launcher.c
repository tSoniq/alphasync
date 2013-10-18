/** @file       aq_launcher.c
 *  @brief      Launcher service library.
 *
 *  Copyright (c) 2008-2013, tSoniq.
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
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>
#include "aq_launcher.h"
#include "aq_launcherconfig.h"
#include "aq_launcherprivate.h"


#pragma mark    ----------  Definitions  ----------


/* Debug log macros.
 */
#define LOG(...)        do { fprintf(stderr, "Launcher:: %s: ", __FUNCTION__); fprintf(stderr, __VA_ARGS__); } while (0)
#define LOG_ERROR(...)  do { fprintf(stderr, "Launcher:: FAIL:: %s: ", __FUNCTION__); fprintf(stderr, __VA_ARGS__); } while (0)


/** The session handle.
 */
struct AQLauncher
{
    CFStringRef m_path;                         /**< Application path for this session. */
    int m_flushNeeded;                          /**< Set non-zero if a call to flush is required before final close. */
    struct AQLaunchItem* m_items;               /**< List of launch items. */
    struct AQLaunchItem* m_nextToEnumerate;     /**< Next item in enumeration. */
    CFMessagePortRef m_controlPort;             /**< Mach message port used for client-server communication. */
    FSRef m_pathFsRef;                          /**< FSRef for m_path. */
    UInt8 m_pathBuffer[PATH_MAX+1];             /**< The path in file system form. */
};


/** Structure describing device matching criteria and an application to open on detection.
 */
struct AQLaunchItem
{
    struct AQLaunchItem* m_next;        /**< Next item in list. */
    CFStringRef m_applicationID;        /**< Application to launch. */
    SInt32 m_vendorID;                  /**< USB device's vendor ID, or -1 to match any vendor. */
    SInt32 m_productID;                 /**< USB device's product ID, or -1 to match any product. */
};



/* Forward declarations.
 */
static int aq_launcherServerConnect(AQLauncherRef lh);
static void aq_launcherServerDisconnect(AQLauncherRef lh);
static uint32_t aq_launcherServerGetVersion(AQLauncherRef lh);
static void aq_launcherServerLoadPreferences(AQLauncherRef lh);
static void aq_launcherServerQuit(AQLauncherRef lh);



#pragma mark    ----------  Server Control  ----------



/** Connect to the server application message port. On return the session handle's m_controlPort handle
 *  will have been initialised or set to zero if no client is running.
 *
 *  @param  lh      The session handle.
 *  @return         Logical true if a connection was established.
 */
static int aq_launcherServerConnect(AQLauncherRef lh)
{
    if (!lh->m_controlPort)
    {
        lh->m_controlPort = CFMessagePortCreateRemote(NULL, kAQLauncherControlPort);
        if (lh->m_controlPort)
        {
            uint32_t version = aq_launcherServerGetVersion(lh);
            if ((version & 0xff00) != (kAQLauncherVersion & 0xff00))
            {
                LOG_ERROR("Incompatible client-server versions: want %04x got %04x\n", kAQLauncherVersion, version);
                aq_launcherServerDisconnect(lh);
            }
            else
            {
                // connected
            }
        }
    }

    return (0 != lh->m_controlPort);
}


/** Disconnect from the server.
 *
 *  @param  lh      The session handle.
 */
static void aq_launcherServerDisconnect(AQLauncherRef lh)
{
    if (0 != lh->m_controlPort)
    {
        CFRelease(lh->m_controlPort);
        lh->m_controlPort = 0;
        // disconnected
    }
}



/** Get the server version number.
 *
 *  @param  lh      The session handle.
 *  @return         The server version, or zero if no server is available.
 */
static uint32_t aq_launcherServerGetVersion(AQLauncherRef lh)
{
    uint32_t version = 0;

    if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
    if (0 != lh->m_controlPort)
    {
        CFDataRef replyData = 0;
        SInt32 status = CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_Version, (CFDataRef)NULL, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
        if (kCFMessagePortSuccess == status)
        {
            assert(CFDataGetLength(replyData) == 4);
            if (CFDataGetLength(replyData) == 4) CFDataGetBytes(replyData, CFRangeMake(0, 4), (UInt8*)&version);
            CFRelease(replyData);
        }
    }
    return version;
}


/** Start the server.
 *
 *  @param  lh      The session handle.
 */
static void aq_launcherServerStart(AQLauncherRef lh)
{
    CFDataRef replyData = 0;
    if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
    if (0 != lh->m_controlPort) CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_Start, (CFDataRef)NULL, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
    if (replyData) CFRelease(replyData);
}


/** Stop the server.
 *
 *  @param  lh      The session handle.
 */
static void aq_launcherServerStop(AQLauncherRef lh)
{
    CFDataRef replyData = 0;
    if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
    if (0 != lh->m_controlPort) CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_Stop, (CFDataRef)NULL, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
}


/** Pause the server.
 *
 *  @param  lh      The session handle.
 *  @param  seconds The number of seconds to pause.
 */
static void aq_launcherServerPause(AQLauncherRef lh, float seconds)
{
    CFDataRef replyData = 0;
    CFDataRef data = CFDataCreate(kCFAllocatorDefault, (UInt8*)&seconds, sizeof seconds);
    assert(data);
    if (data)
    {
        if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
        if (0 != lh->m_controlPort) CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_PauseThenStart, data, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
        if (replyData) CFRelease(replyData);
        CFRelease(data);
    }
}


/** Quit the server.
 *
 *  @param  lh      The session handle.
 */
static void aq_launcherServerQuit(AQLauncherRef lh)
{
    CFDataRef replyData = 0;
    if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
    if (0 != lh->m_controlPort) CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_Quit, (CFDataRef)NULL, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
    if (replyData) CFRelease(replyData);
}


/** Ask the server to reload preference data.
 *
 *  @param  lh      The session handle.
 */
static void aq_launcherServerLoadPreferences(AQLauncherRef lh)
{
    CFDataRef replyData = 0;
    if (0 == lh->m_controlPort) aq_launcherServerConnect(lh);
    if (0 != lh->m_controlPort) CFMessagePortSendRequest(lh->m_controlPort, kAQLauncherControlMsgID_LoadPreferences, (CFDataRef)NULL, 1.0, 1.0, kCFRunLoopDefaultMode, &replyData);
    if (replyData) CFRelease(replyData);
}





#pragma mark    ----------  Local Functions  ----------


/** Find an existing item.
 *
 *  @param  app     The application bundle ID.
 *  @param  vendor  The USB vendor ID, or -1 for any vendor.
 *  @param  product The USB product ID, or -1 for any product.
 *  @return         The item reference or zero if not found.
 */
static struct AQLaunchItem* aq_launcherFindItem(AQLauncherRef lh, CFStringRef app, SInt32 vendor, SInt32 product)
{
    struct AQLaunchItem* item = lh->m_items;
    while (item)
    {
        if ((kCFCompareEqualTo == CFStringCompare(item->m_applicationID, app, 0)) &&
            (vendor == item->m_vendorID) &&
            (product == item->m_productID))
        {
            break;
        }
        item = item->m_next;
    }
    return item;
}


/** Create a new launch item.
 *
 *  @param  lh          The session handle.
 *  @param  app         The application bundle ID.
 *  @param  vendorID    The USB vendor ID.
 *  @param  productID   The USB product ID.
 *  @return             Non-zero if the list was changed.
 */
static int aq_launcherAddItem(AQLauncherRef lh, CFStringRef app, SInt32 vendorID, SInt32 productID)
{
    int changed = 0;
    struct AQLaunchItem* item = aq_launcherFindItem(lh, app, vendorID, productID);
    if (0 == item)
    {
        item = (struct AQLaunchItem*)malloc(sizeof (struct AQLaunchItem));
        if (item)
        {
            CFRetain(app);
            item->m_applicationID = app;
            item->m_vendorID = vendorID;
            item->m_productID = productID;
            item->m_next = 0;

            if (0 == lh->m_items)
            {
                lh->m_items = item;
            }
            else
            {
                struct AQLaunchItem* l = lh->m_items;
                while (l->m_next) l = l->m_next;
                l->m_next = item;
            }

            changed = 1;
        }
    }

    return changed;
}


/** Delete a specified item on the launch list.
 *
 *  @param  lh      The session handle.
 *  @param  item    The item to delete.
 *  @return         Non-zero if the list was changed.
 */
static int aq_launcherDeleteItem(AQLauncherRef lh, AQLaunchItemRef item)
{
    int changed = 0;

    struct AQLaunchItem* prev = 0;
    struct AQLaunchItem* search = lh->m_items;
    while (search && search != item)
    {
        prev = search;
        search = search->m_next;
    }

    if (search)
    {
        if (!prev) lh->m_items = search->m_next;
        else prev->m_next = search->m_next;

        if (lh->m_nextToEnumerate == search) lh->m_nextToEnumerate = search->m_next;

        CFRelease(search->m_applicationID);
        search->m_applicationID = 0;
        search->m_vendorID = 0;
        search->m_productID = 0;
        free(search);

        changed = 1;
    }

    return changed;
}


/** Delete all items on the launch list.
 *
 *  @param  lh      The session handle.
 *  @return         Non-zero if the list was changed.
 */
static int aq_launcherDeleteAllItems(AQLauncherRef lh)
{
    int changed = (0 != lh->m_items);

    struct AQLaunchItem* item = lh->m_items;
    while (item)
    {
        struct AQLaunchItem* next = item->m_next;
        CFRelease(item->m_applicationID);
        free(item);
        item = next;
    }

    lh->m_items = 0;
    lh->m_nextToEnumerate = 0;

    return changed;
}



/** Release the preference data. This only deletes the local copy - the information
 *  saved to disk is left unchanged.
 *
 *  @param  lh          The session handle.
 */
static void aq_launcherReleasePreferences(AQLauncherRef lh)
{
    struct AQLaunchItem* item = (struct AQLaunchItem*)aq_LauncherGetFirstLaunchItem(lh);
    while (item)
    {
        aq_LauncherDeleteLaunchItem(lh, item);
        item = (struct AQLaunchItem*)aq_LauncherGetNextLaunchItem(lh);
    }
}


/** Load the preference data.
 *
 *  @param  lh          The session handle.
 */
static void aq_launcherLoadPreferences(AQLauncherRef lh)
{
    aq_launcherReleasePreferences(lh);

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
                    aq_launcherAddItem(lh, app, vendor, product);
                }
            }
            idx ++;
        }
    }
    if (0 != launchList) CFRelease(launchList);
}


/** Save the preference data.
 *
 *  @param  lh          The session handle.
 */
static void aq_launcherSavePreferences(AQLauncherRef lh)
{
    CFMutableArrayRef launchList = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    assert(launchList);
    if (!launchList) return;

    struct AQLaunchItem* item = lh->m_items;
    while (item)
    {
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        assert(dict);
        if (!dict) return;

        CFNumberRef vendorID = CFNumberCreate(NULL, kCFNumberSInt32Type, &item->m_vendorID);
        CFNumberRef productID = CFNumberCreate(NULL, kCFNumberSInt32Type, &item->m_productID);

        CFDictionaryAddValue(dict, kPreferenceKeyAppID, item->m_applicationID);
        CFDictionaryAddValue(dict, kPreferenceKeyUsbVendorID, vendorID);
        CFDictionaryAddValue(dict, kPreferenceKeyUsbProductID, productID);

        CFRelease(vendorID);
        CFRelease(productID);

        CFArrayAppendValue(launchList, dict);

        CFRelease(dict);

        item = item->m_next;
    }

    CFPreferencesSetAppValue(kPreferenceKeyLaunchList, launchList, kAQLauncherPreferenceID);
    CFPreferencesAppSynchronize(kAQLauncherPreferenceID);

    CFRelease(launchList);
}



#pragma mark    ----------  Exported API  ----------



AQLauncherRef aq_LauncherOpen(CFStringRef path)
{
    AQLauncherRef lh = (AQLauncherRef)malloc(sizeof *lh);
    if (lh)
    {
        memset(lh, 0, sizeof *lh);

        lh->m_path = CFRetain(path);
        lh->m_flushNeeded = 0;
        lh->m_items = 0;
        lh->m_nextToEnumerate = 0;
        lh->m_controlPort = 0;

        aq_launcherLoadPreferences(lh);

        if (lh->m_path)
        {
            // Get an FSREF to use with open(m_path). This verifies that the target application exists.
            OSStatus oserr = 0;
            lh->m_pathBuffer[0] = 0;
            if (!CFStringGetFileSystemRepresentation(lh->m_path, (char*)lh->m_pathBuffer, sizeof lh->m_pathBuffer) ||
                ((oserr = FSPathMakeRef(lh->m_pathBuffer, &lh->m_pathFsRef, NULL))) )
            {
                LOG_ERROR("Invalid path to launcher: %d: %s\n", (int)oserr, lh->m_pathBuffer);
                aq_LauncherClose(lh);
                lh = 0;
            }
        }
    }
    return lh;
}


void aq_LauncherClose(AQLauncherRef lh)
{
    if (0 != lh)
    {
        if (lh->m_flushNeeded) aq_LauncherFlushLaunchItems(lh);
        lh->m_flushNeeded = 0;

        aq_launcherServerDisconnect(lh);

        if (0 != lh->m_path) CFRelease(lh->m_path);
        aq_launcherDeleteAllItems(lh);

        free(lh);
    }
}


int aq_LauncherIsInstalled(AQLauncherRef lh)
{
    int result = 0;

    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
    if (loginItems)
    {
        UInt32 seedValue = 0;
        CFArrayRef loginItemsArray = LSSharedFileListCopySnapshot(loginItems, &seedValue);
        if (loginItemsArray)
        {
            for (CFIndex index = 0; index !=  CFArrayGetCount(loginItemsArray); ++index)
            {
                CFURLRef url = 0;
                LSSharedFileListItemRef item = (LSSharedFileListItemRef) CFArrayGetValueAtIndex(loginItemsArray, index);
                if (item && 0 == LSSharedFileListItemResolve(item, 0, &url, NULL))
                {
                    CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
                    if (path)
                    {
                        if (kCFCompareEqualTo == CFStringCompare(path, lh->m_path, 0))
                        {
                            // Found a matching dictionary entry. If necessary, kick the launcher to get it running (eg: recover from a crash).
                            result = 1;
                            if (!aq_launcherServerConnect(lh)) LSOpenFSRef(&lh->m_pathFsRef, NULL);
                            CFRelease(path);
                            break;
                        }
                        CFRelease(path);
                    }
                }
            }
            CFRelease(loginItemsArray);
        }
        CFRelease(loginItems);
    }

    return result;
}


int aq_LauncherInstall(AQLauncherRef lh)
{
    if (0 == lh->m_path) return -1;     // Can't install unless the client nominated a launcher application path

    /* Flush preferences before starting the launcher. This ensures that the launcher is started
     * with the most recent preference data.
     */
    aq_LauncherFlushLaunchItems(lh);

    int result = 0;

    if (!aq_LauncherIsInstalled(lh))
    {
        CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, lh->m_path, kCFURLPOSIXPathStyle, TRUE);
        if (url)
        {
            LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
            if (loginItems)
            {
                LSSharedFileListItemRef item = LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemLast, NULL, NULL, url, NULL, NULL);
                if (item)
                {
                    CFRelease(item);
                    result = 1;
                }
                CFRelease(loginItems);
            }
            CFRelease(url);
        }
    }

    return result;
}


extern void aq_LauncherUninstall(AQLauncherRef lh)
{
    aq_launcherServerQuit(lh);
    aq_launcherServerDisconnect(lh);

    if (0 == lh->m_path) return;

    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
    if (loginItems)
    {
        UInt32 seedValue = 0;
        CFArrayRef loginItemsArray = LSSharedFileListCopySnapshot(loginItems, &seedValue);
        if (loginItemsArray)
        {
            for (CFIndex index = 0; index !=  CFArrayGetCount(loginItemsArray); ++index)
            {
                CFURLRef url = 0;
                LSSharedFileListItemRef item = (LSSharedFileListItemRef) CFArrayGetValueAtIndex(loginItemsArray, index);
                if (item && 0 == LSSharedFileListItemResolve(item, 0, &url, NULL))
                {
                    CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
                    if (path)
                    {
                        if (kCFCompareEqualTo == CFStringCompare(path, lh->m_path, 0))
                        {
                            // Found a matching dictionary entry. Delete it.
                            LSSharedFileListItemRemove(loginItems, item);
                            CFRelease(path);
                            break;
                        }
                        CFRelease(path);
                    }
                }
            }
            CFRelease(loginItemsArray);
        }
        CFRelease(loginItems);
    }
}


AQLaunchItemRef aq_LauncherGetFirstLaunchItem(AQLauncherRef lh)
{
    if (lh->m_items)
    {
        lh->m_nextToEnumerate = lh->m_items->m_next;
        return lh->m_items;
    }
    else
    {
        lh->m_nextToEnumerate = 0;
        return 0;
    }
}


AQLaunchItemRef aq_LauncherGetNextLaunchItem(AQLauncherRef lh)
{
    if (lh->m_nextToEnumerate)
    {
        const struct AQLaunchItem* result = lh->m_nextToEnumerate;
        lh->m_nextToEnumerate = result->m_next;
        return result;
    }
    else
    {
        return 0;
    }
}


void aq_LauncherClearLaunchItems(AQLauncherRef lh)
{
    lh->m_flushNeeded = aq_launcherDeleteAllItems(lh) || lh->m_flushNeeded;
}


void aq_LauncherDeleteLaunchItem(AQLauncherRef lh, AQLaunchItemRef item)
{
    lh->m_flushNeeded = aq_launcherDeleteItem(lh, item) || lh->m_flushNeeded;
}


void aq_LauncherAddLaunchItem(AQLauncherRef lh, CFStringRef app, SInt32 vendorID, SInt32 productID)
{
    lh->m_flushNeeded = aq_launcherAddItem(lh, app, vendorID, productID) || lh->m_flushNeeded;
}


void aq_LauncherFlushLaunchItems(AQLauncherRef lh)
{
    aq_launcherSavePreferences(lh);         // write the updates to disk
    aq_launcherServerLoadPreferences(lh);   // if the launcher is running, tell it to reload its preferences
    lh->m_flushNeeded = 0;
}


void aq_LauncherStart(AQLauncherRef lh)
{
    aq_launcherServerStart(lh);
}


void aq_LauncherStop(AQLauncherRef lh)
{
    aq_launcherServerStop(lh);
}


void aq_LauncherPause(AQLauncherRef lh, float seconds)
{
    aq_launcherServerPause(lh, seconds);
}
