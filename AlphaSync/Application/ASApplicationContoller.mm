/** @file   ASApplicationContoller.mm
 *  @brief  Application controller class. This is responsible for creating and managing the device
 *          driver and user interface.
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

#import "ASApplicationContoller.h"
#import "ASInspectorWindowController.h"
#import "ASFileCell.h"
#import "AQFileUtilities.h"
#import "aq_launcher.h"
#import "aq_launcherconfig.h"

using namespace ts;

#pragma mark    Forward Declarations

/** The launcher application name.
 */
#define kASLauncherName         @"AlphaSyncLauncher"
#define kASLauncherExtension    @"app"

/** The application's bundle ID. This is used by the launcher to start the application
 *  when a device is connected. It must match the value in the applications info.plist file.
 */
#define kASApplicationBundleID      @"com.tsoniq.AlphaSync"


/** The name of the application support folder.
 */
#define kASApplicationSupportFolder @"~/Library/Application Support/AlphaSync"


/** Enumeration used to select the colour of the status indicator light.
 */
enum ASAlertStatus
{
    kASAlertStatusGreen,        /**< Device is idle or functioning correctly. */
    kASAlertStatusYellow,       /**< Device is connecting and functioning correctly. */
    kASAlertStatusRed           /**< Last operation returned an error status. */
};

@interface ASApplicationController(PrivateAPI)
- (void)uninstall;
- (void)installApplicationSupport;
- (void)deviceControllerNotification:(NSNotification*)notification;
- (void)displayMemoryStatus;
- (void)displayStatusTimerDone;
- (void)setStatusBusy:(NSString*)actionName;
- (void)setStatusBusy:(NSString*)actionName timeout:(float)seconds alertStatus:(ASAlertStatus)alertStatus;
- (void)setStatusError:(NSString*)errorName;
- (void)setStatusIdle;
- (void)setStatusImmediateIdle;
- (void)setupToolbar;
@end


static NSString* kToolbarIdentifierMainWindow   = @"Toolbar:MainWindow";
static NSString* kToolbarItemIdentifierInfo     = @"ToolbarItem:Info";
static NSString* kToolbarItemIdentifierBackup   = @"ToolbarItem:Backup";
static NSString* kToolbarItemIdentifierDelete   = @"ToolbarItem:Delete";

static const NSInteger kMenuItemTagBackup   = 100;


@implementation ASApplicationController


/** Class initialisation. This is called once before any objects are instantiated by the class.
 *  It is used here to initialise the user defaults.
 */
+ (void)initialize
{
    NSMutableDictionary* defaults = [NSMutableDictionary dictionary];

    NSNumber* defaultFilter = [NSNumber numberWithInt:kASPreferenceDefaultFilter];
    NSString* defaultBackupFolder = [kASPreferenceDefaultBackupFolder stringByExpandingTildeInPath];

    [defaults setObject:defaultFilter forKey:kASPreferenceKeyFilter];
    [defaults setObject:defaultBackupFolder forKey:kASPreferenceKeyBackupFolder];

    [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];
}




- (id)init
{
    if ((self = [super init]))
    {
        appIsUninstalling = FALSE;
    }
    return self;
}


- (void)dealloc
{
    [preferenceController release];
    [deviceController release];
    [statusDisplayTimer invalidate];
    [launcherInstallPath release];
    [super dealloc];
}


