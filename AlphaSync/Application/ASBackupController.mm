/** @file   ASBackupController.mm
 *  @brief  Manage backups
 *
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

#import "ASBackupController.h"
#import "ASDeviceNode.h"
#import "ASAppletNode.h"
#import "ASFileNode.h"
#import "ASPreferenceController.h"
#import "AQFileUtilities.h"


using namespace ts;


#define kDeviceBackupMaxBackupsInOneDay (999)       /**< Don't allow more than this many backups in one day. */


#pragma mark    ----------  ASBackupItem  -----------


/** Class used to describe a backup action.
 */
@interface ASBackupItem : NSObject
{
    BOOL newDevice;         /**< Logical true if this is the first file for a new device. */
    ASFileNode* fileNode;
    NSString* appletName;
    NSString* deviceName;
}
- (id)initWithNewDevice:(BOOL)nd fileNode:(ASFileNode*)fn appletName:(NSString*)an deviceName:(NSString*)dn;
- (BOOL)newDevice;
- (ASFileNode*)fileNode;
- (NSString*)appletName;
- (NSString*)deviceName;
@end

@implementation ASBackupItem
- (id)initWithNewDevice:(BOOL)nd fileNode:(ASFileNode*)fn appletName:(NSString*)an deviceName:(NSString*)dn
{
    if ((self = [super init]))
    {
        newDevice = nd;
        fileNode = [fn retain];
        appletName = [an retain];
        deviceName = [dn retain];
    }
    return self;
}
- (void)dealloc
{
    [fileNode release];
    [appletName release];
    [deviceName release];
    [super dealloc];
}
- (BOOL)newDevice { return newDevice; }
- (ASFileNode*)fileNode { return fileNode; }
- (NSString*)appletName { return appletName; }
- (NSString*)deviceName { return deviceName; }

@end



#pragma mark    ----------  ASBackupController  -----------

@implementation ASBackupController


- (id)init
{
    if ((self = [super initWithWindowNibName:@"Backup"]))
    {
        // Nothing
    }
    return self;
}


/** Make a new device session path and create the necessary directories.
 *
 *  @param  deviceName  The device name, or nil to omit this from the backup path.
 *  @return             Logical TRUE if succeeded.
 */
- (BOOL)makeNewDeviceSessionPath:(NSString*)deviceName
{
    [currentSessionPath release];
    currentSessionPath = nil;

    /* Find a unique path string, formed from <root-path>/<device-name>/<date> and optionally appending an index
     * number to make the path unique. The <root-path> is specified in the user preferences.
     */
    NSString* rootPath = [[NSUserDefaults standardUserDefaults] stringForKey:kASPreferenceKeyBackupFolder];
    if (!rootPath) return FALSE;

    NSString* dateName = [[NSDate date] descriptionWithCalendarFormat:@"%Y-%m-%d" timeZone:nil locale:[[NSUserDefaults standardUserDefaults] dictionaryRepresentation]];


    if (nil != deviceName) rootPath = [rootPath stringByAppendingPathComponent:deviceName];

    NSString* path = [rootPath stringByAppendingPathComponent:dateName];

    NSFileManager* fileManager = [NSFileManager defaultManager];
    int uniqueID = 1;
    while ([fileManager fileExistsAtPath:path])
    {
        if (uniqueID > kDeviceBackupMaxBackupsInOneDay) return FALSE;
        NSString* uniqueSuffix = [NSString stringWithFormat:@"%@ (%d)", dateName, uniqueID];
        path = [rootPath stringByAppendingPathComponent:uniqueSuffix];
        uniqueID ++;
    }


    /* Create the path.
     */
    if (!aq_FileUtilitiesCreateDirectoriesForPath(path)) return FALSE;

    currentSessionPath = [path retain];
    return TRUE;
}


/** Initialise a backup.
 */
