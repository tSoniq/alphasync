/** @file   ASInspectorWindowController.mm
 *  @brief  Controller for the device info window.
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
#import "ASInspectorWindowController.h"
#import "ASDeviceNode.h"
#import "ASAppletNode.h"
#import "ASFileNode.h"
#import "ASDevice.h"

using namespace ts;

@implementation ASInspectorWindowController


- (id)init
{
    return [self initWithDisplay:nil];
}


- (id)initWithDisplay:(NSArray*)nodes
{
    if ((self = [super initWithWindowNibName:@"Inspector"]))
    {
        displayNodes = [nodes retain];
        iconNothing = [NSImage imageNamed:@"icon-file-noselection"];

        for (int i = 0; i < kASInspectorMaxBox; i++)  showBox[i] = TRUE;
    }
    return self;
}


- (void)dealloc
{
    [displayNodes release];
    [super dealloc];
}


- (void)configurePanelsToShowSystem:(BOOL)showSystem showApplet:(BOOL)showApplet showFile:(BOOL)showFile
{
    BOOL showPreview = ([outletShowPreviewDisclosure state] == NSOnState);
    BOOL showDetail = ([outletShowDetailDisclosure state] == NSOnState);

    for (int i = 0; i < kASInspectorMaxBox; i++)  showBox[i] = TRUE;
    showBox[kASInspectorBoxPreview] = showPreview;
    showBox[kASInspectorBoxDetailSystem] = showDetail && showSystem;
    showBox[kASInspectorBoxDetailApplet] = showDetail && showApplet;
    showBox[kASInspectorBoxDetailFile] = showDetail && showFile;


    // get the original settings for reestablishing later:
    NSUInteger sizingMasks[kASInspectorMaxBox];
    for (int i = 0; i < kASInspectorMaxBox; i++)
    {
        sizingMasks[i] = [boxes[i] autoresizingMask];
        [boxes[i] setAutoresizingMask:NSViewNotSizable];
    }

    float displacement = 0.0;

    NSRect dynamicFrame[kASInspectorMaxBox];
    for (int i = 0; i < kASInspectorMaxBox; i++) dynamicFrame[i] = boxFrame[i];

    for (int i = 0; i < kASInspectorMaxBox; i++)
    {
        if (!showBox[i])
        {
            for (int j = 0; j < i; j++)
            {
                dynamicFrame[j].origin.y -= boxFrame[i].size.height;
            }
            displacement += boxFrame[i].size.height;
            [boxes[i] setHidden:YES];
        }
        else
        {
            [boxes[i] setHidden:NO];
        }
    }

    for (int i = 0; i < kASInspectorMaxBox; i++)
    {
        [boxes[i] setFrame:dynamicFrame[i]];
    }

    NSRect currentWindowFrame = [[self window] frame];
    float oldHeight = currentWindowFrame.size.height;
    float newHeight = windowFrame.size.height - displacement;
    currentWindowFrame.size.height = newHeight;
    currentWindowFrame.origin.y -= (newHeight - oldHeight);

    [[self window] setFrame:currentWindowFrame display:YES];
    [[self window] display];

    for (int i = 0; i < kASInspectorMaxBox; i++)
    {
        [boxes[i] setAutoresizingMask:sizingMasks[i]];
    }
}


- (void)setDetailSystem:(ASDeviceNode*)deviceNode setMain:(BOOL)setMain
{
    NSString* mainName = @"";
    NSString* mainType = @"";
    NSString* mainSize = @"";
    NSString* mainRamRom = @"";
    NSString* systemVersion = @"";
    NSString* systemBuild = @"";
    NSString* systemDate = @"";
    NSString* systemInfo = @"";
    NSString* systemRAM = @"";
    NSString* systemROM = @"";

    if (deviceNode)
    {
        unsigned ram = [deviceNode availableRam];
        unsigned rom = [deviceNode availableRom];

        mainName = [deviceNode displayName];
        mainType = [NSString stringWithFormat:@"System %@",[deviceNode systemVersion]];
        mainRamRom = @"";
        mainSize = @"";

        systemVersion = [deviceNode systemVersion];
        systemBuild = [deviceNode systemBuild];
        systemDate = [deviceNode systemDate];
        systemInfo = [deviceNode systemInfo];
        systemRAM = [NSString stringWithFormat:@"%.1lf kbytes", double(ram)/1024];
        systemROM = [NSString stringWithFormat:@"%.1lf kbytes", double(rom)/1024];
    }

    if (setMain)
    {
        [outletMainName setStringValue:mainName];
        [outletMainType setStringValue:mainType];
        [outletMainRamRom setStringValue:mainRamRom];
        [outletMainSize setStringValue:mainSize];
    }

    [outletSystemVersion setStringValue:systemVersion];
    [outletSystemBuild setStringValue:systemBuild];
    [outletSystemDate setStringValue:systemDate];
    [outletSystemInfo setStringValue:systemInfo];
    [outletSystemRAM setStringValue:systemRAM];
    [outletSystemROM setStringValue:systemROM];
}


- (void)setDetailApplet:(ASAppletNode*)appletNode setMain:(BOOL)setMain
{
    NSString* mainName = @"";
    NSString* mainType = @"";
    NSString* mainSize = @"";
    NSString* mainRamRom = @"";
    NSString* appletDescription = @"";
    NSString* appletInfo = @"";
    NSString* appletFlags = @"";
    NSString* appletROM = @"";
    NSString* appletRAM1 = @"";
    NSString* appletRAM2 = @"";
    NSString* appletMinFileSize = @"";
    NSString* appletMaxFileSize = @"";

    if (appletNode)
    {
        ASDevice* device = [appletNode device];
        const ASApplet* applet = device->appletAtIndex([appletNode appletIndex]);
        char version[16];
        snprintf(version, sizeof version, "Version %u.%u%c", applet->appletVersionMajor(), applet->appletVersionMinor(), applet->appletVersionRevision());
        char description[256];
        snprintf(description, sizeof description, "%s %u.%u%c (%s)",
            applet->appletName(),
            applet->appletVersionMajor(), applet->appletVersionMinor(), applet->appletVersionRevision(),
            applet->appletLanguageName());
        unsigned flags = applet->appletFlags();
        BOOL isHidden = (flags & kASAppletFlagsHidden) != 0;

        mainName = [appletNode displayName];
        mainType = [NSString stringWithCString:version encoding:NSWindowsCP1252StringEncoding];
        unsigned totalRAM = [appletNode ramUsed] + applet->appletRamSize();
        if (totalRAM < 250) mainSize = [NSString stringWithFormat:@"%d bytes", totalRAM];
        else mainSize = [NSString stringWithFormat:@"%3.1lfkB", double(totalRAM)/1024.0];
        unsigned fileCount = [appletNode fileCount];
        if (0 == fileCount) mainRamRom = @"";
        else if (1 == fileCount) mainRamRom = @"1 file";
        else mainRamRom = [NSString stringWithFormat:@"%u files", fileCount];

        appletDescription = [NSString stringWithCString:description encoding:NSWindowsCP1252StringEncoding];
        appletInfo = [NSString stringWithCString:applet->appletInfo() encoding:NSWindowsCP1252StringEncoding];
        appletFlags = (isHidden) ? @"hidden" : @"visible";

        unsigned min = [appletNode minFileSize];
        unsigned max = [appletNode maxFileSize];
        appletMinFileSize = [NSString stringWithFormat:@"%u bytes", min];
        appletMaxFileSize = (max == UINT_MAX) ? @"unlimited" : [NSString stringWithFormat:@"%u bytes", max];

        appletROM = [NSString stringWithFormat:@"%u bytes", applet->appletRomSize()];
        appletRAM1 = [NSString stringWithFormat:@"%u bytes (workspace)", applet->appletRamSize()];
        appletRAM2 = [NSString stringWithFormat:@"%u bytes (files)", [appletNode ramUsed]];
    }

    if (setMain)
    {
        [outletMainName setStringValue:mainName];
        [outletMainType setStringValue:mainType];
        [outletMainRamRom setStringValue:mainRamRom];
        [outletMainSize setStringValue:mainSize];
    }

    [outletAppletDescription setStringValue:appletDescription];
    [outletAppletInfo setStringValue:appletInfo];
    [outletAppletFlags setStringValue:appletFlags];
    [outletAppletROM setStringValue:appletROM];
    [outletAppletRAM1 setStringValue:appletRAM1];
    [outletAppletRAM2 setStringValue:appletRAM2];
    [outletAppletMinFileSize setStringValue:appletMinFileSize];
    [outletAppletMaxFileSize setStringValue:appletMaxFileSize];
}


- (void)setDetailFile:(ASFileNode*)fileNode setMain:(BOOL)setMain
{
    NSString* mainName = @"";
    NSString* mainType = @"";
    NSString* mainSize = @"";
    NSString* mainRamRom = @"";
    NSString* filePassword = @"";
    NSString* fileRam = @"";
    NSString* fileKey = @"";

    if (fileNode)
    {
        ASAppletNode* appletNode = [fileNode appletNode];
        mainName = [fileNode displayName];
        mainType = (appletNode) ? [appletNode displayName] : @"";
        int fileSize = [fileNode fileSize];
        if (fileSize < 250) mainSize = [NSString stringWithFormat:@"%d bytes", fileSize];
        else  mainSize = [NSString stringWithFormat:@"%3.1lfkB", double(fileSize)/1024.0];
        mainRamRom = @"";

        filePassword = [fileNode password];
        fileRam = [NSString stringWithFormat:@"%u bytes", fileSize];
        fileKey = [fileNode displayFileSpace];
    }

    if (setMain)
    {
        [outletMainName setStringValue:mainName];
        [outletMainType setStringValue:mainType];
        [outletMainRamRom setStringValue:mainRamRom];
        [outletMainSize setStringValue:mainSize];
    }

    [outletFilePassword setStringValue:filePassword];
    [outletFileRam setStringValue:fileRam];
    [outletFileKey setStringValue:fileKey];
}


- (void)configureDisplay
{
    if (!completedAwakeFromNib) return;

    if (nil == displayNodes || [displayNodes count] == 0)
    {
        // Nothing selected
        [self configurePanelsToShowSystem:NO showApplet:NO showFile:NO];
        [outletMainIcon setImage:iconNothing];
        [outletMainName setStringValue:@"nothing selected"];
        [outletMainType setStringValue:@""];
        [outletMainRamRom setStringValue:@""];
        [outletMainSize setStringValue:@""];
        [outletPreviewAsIcon setImage:iconNothing];
        [outletPreviewAsText setHidden:YES];
        [outletPreviewAsIcon setHidden:NO];
    }
    else if ([displayNodes count] == 1)
    {
        // Single selection
        id item = [displayNodes objectAtIndex:0];
        [outletMainIcon setImage:[item largeIcon]];

        NSString* previewString = nil;
        if ([item respondsToSelector:@selector(filePreview)]) previewString = [item filePreview];
        if (previewString)
        {
            [outletPreviewAsText setStringValue:previewString];
            [outletPreviewAsText setHidden:NO];
            [outletPreviewAsIcon setHidden:YES];
        }
        else
        {
            [outletPreviewAsIcon setImage:[item largeIcon]];
            [outletPreviewAsText setHidden:YES];
            [outletPreviewAsIcon setHidden:NO];
        }

        if ([item isMemberOfClass:[ASFileNode class]])
        {
            ASFileNode* fileNode = item;
            ASAppletNode* appletNode = [fileNode appletNode];
            ASDeviceNode* deviceNode = [appletNode deviceNode];
            [self configurePanelsToShowSystem:NO showApplet:NO showFile:YES];
            [self setDetailFile:fileNode setMain:YES];
            [self setDetailApplet:appletNode setMain:NO];
            [self setDetailSystem:deviceNode setMain:NO];
        }
        else if ([item isMemberOfClass:[ASAppletNode class]])
        {
            ASFileNode* fileNode = nil;
            ASAppletNode* appletNode = item;
            ASDeviceNode* deviceNode = [appletNode deviceNode];
            [self configurePanelsToShowSystem:NO showApplet:YES showFile:NO];
            [self setDetailFile:fileNode setMain:NO];
            [self setDetailApplet:appletNode setMain:YES];
            [self setDetailSystem:deviceNode setMain:NO];
        }
        else if ([item isMemberOfClass:[ASDeviceNode class]])
        {
            ASFileNode* fileNode = nil;
            ASAppletNode* appletNode = nil;
            ASDeviceNode* deviceNode = item;
            [self configurePanelsToShowSystem:YES showApplet:NO showFile:NO];
            [self setDetailFile:fileNode setMain:NO];
            [self setDetailApplet:appletNode setMain:NO];
            [self setDetailSystem:deviceNode setMain:YES];
        }
    }
    else
    {
        // Multiple selection
        [outletMainName setStringValue:@"multiple items"];
        [self configurePanelsToShowSystem:NO showApplet:NO showFile:NO];

    }
}

- (void)awakeFromNib
{
    windowFrame = [[self window] frame];
    boxes[kASInspectorBoxMain] = outletBox0;
    boxes[kASInspectorBoxShowPreview] = outletBox1;
    boxes[kASInspectorBoxPreview] = outletBox2;
    boxes[kASInspectorBoxShowDetail] = outletBox3;
    boxes[kASInspectorBoxDetailSystem] = outletBox4;
    boxes[kASInspectorBoxDetailApplet] = outletBox5;
    boxes[kASInspectorBoxDetailFile] = outletBox6;

    for (int i = 0; i < kASInspectorMaxBox; i++)
    {
        boxFrame[i] = [boxes[i] frame];
    }

    completedAwakeFromNib = TRUE;
    [self configureDisplay];
}


- (void)setDisplay:(NSArray*)nodes
{
    if (nodes != displayNodes)
    {
        [displayNodes release];
        displayNodes = [nodes retain];
        [self configureDisplay];
    }
}


- (void)clearDisplay
{
    if (displayNodes)
    {
        [displayNodes release];
        [self configureDisplay];

    }
}


#pragma mark    ----------  Actions  -----------

- (IBAction)actionPreviewDisclosure:(id)sender
{
    (void)sender;
    [self configureDisplay];
}


- (IBAction)actionDetailDisclosure:(id)sender
{
    (void)sender;
    [self configureDisplay];
}


@end