- (void)awakeFromNib
{
    [outletMainWindow setDelegate:self];
//    [NSApp setDelegate:self];

    NSBundle* bundle = [NSBundle mainBundle];
    iconDisconnected = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-status-clear" ofType:@"pdf"]];
    iconRed = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-status-red" ofType:@"pdf"]];
    iconYellow = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-status-yellow" ofType:@"pdf"]];
    iconGreen = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-status-green" ofType:@"pdf"]];
    iconToolbarInfo = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-toolbar-info" ofType:@"pdf"]];
    iconToolbarBackup = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-toolbar-backup" ofType:@"pdf"]];
    iconToolbarDeleteDisabled = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-toolbar-delete-disabled" ofType:@"pdf"]];
    iconToolbarDeleteFile = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-toolbar-delete-single" ofType:@"pdf"]];
    iconToolbarDeleteFiles = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-toolbar-delete-multiple" ofType:@"pdf"]];

    [outletStatusImage setImageFrameStyle:NSImageFrameNone];
    [outletStatusImage setImageAlignment:NSImageAlignCenter];
    [outletStatusImage setImageScaling:NSScaleProportionally];

    [outletProgressIndicator stopAnimation:self];
    [outletProgressIndicator setUsesThreadedAnimation:YES];
    [self setStatusImmediateIdle];

    [self setupToolbar];

    [self installApplicationSupport];
    [self setupAutoRun:0.0];

    // Setup custom cell for the file name and icon view.
    NSTableColumn *tc = [outletFileView tableColumnWithIdentifier:@"Name"];
    ASFileCell *fs = [[[ASFileCell alloc] init] autorelease];
    [fs setEditable:YES];
    [tc setDataCell:fs];

    // Setup drag and drop.
    [outletFileView registerForDraggedTypes:[NSArray arrayWithObjects:NSStringPboardType, NSFilenamesPboardType, nil]];
    [outletFileView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [outletFileView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:NO];
    [outletFileView setAutoresizesOutlineColumn:NO];

    deviceController = [[ASDeviceController alloc] init];
    if (!deviceController)
    {
        // REVIEW: display error dialogue and exit
        NSLog(@"Failed to initialise device controller");
    }
    [[deviceController systemRoot] setDelegate:self];     // This object will receive ASDeviceAccessProtocol delgate events

    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(deviceControllerNotification:) name:kASDeviceControllerDeviceDetected object:nil];
    [nc addObserver:self selector:@selector(deviceControllerNotification:) name:kASDeviceControllerDeviceConnected object:nil];
    [nc addObserver:self selector:@selector(deviceControllerNotification:) name:kASDeviceControllerDeviceDisconnected object:nil];

    dataSource = [[ASNodeDataSource alloc] init];
    [dataSource setDeviceController:deviceController];
    [outletFileView setDataSource:dataSource];
    [outletFileView setDelegate:self];

    [deviceController enable];      // this may trigger an immediate notification (eg for device connect)
    [dataSource reloadData:outletFileView];

    [self displayMemoryStatus];
}



#pragma Device Controller Notifications



/** Handle notifications from the device controller.
 */
- (void)deviceControllerNotification:(NSNotification*)notification
{
    NSString* name = [notification name];
    if (NSOrderedSame == [name compare:kASDeviceControllerDeviceDetected])
    {
        [self setStatusBusy:@"Connecting" timeout:10.0 alertStatus:kASAlertStatusYellow];
    }
    else if (NSOrderedSame == [name compare:kASDeviceControllerDeviceConnected])
    {
        [self setStatusImmediateIdle];
        [dataSource reloadData:outletFileView];
    }
    else if (NSOrderedSame == [name compare:kASDeviceControllerDeviceDisconnected])
    {
        [self setStatusIdle];
        [dataSource reloadData:outletFileView];
    }
    else
    {
        NSLog(@"ASApplicationController: ignoring unknown device message: %@", name);
    }

    [toolbar validateVisibleItems];
}


#pragma mark -
#pragma mark Inspector

- (void)updateInspectorDisplay
{
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:16];
    NSIndexSet* selectedRows = [outletFileView selectedRowIndexes];
    NSUInteger row = [selectedRows firstIndex];
    while (NSNotFound != row)
    {
        id item = [outletFileView itemAtRow:(NSInteger)row];
        [array addObject:item];
        row = [selectedRows indexGreaterThanIndex:row];
    }

    [inspectorController setDisplay:array];
}


#pragma mark -
#pragma mark Actions