- (void)initBackup:(ASNode*)root
{
    assert(!backupItems);

    backupItems = [[NSMutableArray arrayWithCapacity:32] retain];
    nextBackupItem = 0;
    bytesToBackup = 0;
    bytesSoFar = 0;

    for (NSUInteger i = 0; i != [root numberOfChildren]; i++)
    {
        ASDeviceNode* deviceNode = (ASDeviceNode*)[root childAtIndex:i];
        BOOL newDevice = TRUE;
        for (NSUInteger j = 0; j != [deviceNode numberOfChildren]; j++)
        {
            ASAppletNode* appletNode = (ASAppletNode*)[deviceNode childAtIndex:j];
            for (NSUInteger k = 0; k != [appletNode numberOfChildren]; k++)
            {
                ASFileNode* fileNode = (ASFileNode*)[appletNode childAtIndex:k];
                ASBackupItem* backupItem = [[[ASBackupItem alloc] initWithNewDevice:newDevice
                                                                  fileNode:fileNode
                                                                  appletName:[appletNode displayName]
                                                                  deviceName:[deviceNode displayName]] autorelease];
                [backupItems addObject:backupItem];
                newDevice = FALSE;
                bytesToBackup += [fileNode fileSize];
            }
        }
    }

    [outletFilesRemaining setStringValue:[NSString stringWithFormat:@"Backing up %ld items", (unsigned long)[backupItems count]]];
    [outletBackupItem setStringValue:@""];
    [outletProgressIndicator setDoubleValue:0.0];
    [outletProgressIndicator startAnimation:self];
    [outletStopDoneButton setTitle:@"Stop"];
    [outletStopDoneButton setKeyEquivalent:@"\033"];  // use escape to stop
}


/** Stop the backup running.
 */
- (void)stopBackup:(NSString*)message
{
    [backupItems release];
    backupItems = nil;
    nextBackupItem = 0;
    bytesToBackup = 0;
    bytesSoFar = 0;

    [outletFilesRemaining setStringValue:message];
    [outletBackupItem setStringValue:@""];
    [outletProgressIndicator stopAnimation:self];
    [outletStopDoneButton setTitle:@"OK"];
    [outletStopDoneButton setKeyEquivalent:@"\r"];  // use return to dismiss
    [backupTimer invalidate];
    backupTimer = nil;
}


/** Backup one item.
 */
- (void)backupOneItem
{
    assert(backupItems);

    if (nextBackupItem >= [backupItems count])
    {
        [outletProgressIndicator setDoubleValue:100.0];
        [self stopBackup:@"Backup complete"];
    }
    else
    {
        ASBackupItem* backupItem = [backupItems objectAtIndex:nextBackupItem];
        ASFileNode* fileNode = [backupItem fileNode];
        NSString* appletName = [backupItem appletName];
        NSString* deviceName = [backupItem deviceName];

        [outletFilesRemaining setStringValue:[NSString stringWithFormat:@"Backing up %d items",(int)([backupItems count] - nextBackupItem)]];
        [outletBackupItem setStringValue:[NSString stringWithFormat:@"%@: %@: %@", deviceName, appletName, [fileNode displayName]]];

        if ([backupItem newDevice]) [self makeNewDeviceSessionPath:(NSString*)deviceName];
        if (!currentSessionPath)
        {
            NSLog(@"Unable to create backup folder for file %@/%@/%@", deviceName, appletName, [fileNode displayName]);
        }
        else
        {
            NSString* path = [currentSessionPath stringByAppendingPathComponent:appletName];
            if (!aq_FileUtilitiesCreateDirectoriesForPath(path) || ![fileNode saveUnderPath:path])
            {
                NSLog(@"Error backing up file %@/%@/%@", deviceName, appletName, [fileNode displayName]);
            }
        }

        bytesSoFar += [fileNode fileSize];
        nextBackupItem ++;

        [outletProgressIndicator setDoubleValue:double(bytesSoFar)/double(bytesToBackup)*100.0];
    }
}


- (IBAction)actionStopDone:(id)sender
{
    NSWindow* sheet = [self window];

    if (backupTimer)
    {
        // Backup still running
        [self stopBackup:@"Cancelled"];
    }
    else
    {
        // Backup stopped
        [sheet orderOut:sender];
        [NSApp endSheet:sheet returnCode:1];
    }
}


- (void)sheetDidEnd:(NSWindow*)sheet returnCode:(int)returnCode contextInfo:(void*)contextInfo
{
    (void)sheet;
    (void)returnCode;
    (void)contextInfo;
    // Nothing
}


- (void)runBackupFor:(ASNode*)root modalForWindow:(NSWindow*)window
{
    [outletProgressIndicator setUsesThreadedAnimation:YES];

    NSWindow* sheet = [self window];
    NSApplication* app = [NSApplication sharedApplication];

    [app beginSheet:sheet
         modalForWindow:window
         modalDelegate:self
         didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:)
         contextInfo:self];

    [self initBackup:root];
    backupTimer = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(backupOneItem) userInfo:nil repeats:YES];
}


@end
