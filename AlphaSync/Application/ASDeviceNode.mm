/** @file   ASDeviceNode.mm
 *  @brief  Node used to construct a tree representation of the contents of a device.
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
#import "ASDeviceNode.h"
#import "ASAppletNode.h"
#import "ASDevice.h"
#import "ASApplet.h"
#import "ASPreferenceController.h"

using namespace ts;


@implementation ASDeviceNode


static NSImage* iconDeviceNeo = nil;              /**< Icon to use for Neo mk1 devices. */



/** Raise an error if the default constructor is used.
 */
- (id)init
{
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}


/** Designated initialiser. This will automatically try to enumerate the applets contained in the device.
 *
 *  @param  dev     The device that this node will represent.
 *  @return         A pointer to self.
 */
- (id)initWithDevice:(ts::ASDevice*)dev
{
    if ((self = [super init]))
    {
        if (!iconDeviceNeo)
        {
            NSBundle* bundle = [NSBundle mainBundle];
            iconDeviceNeo = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-neo" ofType:@"pdf"]];
            [iconDeviceNeo setDataRetained:YES];
        }

        device = dev;

        // REVIEW: need code here to find an explicit name for the device from hardware IDs and/or files on the device
        [self reload];
    }
    return self;
}


/** Destructor. This will implicitly destroy any child nodes.
 */
- (void)dealloc
{
    if (systemVersion) [systemVersion release];
    if (systemBuild) [systemBuild release];
    if (systemDate) [systemDate release];
    if (systemInfo) [systemInfo release];

    systemVersion = nil;
    systemBuild = nil;
    systemDate = nil;
    systemInfo = nil;
    [super dealloc];
}



/** Return the device handle.
 */
- (ts::ASDevice*)device
{
    return device;
}


/** Return the system version.
 */
- (NSString*)systemVersion
{
    return systemVersion;
}


/** Return the system build.
 */
- (NSString*)systemBuild
{
    return systemBuild;
}


/** Return the system date.
 */
- (NSString*)systemDate
{
    return systemDate;
}


/** Return the system info string.
 */
- (NSString*)systemInfo
{
    return systemInfo;
}


/** Return the available ram.
 */
- (unsigned)availableRam
{
    return availableRam;
}


/** Return the available rom.
 */
- (unsigned)availableRom
{
    return availableRom;
}



/** Refresh device data.
 */
- (void)refresh
{
    char build[64];
    char date[64];
    unsigned major;
    unsigned minor;

    if (systemVersion) [systemVersion release];
    if (systemBuild) [systemBuild release];
    if (systemDate) [systemDate release];
    if (systemInfo) [systemInfo release];

    const ASApplet* systemApplet = device->appletForID(kASAppletID_System);
    if (systemApplet)
    {
        device->systemVersion(&major, &minor, build, date);

        systemVersion = [[NSString stringWithFormat:@"OS %u.%u", major, minor] retain];
        systemBuild = [[NSString stringWithCString:build encoding:NSWindowsCP1252StringEncoding] retain];
        systemDate = [[NSString stringWithCString:date encoding:NSWindowsCP1252StringEncoding] retain];
        const char* appletInfo = (systemApplet) ? systemApplet->appletInfo() : "";
        systemInfo = [[NSString stringWithCString:appletInfo encoding:NSWindowsCP1252StringEncoding] retain];
        device->systemMemory(&availableRam, &availableRom);

        [self setIcon:iconDeviceNeo];
        [self setDisplayName:@"Neo"];
        [self setDisplayKind:@"Device"];
        [self setDisplaySize:@""];
    }
    else
    {
        systemVersion = @"";
        systemBuild = @"";
        systemDate = @"";
        systemInfo = @"";
        availableRam = 0;
        availableRom = 0;

        [self setIcon:[NSImage imageNamed:@"icon-file-noselection"]];
        [self setDisplayName:@"disconnected"];
        [self setDisplayKind:@"Device"];
        [self setDisplaySize:@""];
    }

    [[self parent] refresh];
}


/** Re-enumerate. Any existing children will be destroyed and new ones constructed.
 *  This is implicitly called when the device is constructed, but can also be used to force a
 *  re-enumeration following changes to the device data.
 */
- (void)reload
{
    [self removeAllChildren];

    int displayOption = [[NSUserDefaults standardUserDefaults] integerForKey:kASPreferenceKeyFilter];

    const ASApplet* systemApplet = device->appletForID(kASAppletID_System);
    if (systemApplet)
    {
        int appletIndex = 0;
        const ASApplet* applet;
        while ((applet = device->appletAtIndex(appletIndex)))
        {
            BOOL ignore = true;
            ASAppletID appletID = applet->appletID();

            switch (displayOption)
            {
                case kASPreferenceOptionFilterAlphawordOnly:
                    ignore = (kASAppletID_AlphaWord != appletID);
                    break;

                case kASPreferenceOptionFilterAlphawordAndDictionary:
                    ignore = (kASAppletID_AlphaWord != appletID) && (kASAppletID_Dictionary != appletID);
                    break;

                case kASPreferenceOptionFilterAll:
                default:
                    ignore = false;
                    break;
            }

            if (!ignore)
            {
                ASAppletNode* appletNode = [[[ASAppletNode alloc] initWithDevice:device appletIndex:appletIndex] autorelease];
                if (appletNode) [self addChild:appletNode];
            }
            appletIndex ++;
        }

        [self sortChildren];
    }

    [self refresh];
}

@end