- (IBAction)actionDelete:(id)sender
{
    (void)sender;

    if ([outletFileView numberOfSelectedRows] == 1)
    {
        id selectedItem = [outletFileView itemAtRow:[outletFileView selectedRow]];
        if ([selectedItem conformsToProtocol:@protocol(ASDeleteProtocol)])
        {
            [selectedItem deleteSelf];      // Delete the item (will invoke deviceWillDelete delegate, if appropriate).

            ASAppletNode* modifiedApplet = [selectedItem ancestorOfClass:[ASAppletNode class]];
            if (modifiedApplet) [modifiedApplet reload];
        }

        [outletFileView reloadData];
    }
}


- (IBAction)actionInfo:(id)sender
{
    (void)sender;

    if (! inspectorController) inspectorController = [[ASInspectorWindowController alloc] init];
    [self updateInspectorDisplay];
    [inspectorController showWindow:self];
}


- (IBAction)actionBackup:(id)sender
{
    (void)sender;

    if (!backupController) backupController = [[ASBackupController alloc] init];
    [backupController runBackupFor:[deviceController systemRoot] modalForWindow:outletMainWindow];
}


- (IBAction)actionOpenBackupFolder:(id)sender
{
    (void)sender;

    NSString* path = [[NSUserDefaults standardUserDefaults] stringForKey:kASPreferenceKeyBackupFolder];
	[[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
}



- (IBAction)actionShowPreferences:(id)sender
{
    (void)sender;

    if (!preferenceController) preferenceController = [[ASPreferenceController alloc] initWithAppController:self];
    [preferenceController showWindow:self];
}


- (IBAction)actionUninstall:(id)sender
{
    (void)sender;
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [[alert addButtonWithTitle:@"Cancel"] setKeyEquivalent:@"\033"];
    [[alert addButtonWithTitle:@"Uninstall"] setKeyEquivalent:@"\r"];
    [alert setMessageText:@"Uninstall AlphaSync?"];
    [alert setInformativeText:@"The application and supporting files will be moved to the trash."];

    int button = [alert runModal];
    BOOL shouldProceed = (button == NSAlertSecondButtonReturn);
    [alert release];

    if (shouldProceed) [self uninstall];
}


- (IBAction)reloadDeviceData:(id)sender
{
    (void)sender;
    [outletFileView deselectAll:self];
    [deviceController reloadAllDevices];
    [dataSource reloadData:outletFileView];

    [self displayMemoryStatus];
    [self updateInspectorDisplay];
}


#pragma mark -
#pragma mark Status bar handling


/** Update the available memory display.
 */
- (void)displayMemoryStatus
{
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:16];
    NSIndexSet* selectedRows = [outletFileView selectedRowIndexes];
    NSUInteger row = [selectedRows firstIndex];
    while (NSNotFound != row)
    {
        id item = [[outletFileView itemAtRow:(NSInteger)row] ancestorOfClass:[ASDeviceNode class]];;
        if (!item) break;
        [array addObject:item];
        row = [selectedRows indexGreaterThanIndex:row];
    }

    NSUInteger count = [array count];
    if (0 == count)
    {
        [outletAvailableMemory setStringValue:@""];
    }
    else if (1 == count)
    {
        ASDeviceNode* deviceNode = [array objectAtIndex:0];
        double availableRam = double([deviceNode availableRam]) / 1024.0;
        [outletAvailableMemory setStringValue:[NSString stringWithFormat:@"%3.1lf kbytes available", availableRam]];
    }
    else
    {
        [outletAvailableMemory setStringValue:@"multiple devices selected"];
    }
}


/** Callback on expiry of the status display timer. Flip back to idle.
 */
- (void)displayStatusTimerDone
{
    [statusDisplayTimer invalidate];
    statusDisplayTimer = nil;
    [self setStatusIdle];
}


/** Set busy status. The status will be maintained for a few seconds or until an explicit change is invoked.
 */
- (void)setStatusBusy:(NSString*)actionName
{
    [self setStatusBusy:actionName timeout:2.0 alertStatus:kASAlertStatusGreen];
}


/** Set busy status. The status will be maintained for the specified time or
 *  until one of setStatusIdle or setStatusBusy is invoked.
 */
- (void)setStatusBusy:(NSString*)actionName timeout:(float)seconds alertStatus:(ASAlertStatus)alertStatus
{
    NSImage* icon;

    switch (alertStatus)
    {
        case kASAlertStatusRed:
            icon = iconRed;
            [outletProgressIndicator stopAnimation:self];
            [outletProgressIndicator setHidden:YES];
            break;

        case kASAlertStatusYellow:
            icon = iconYellow;
            [outletProgressIndicator startAnimation:self];
            [outletProgressIndicator setHidden:NO];
            break;

        case kASAlertStatusGreen:
        default:
            [outletProgressIndicator startAnimation:self];
            [outletProgressIndicator setHidden:NO];
            if ([[deviceController systemRoot] numberOfChildren] > 0) icon = iconGreen;
            else icon = iconDisconnected;

    }

    [outletStatusImage setImage:icon];
    [outletStatusText setStringValue:actionName];

    [outletStatusImage display];
    [outletStatusText display];
    [outletProgressIndicator display];

    if (nil != statusDisplayTimer) [statusDisplayTimer invalidate];
    statusDisplayTimer = [NSTimer scheduledTimerWithTimeInterval:seconds target:self selector:@selector(displayStatusTimerDone) userInfo:nil repeats:NO];
}


/** Set an error status. The status will be maintained for a few seconds or until an explicit change is invoked.
 */
- (void)setStatusError:(NSString*)errorName
{
    [self setStatusBusy:errorName timeout:8.0 alertStatus:kASAlertStatusRed];
}


/** Set idle status. The status is updated immediately.
 */
- (void)setStatusIdle
{
    [outletProgressIndicator stopAnimation:self];
    [outletProgressIndicator setHidden:YES];

    if (nil == statusDisplayTimer) [self setStatusImmediateIdle];
    // else wait for the last display timer to fire and call us later
}


/** Set idle status. The status is updated immediately and the display timer (if any) is canceled.
 */
- (void)setStatusImmediateIdle
{
    if (nil != statusDisplayTimer)
    {
        [statusDisplayTimer invalidate];
        statusDisplayTimer = nil;
    }


    NSString* statusName;
    NSImage* statusIcon;

    NSUInteger deviceCount = [[deviceController systemRoot] numberOfChildren];

    if (deviceCount == 0)
    {
        statusName = @"Disconnected";
        statusIcon = iconDisconnected;
    }
    else if (deviceCount == 1)
    {
        statusName = @"Connected";
        statusIcon = iconGreen;
    }
    else
    {
        statusName = [NSString stringWithFormat:@"Connected to %u devices", (unsigned)deviceCount];
        statusIcon = iconGreen;
    }

    [outletStatusText setStringValue:statusName];
    [outletStatusImage setImage:statusIcon];
    [outletProgressIndicator stopAnimation:self];
    [outletProgressIndicator setHidden:YES];

    [outletStatusText display];
    [outletStatusImage display];
    [outletProgressIndicator display];
}



#pragma mark -
#pragma mark Toolbar handling

- (void)setupToolbar
{
    toolbar = [[[NSToolbar alloc] initWithIdentifier: kToolbarIdentifierMainWindow] autorelease];

    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDelegate:self];
    [outletMainWindow setToolbar:toolbar];
}



