/** @file   ASApplicationContoller.h
 *  @brief  Application controller class. This is responsible for creating and managing the device
 *          driver and user interface.
 *
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
#import <Cocoa/Cocoa.h>
#import "ASDeviceController.h"
#import "ASPreferenceController.h"
#import "ASBackupController.h"
#import "ASNodeDataSource.h"
#import "ASDeviceAccessProtocol.h"
#import "ASInspectorWindowController.h"

@interface ASApplicationController : NSObject <ASDeviceAccessProtocol, NSWindowDelegate, NSOutlineViewDelegate, NSToolbarDelegate>
{
    IBOutlet NSWindow* outletMainWindow;
    IBOutlet NSOutlineView* outletFileView;
    IBOutlet NSTextField* outletStatusText;
    IBOutlet NSProgressIndicator* outletProgressIndicator;
    IBOutlet NSImageView* outletStatusImage;
    IBOutlet NSMenuItem* outletGetInfoMenu;
    IBOutlet NSMenuItem* outletSaveMenu;
    IBOutlet NSTextField* outletAvailableMemory;

    ASNodeDataSource* dataSource;

    NSImage* iconDisconnected;
    NSImage* iconRed;
    NSImage* iconYellow;
    NSImage* iconGreen;
    NSImage* iconToolbarInfo;
    NSImage* iconToolbarBackup;
    NSImage* iconToolbarDeleteDisabled;
    NSImage* iconToolbarDeleteFile;
    NSImage* iconToolbarDeleteFiles;

    NSString* launcherInstallPath;

    ASPreferenceController* preferenceController;
    ASBackupController* backupController;
    ASDeviceController* deviceController;
    ASInspectorWindowController* inspectorController;

    NSTimer* statusDisplayTimer;
    NSToolbar* toolbar;

    BOOL appIsUninstalling;
}

- (id)init;
- (void)dealloc;
- (void)awakeFromNib;

- (IBAction)actionDelete:(id)sender;
- (IBAction)actionInfo:(id)sender;
- (IBAction)actionBackup:(id)sender;
- (IBAction)actionShowPreferences:(id)sender;
- (IBAction)actionOpenBackupFolder:(id)sender;
- (IBAction)actionUninstall:(id)sender;
- (IBAction)reloadDeviceData:(id)sender;

- (void)setupAutoRun:(float)secondsToWait;

@end
