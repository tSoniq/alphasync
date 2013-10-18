/** @file   ASInspectorWindowController.h
 *  @brief  Controller for the inspector panel.
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
#import "ASDeviceNode.h"

#define kASInspectorBoxMain         (0)
#define kASInspectorBoxShowPreview  (1)
#define kASInspectorBoxPreview      (2)
#define kASInspectorBoxShowDetail   (3)
#define kASInspectorBoxDetailSystem (4)
#define kASInspectorBoxDetailApplet (5)
#define kASInspectorBoxDetailFile   (6)
#define kASInspectorMaxBox          (7)

@interface ASInspectorWindowController : NSWindowController
{
    IBOutlet NSBox* outletBox0;
    IBOutlet NSBox* outletBox1;
    IBOutlet NSBox* outletBox2;
    IBOutlet NSBox* outletBox3;
    IBOutlet NSBox* outletBox4;
    IBOutlet NSBox* outletBox5;
    IBOutlet NSBox* outletBox6;
    IBOutlet NSImageView* outletMainIcon;
    IBOutlet NSTextField* outletMainName;
    IBOutlet NSTextField* outletMainType;
    IBOutlet NSTextField* outletMainSize;
    IBOutlet NSTextField* outletMainRamRom;
    IBOutlet NSButton* outletShowPreviewDisclosure;
    IBOutlet NSTextField* outletPreviewAsText;
    IBOutlet NSImageView* outletPreviewAsIcon;
    IBOutlet NSButton* outletShowDetailDisclosure;
    IBOutlet NSTextField* outletSystemVersion;
    IBOutlet NSTextField* outletSystemBuild;
    IBOutlet NSTextField* outletSystemDate;
    IBOutlet NSTextField* outletSystemInfo;
    IBOutlet NSTextField* outletSystemRAM;
    IBOutlet NSTextField* outletSystemROM;
    IBOutlet NSTextField* outletAppletDescription;
    IBOutlet NSTextField* outletAppletInfo;
    IBOutlet NSTextField* outletAppletFlags;
    IBOutlet NSTextField* outletAppletROM;
    IBOutlet NSTextField* outletAppletRAM1;
    IBOutlet NSTextField* outletAppletRAM2;
    IBOutlet NSTextField* outletAppletMinFileSize;
    IBOutlet NSTextField* outletAppletMaxFileSize;
    IBOutlet NSTextField* outletFilePassword;
    IBOutlet NSTextField* outletFileRam;
    IBOutlet NSTextField* outletFileKey;

    BOOL completedAwakeFromNib;

    NSArray* displayNodes;

    NSRect windowFrame;
    NSBox* boxes[kASInspectorMaxBox];
    NSRect boxFrame[kASInspectorMaxBox];
    BOOL showBox[kASInspectorMaxBox];

    NSImage* iconNothing;
}

- (id)initWithDisplay:(NSArray*)nodes;

- (IBAction)actionPreviewDisclosure:(id)sender;
- (IBAction)actionDetailDisclosure:(id)sender;

- (void)setDisplay:(NSArray*)nodes;
- (void)clearDisplay;

@end