- (NSToolbarItem*)toolbar:(NSToolbar*)tbar itemForItemIdentifier:(NSString*)itemIdent willBeInsertedIntoToolbar:(BOOL)willBeInserted
{
    (void)tbar;
    (void)willBeInserted;

    NSToolbarItem *toolbarItem = nil;

    if ([itemIdent isEqual:kToolbarItemIdentifierInfo])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier:itemIdent] autorelease];

        [toolbarItem setLabel:@"Info"];
        [toolbarItem setPaletteLabel:@"Info"];
        [toolbarItem setToolTip:@"Show inspector"];
        [toolbarItem setImage:iconToolbarInfo];
        [toolbarItem setTarget:self];
        [toolbarItem setAction:@selector(actionInfo:)];
    }
    else if ([itemIdent isEqual:kToolbarItemIdentifierBackup])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier:itemIdent] autorelease];

        [toolbarItem setLabel:@"Backup"];
        [toolbarItem setPaletteLabel:@"Backup"];
        [toolbarItem setToolTip:@"Backup all files"];
        [toolbarItem setImage:iconToolbarBackup];
        [toolbarItem setTarget:self];
        [toolbarItem setAction:@selector(actionBackup:)];
    }
    else if ([itemIdent isEqual:kToolbarItemIdentifierDelete])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier:itemIdent] autorelease];

        [toolbarItem setLabel:@"Clear"];
        [toolbarItem setPaletteLabel:@"Clear"];
        [toolbarItem setToolTip:@"Clear a single file, or all files belonging to an applet"];
        [toolbarItem setImage:iconToolbarDeleteFiles];
        [toolbarItem setTarget:self];
        [toolbarItem setAction:@selector(actionDelete:)];
    }

    return toolbarItem;
}


