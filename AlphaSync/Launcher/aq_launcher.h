/** @file       aq_launcher.h
 *  @brief      Application interface to the launcher application.
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
 *
 *  The launcher API is intended to be used with an auxillary application, typically bundled in the main
 *  application's resource directory. The auxillary application can be installed to run at login and
 *  can be configured to start the main application when a particular USB device is connected.
 *
 *  The basic API methods are:
 *
 *      aq_LauncherOpen()       --  Open a new session and check that the specified launcher application exists
 *      aq_LauncherClose()      --  Close the session
 *
 *      aq_LauncherIsInstalled()    --  Check if the launcher is currently installed
 *      aq_LauncherInstall()        --  Install the launcher in the login items and start it running
 *      aq_LauncherUninstall()      --  Remove the launcher from the login items and stop any existing instance running
 *
 *      aq_LauncherGetFirstLaunchItem() --  Enumeration (first item)
 *      aq_LauncherGetNextLaunchItem()  --  Enumeration (next items)
 *      aq_LauncherAddLaunchItem()      --  Add an item to launch on connection of a device
 *      aq_LauncherClearLaunchItems()   --  Clear all launch items
 *      aq_LauncherDeleteLaunchItem()   --  Remove a launch item
 *      aq_LauncherFlushLaunchItems()   --  Flush changes to the launch items and notify the server if it is running
 *
 *  The code uses a passive approach to the server and will not start the launcher application unless it is installed
 *  and is not already running.
 */
#ifndef COM_TSONIQ_aq_launcher_h
#define COM_TSONIQ_aq_launcher_h    (1)

#include <stdint.h>
#include <assert.h>
#include <CoreFoundation/CoreFoundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Launcher session handle.
 */
typedef struct AQLauncher* AQLauncherRef;


/** Handle describing device matching criteria and an application to open on detection.
 */
typedef const struct AQLaunchItem* AQLaunchItemRef;


/** Open a new session.
 *
 *  @param  path        The path to the launcher application. This must be an absolute path. The open will fail
 *                      if the launcher application can not be accessed.
 *  @return             A session handle, or zero if failed.
 */
extern AQLauncherRef aq_LauncherOpen(CFStringRef path);


/** Close a session.
 *
 *  @param  lh          The session handle.
 */
extern void aq_LauncherClose(AQLauncherRef lh);


/** Check if the launcher application is installed (set in the login auto-run list).
 *
 *  @return             Zero if the launcher is not installed, +1 if it is and -1 if the status
 *                      can not be determined (for example, if the launcher path was not supplied
 *                      at open).
 */
int aq_LauncherIsInstalled(AQLauncherRef lh);


/** Install the launcher application. This will cause the launcher to start automatically at login.
 *  The install affects only the current user. The launcher application is automatically started
 *  on installation if it is not already executing.
 *
 *  @param  path        The path to the launcher application. This must be an absolute path string.
 *  @return             Non-zero if the launcher was installed correctly.
 */
extern int aq_LauncherInstall(AQLauncherRef lh);


/** Uninstall the launcher application. The launcher is removed from the login list and any running
 *  instance is terminated. If the launcher was not previously installed this function does nothing.
 *
 *  @param  path        The path to the launcher application (as supplied to @e aq_LauncherInstallLanucher()).
 */
extern void aq_LauncherUninstall(AQLauncherRef lh);


/** Enumerator for launch items.
 *
 *  @param  lh          The session handle.
 *  @return             The first item, or zero if none are present.
 */
extern AQLaunchItemRef aq_LauncherGetFirstLaunchItem(AQLauncherRef lh);


/** Enumerator for launch items.
 *
 *  @param  lh          The session handle.
 *  @return             The next item, or zero if no more are present.
 */
extern AQLaunchItemRef aq_LauncherGetNextLaunchItem(AQLauncherRef lh);


/** Delete all launch items. The change will not be written to disk or notified to a running launcher
 *  until either aq_LauncherClose() or aq_LauncherFlush() is called.
 *
 *  @param  lh          The session handle.
 *  @param  item        The item to delete (as returned from the enumeration functions).
 */
extern void aq_LauncherClearLaunchItems(AQLauncherRef lh);


/** Delete a launch item. The change will not be written to disk or notified to a running launcher
 *  until either aq_LauncherClose() or aq_LauncherFlush() is called.
 *
 *  @param  lh          The session handle.
 *  @param  item        The item to delete (as returned from the enumeration functions).
 */
extern void aq_LauncherDeleteLaunchItem(AQLauncherRef lh, AQLaunchItemRef item);


/** Add a new preference item. The change will not be written to disk or notified to a running launcher
 *  until either aq_LauncherClose() or aq_LauncherFlush() is called.
 *
 *  @param  lh          The session handle.
 *  @param  app         The application's bundle ID.
 *  @param  vendorID    The USB vendor ID.
 *  @param  productID   The USB product ID.
 */
extern void aq_LauncherAddLaunchItem(AQLauncherRef lh, CFStringRef app, SInt32 m_vendorID, SInt32 m_productID);


/** Flush launch items to disk and notify a running server (if any).
 *
 *  @param  lh          The session handle.
 */
extern void aq_LauncherFlushLaunchItems(AQLauncherRef lh);


/** Enable launch processing events.
 *
 *  @param  lh          The session handle.
 */
extern void aq_LauncherStart(AQLauncherRef lh);


/** Disable launch processing events. Note that this setting is temporary - the launcher
 *  application will automatically start processing if it is restarted.
 *
 *  @param  lh          The session handle.
 */
extern void aq_LauncherStop(AQLauncherRef lh);


/** Pause launching for a specified time. Launch processing (device plug/unplug events) will
 *  be ignored for the specified interval, after which an implicit aq_LauncherStart() call is
 *  made. Explicit calls to aq_LauncherStart() or aq_LauncherStop() cancel any pending pause
 *  request.
 *
 *  @param  lh          The session handle.
 *  @param  seconds     The number of seconds to pause for.
 */
extern void aq_LauncherPause(AQLauncherRef lh, float seconds);


#ifdef __cplusplus
}
#endif

#endif  //COM_TSONIQ_aq_launcher_h