- (NSArray*)toolbarDefaultItemIdentifiers:(NSToolbar*)tbar
{
    (void)tbar;

    // Required delegate method:  Returns the ordered list of items to be shown in the toolbar by default
    // If during the toolbar's initialization, no overriding values are found in the user defaults, or if the
    // user chooses to revert to the default items this set will be used
    return [NSArray arrayWithObjects:
            kToolbarItemIdentifierBackup,
            NSToolbarFlexibleSpaceItemIdentifier,
            kToolbarItemIdentifierDelete,
            NSToolbarSpaceItemIdentifier,
            kToolbarItemIdentifierInfo,
            nil];
}


- (NSArray*)toolbarAllowedItemIdentifiers:(NSToolbar*)tbar
{
    (void)tbar;

    // Required delegate method:  Returns the list of all allowed items by identifier.  By default, the toolbar
    // does not assume any items are allowed, even the separator.  So, every allowed item must be explicitly listed
    // The set of allowed items is used to construct the customization palette
    return [NSArray arrayWithObjects:
            kToolbarItemIdentifierBackup,
            kToolbarItemIdentifierInfo,
            kToolbarItemIdentifierDelete,
            NSToolbarSpaceItemIdentifier,
            NSToolbarCustomizeToolbarItemIdentifier,
            NSToolbarFlexibleSpaceItemIdentifier,
            NSToolbarSpaceItemIdentifier,
            NSToolbarSeparatorItemIdentifier,
            nil];
}


- (BOOL)validateToolbarItem:(NSToolbarItem*)toolbarItem
{
    BOOL enable = NO;

    if ([[toolbarItem itemIdentifier] isEqual:kToolbarItemIdentifierInfo])
    {
        enable = YES;
    }
    else if ([[toolbarItem itemIdentifier] isEqual:kToolbarItemIdentifierDelete])
    {
        // Only enable the delete option if there is only one item selected and that item supports the delete protocol
        if ([outletFileView numberOfSelectedRows] == 1)
        {
            id selectedItem = [outletFileView itemAtRow:[outletFileView selectedRow]];
            enable = [selectedItem conformsToProtocol:@protocol(ASDeleteProtocol)] && [selectedItem deletePermitted];
            if ([selectedItem isMemberOfClass:[ASFileNode class]])
            {
                [toolbarItem setToolTip:@"Clear the contents of a single file"];
                [toolbarItem setImage:iconToolbarDeleteFile];
            }
            else if ([selectedItem isMemberOfClass:[ASAppletNode class]])
            {
                [toolbarItem setToolTip:@"Clear all files belonging to an applet"];
                [toolbarItem setImage:iconToolbarDeleteFiles];
            }
            else
            {
                [toolbarItem setToolTip:@"Clear the thing that is selected"];
                [toolbarItem setImage:iconToolbarDeleteFile];
            }
        }
        if (!enable)
        {
            [toolbarItem setToolTip:@"Clear the contents of one or more files"];
            [toolbarItem setImage:iconToolbarDeleteDisabled];
        }
    }
    else if ([[toolbarItem itemIdentifier] isEqual:kToolbarItemIdentifierBackup])
    {
        // Enable backup only if and when at least one device is connected
        if (0 != [[deviceController systemRoot] numberOfChildren]) enable = true;
    }

    return enable;
}



#pragma mark -
#pragma mark Menu handling


- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    BOOL enable = TRUE;
    if ([menuItem tag] == kMenuItemTagBackup)
    {
        // Enable backup only if and when at least one device is connected
        if (0 == [[deviceController systemRoot] numberOfChildren]) enable = FALSE;
    }
    return enable;
}




#pragma mark -
#pragma mark ASDeviceAccessProtocol


- (BOOL)deviceWillSave:(ASNode*)source to:(NSString*)path sender:(id)sender
{
    (void)source;

    NSString* statusText = [NSString stringWithFormat:@"Saving %@ to %@", [sender displayName], [path lastPathComponent]];
    [self setStatusBusy:statusText];
    return YES;
}


- (BOOL)deviceWillLoad:(NSString*)path sender:(id)sender
{
    (void)sender;

    NSString* statusText = [NSString stringWithFormat:@"Loading %@", [path lastPathComponent]];
    [self setStatusBusy:statusText];
    return YES;
}


- (BOOL)deviceWillReplace:(ASNode*)target with:(NSString*)path sender:(id)sender
{
    (void)path;
    (void)sender;

    NSAlert* alert = [[NSAlert alloc] init];
    [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
    [[alert addButtonWithTitle:@"Cancel"] setKeyEquivalent:@"\033"];
    [alert setMessageText:[NSString stringWithFormat:@"Overwrite file %@?", [target displayName]]];
    [alert setInformativeText:@"This operation can not be undone."];
    [alert setAlertStyle:NSWarningAlertStyle];

    int button = [alert runModal];
    BOOL shouldProceed = (button == NSAlertFirstButtonReturn);
    [alert release];

    if (shouldProceed)
    {
        NSString* statusText = [NSString stringWithFormat:@"Loading %@", [target displayName]];
        [self setStatusBusy:statusText];
    }
    return shouldProceed;
}



- (BOOL)deviceWillDelete:(ASNode*)target sender:(id)sender
{
    (void)sender;

    NSAlert* alert = [[NSAlert alloc] init];
    [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
    [[alert addButtonWithTitle:@"Cancel"] setKeyEquivalent:@"\033"];

    if ([target isMemberOfClass:[ASFileNode class]])
    {
        [alert setMessageText:[NSString stringWithFormat:@"Clear file %@?", [target displayName]]];
        [alert setInformativeText:@"This operation can not be undone."];
        [alert setAlertStyle:NSWarningAlertStyle];
    }
    else if ([target isMemberOfClass:[ASAppletNode class]])
    {
        [alert setMessageText:[NSString stringWithFormat:@"Clear all files for %@?", [target displayName]]];
        [alert setInformativeText:@"This operation can not be undone."];
        [alert setAlertStyle:NSCriticalAlertStyle];
    }
    else
    {
        [alert setMessageText:[NSString stringWithFormat:@"Clear %@?", [target displayName]]];
        [alert setInformativeText:@"This operation can not be undone."];
        [alert setAlertStyle:NSWarningAlertStyle];
    }

    int button = [alert runModal];
    BOOL shouldProceed = (button == NSAlertFirstButtonReturn);
    [alert release];

    if (shouldProceed)
    {
        NSString* statusText = [NSString stringWithFormat:@"Deleting %@", [target displayName]];
        [self setStatusBusy:statusText];
    }
    return shouldProceed;
}


- (void)deviceDidLoad:(NSString*)path error:(NSString*)error sender:(id)sender
{
    (void)path;
    (void)sender;

    if (!error) [self setStatusIdle];
    else
    {
        [self setStatusError:error];
        NSAlert* alert = [[NSAlert alloc] init];
        [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
        [alert setMessageText:[NSString stringWithFormat:@"Failed to load %@", [path lastPathComponent]]];
        [alert setInformativeText:error];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        [alert release];
    }

    [self displayMemoryStatus];
    [self updateInspectorDisplay];
    [outletFileView reloadData];
}


- (void)deviceDidSave:(ASNode*)source error:(NSString*)error sender:(id)sender
{
    (void)source;
    (void)sender;

    if (!error) [self setStatusIdle];
    else
    {
        [self setStatusError:error];
        NSAlert* alert = [[NSAlert alloc] init];
        [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
        [alert setMessageText:[NSString stringWithFormat:@"Failed to save %@", [source displayName]]];
        [alert setInformativeText:error];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        [alert release];
    }

    [self displayMemoryStatus];
    [self updateInspectorDisplay];
    [outletFileView reloadData];
}


- (void)deviceDidReplace:(ASNode*)target error:(NSString*)error sender:(id)sender
{
    (void)target;
    (void)sender;

    if (!error) [self setStatusIdle];
    else
    {
        [self setStatusError:error];
        NSAlert* alert = [[NSAlert alloc] init];
        [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
        [alert setMessageText:[NSString stringWithFormat:@"Failed to replace file %@", [target displayName]]];
        [alert setInformativeText:error];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        [alert release];
    }


    [self displayMemoryStatus];
    [self updateInspectorDisplay];
    [outletFileView reloadData];
}


- (void)deviceDidDelete:(ASNode*)node error:(NSString*)error sender:(id)sender
{
    (void)node;
    (void)sender;

    if (!error) [self setStatusIdle];
    else
    {
        [self setStatusError:error];
        NSAlert* alert = [[NSAlert alloc] init];
        [[alert addButtonWithTitle:@"OK"] setKeyEquivalent:@"\r"];
        [alert setMessageText:[NSString stringWithFormat:@"Failed to delete file %@", [node displayName]]];
        [alert setInformativeText:error];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
        [alert release];
    }

    [self displayMemoryStatus];
    [self updateInspectorDisplay];
    [outletFileView reloadData];
}





#pragma mark -
#pragma mark NSOutlineView Delegate Methods



// Maintain enable state for toolbar and menu items as the displayed selection changes.
- (void)outlineViewSelectionDidChange:(NSNotification*)notification
{
    (void)notification;
    [toolbar validateVisibleItems];
    [self displayMemoryStatus];
    [self updateInspectorDisplay];
}


- (BOOL)outlineView:(NSOutlineView*)outlineView shouldEditTableColumn:(NSTableColumn*)tableColumn item:(id)item
{
    (void)outlineView;
    (void)tableColumn;
    if ([item isMemberOfClass:[ASFileNode class]]) return YES;
    else return NO;         // REVIEW: may allow editing of the device name in future
}


/** Delagate method used to specify the icon to show next to the file name.
 */
- (void)outlineView:(NSOutlineView*)outlineView willDisplayCell:(NSCell*)cell forTableColumn:(NSTableColumn*)tableColumn item:(id)item
{
    (void)outlineView;
    if ([[tableColumn identifier] isEqualToString:@"Name"])
    {
        ASFileCell* fc = (ASFileCell*)cell;
    	[fc setIcon:[item smallIcon]];
    }
}



#pragma mark -
#pragma mark Window Delegate Methods



- (void)windowWillClose:(NSNotification*)notification
{
    (void)notification;
    [NSApp terminate:self];
}



#pragma mark -
#pragma mark Application Support and Auto Run


/** Return the path to the application support folder.
 */
- (NSString*)appSupportFolderPath
{
    NSString* path = @"~/Library/Application Support/AlphaSync";
    path = [path stringByExpandingTildeInPath];
    return path;
}


/** Return the path to the (installed) launcher application.
 */
- (NSString*)launcherInstallPath
{
    NSString* path = [[self appSupportFolderPath] stringByAppendingPathComponent:kASLauncherName];
    path = [path stringByAppendingPathExtension:kASLauncherExtension];
    return path;
}

/** This will create an application support directory and install the launcher
 *  application there (so that the path is independent of the application's install
 *  location.
 */
- (void)installApplicationSupport
{
    NSFileManager* fm = [NSFileManager defaultManager];

    if (!launcherInstallPath)
    {
        launcherInstallPath = [[self launcherInstallPath] retain];
        if (!launcherInstallPath) return;
    }


    [fm removeItemAtPath:launcherInstallPath error:nil];      // REVIEW: should we always force a reinstall?
    if (![fm fileExistsAtPath:launcherInstallPath])
    {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* launcherSourcePath = [bundle pathForResource:kASLauncherName ofType:kASLauncherExtension];

        aq_FileUtilitiesCreateDirectoriesForPath([self appSupportFolderPath]);
        if (![fm copyItemAtPath:launcherSourcePath toPath:launcherInstallPath error:nil])
        {
            NSLog(@"Error copying launcher to application support directory");
        }
        else
        {
            CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)launcherInstallPath, kCFURLPOSIXPathStyle, false);
            if (url)
            {
                LSRegisterURL(url, true);
                CFRelease(url);
            }
        }
    }
}


/** Setup the auto-run configuration.
 *
 *  @param  secondsToWait   The number of seconds to wait before enabling auto-launching.
 */
- (void)setupAutoRun:(float)secondsToWait
{
    BOOL shouldAutoRun = [[NSUserDefaults standardUserDefaults] boolForKey:kASPreferenceKeyAutoRun];

    if (!launcherInstallPath) [self installApplicationSupport];
    if (!launcherInstallPath) return;

    AQLauncherRef launcher = aq_LauncherOpen((CFStringRef)launcherInstallPath);
    if (launcher)
    {
        if (shouldAutoRun)
        {
            NSString* appID = kASApplicationBundleID;
            aq_LauncherClearLaunchItems(launcher);
            aq_LauncherAddLaunchItem(launcher, (CFStringRef)appID, 0x081e, 0xbd04);    // REVIEW: symbolic constants for the USB IDs and remove
            aq_LauncherAddLaunchItem(launcher, (CFStringRef)appID, 0x081e, 0xbd01);    // REVIEW: duplication with the USB device factory
            aq_LauncherInstall(launcher);

            if (secondsToWait <= 0.0) aq_LauncherStart(launcher);
            else aq_LauncherPause(launcher, secondsToWait);
        }
        else
        {
            aq_LauncherStop(launcher);          // stop existing application launching
            aq_LauncherUninstall(launcher);     // remove from login items
        }

        aq_LauncherClose(launcher);
    }
}



#pragma mark -
#pragma mark Application Delegate Methods



- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    (void)sender;
    [deviceController closeAllDevices];
    if (!appIsUninstalling) [self setupAutoRun:8.0];    // disable launch requests to prevent immediate application re-open
    return NSTerminateNow;
}


#pragma mark -
#pragma mark Uninstall

- (void)uninstall
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:kASPreferenceKeyAutoRun];
    [[NSUserDefaults standardUserDefaults] synchronize];

    NSString* applicationSupportPath = [self appSupportFolderPath];
    NSString* launcherPath = [self launcherInstallPath];

    AQLauncherRef launcher = aq_LauncherOpen((CFStringRef)launcherPath);
    if (launcher)
    {
        aq_LauncherUninstall(launcher);
        aq_LauncherClose(launcher);
    }

    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSInteger tag;
    NSString* applicationPath = [[NSBundle mainBundle] bundlePath];
    NSArray* filesToTrash = [NSArray arrayWithObjects:applicationPath, applicationSupportPath, nil];
    [workspace performFileOperation:NSWorkspaceRecycleOperation
               source:@"/"          // source files are all absolute paths
               destination:@""      // there is no destination
               files:filesToTrash
               tag:&tag];

    appIsUninstalling = TRUE;
    [NSApp terminate:self];
}


@end
