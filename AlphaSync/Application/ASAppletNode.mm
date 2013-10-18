/** @file   ASAppletNode.mm
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
#import "limits.h"
#import "ASAppletNode.h"
#import "ASFileNode.h"
#import "ASDeviceAccessProtocol.h"
#import "ASSettings.h"


using namespace ts;


@implementation ASAppletNode


static NSImage* iconApplet = nil;              /**< Icon to use for applets. */



/** Raise an error if the default constructor is used.
 */
- (id)init
{
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}


/** Designated initialiser. This will automatically try to enumerate the files contained by the applet.
 *
 *  @param  devNode     The device that this node will represent.
 *  @return             A pointer to self.
 */
- (id)initWithDevice:(ts::ASDevice*)dev appletIndex:(int)appletIdx
{
    if ((self = [super init]))
    {
        if (!iconApplet)
        {
            NSBundle* bundle = [NSBundle mainBundle];
            iconApplet = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-applet" ofType:@"pdf"]];
            [iconApplet setDataRetained:YES];
        }

        appletIndex = appletIdx;
        device = dev;
        applet = device->appletAtIndex(appletIndex);
        if (!applet)
        {
            self = nil;
        }
        else
        {
            [self reload];
        }
    }
    return self;
}


/** Destructor. This will implicitly destroy any child nodes.
 */
- (void)dealloc
{
    [super dealloc];
}


/** Return the device node for this applet.
 */
- (ASDeviceNode*)deviceNode
{
    return (ASDeviceNode*) [self parent];
}


/** Return the applet index.
 */
- (int)appletIndex
{
    return appletIndex;
}


/** Return the device handle.
 */
- (ts::ASDevice*)device
{
    return device;
}


/** Return the applet handle.
 */
- (const ts::ASApplet*)applet
{
    return applet;
}


/** Return the minimum file size.
 */
- (unsigned)minFileSize
{
    return minFileSize;
}


/** Return the maximum file size.
 */
- (unsigned)maxFileSize
{
    return maxFileSize;
}


/** Return the ram usage (bytes).
 */
- (unsigned)ramUsed
{
    return ramUsed;
}


/** Return the number of files used.
 */
- (unsigned)fileCount
{
    return fileCount;
}



/** Refresh applet data, without re-enumerating.
 */
- (void)refresh
{
    device->getAppletResourceUsage(&fileCount, &ramUsed, applet);

    [self setIcon:iconApplet];
    [self setDisplayName:[NSString stringWithCString:applet->appletName() encoding:NSWindowsCP1252StringEncoding]];
    [self setDisplayKind:@"Applet"];
    [self setDisplaySize:@""];

    maxFileSize = UINT_MAX;
    minFileSize = 0;

    if (applet->appletID() == kASAppletID_AlphaWord)
    {
        uint8_t settingsData[8192];
        unsigned actualBytes;
        if (device->readSettings(settingsData, &actualBytes, sizeof settingsData, applet, 0x0b))
        {
            ASSettings settings(settingsData, actualBytes, sizeof settingsData);
            ASSettingsItem item;
            if (settings.findSettingsItem(&item, kASSettingsType_Range32, kASSettingsIdent_AlphaWord_MaxFileSize))  maxFileSize = item.dataU32();
            if (settings.findSettingsItem(&item, kASSettingsType_Range32, kASSettingsIdent_AlphaWord_MinFileSize))  minFileSize = item.dataU32();
        }
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

    int fileIndex = 1;
    ASFileAttributes attr;
    while (device->getFileAttributes(&attr, applet, fileIndex))
    {
        ASFileNode* fileNode = [[[ASFileNode alloc] initWithDevice:device applet:applet fileIndex:fileIndex fileAttributes:&attr] autorelease];
        if (fileNode) [self addChild:fileNode];
        fileIndex ++;
    }

    [self sortChildren];
    [self refresh];
}



/** Helper method: check if the applet already contains a file corresponding to the path.
 *
 *  @param  path        The path to parse. Any directory components are ignored.
 *  @return             The ASFileNode that matches the file name (from our children).
 */
- (ASFileNode*)findFileNodeMatchingPath:(NSString*)path
{
    NSString* matchingNameWithExtension = [path lastPathComponent];
    NSString* matchingName = [matchingNameWithExtension stringByDeletingPathExtension];
    NSString* matchingExtension = [matchingNameWithExtension pathExtension];

    NSEnumerator* enumerator = [[self children] objectEnumerator];
    ASFileNode* fileNode;
    while ((fileNode = [enumerator nextObject]))
    {
        NSString* basename;
        NSString* extension;

        [fileNode getFileNameBase:&basename extension:&extension];
        if (NSOrderedSame == [basename caseInsensitiveCompare:matchingName] &&
            NSOrderedSame == [extension caseInsensitiveCompare:matchingExtension])
        {
            return fileNode;    // Got a match
        }
    }

    return nil;     // No match
}


#pragma mark    --------  ASLoadProtocol  --------


- (BOOL)loadPermittedFromPath:(NSString*)path
{
    NSFileManager* fm = [NSFileManager defaultManager];
    if (![fm fileExistsAtPath:path]) return FALSE;

    switch (applet->appletID())
    {
        case kASAppletID_AlphaWord:
            if (!filePathConformsToUTI(path, @"public.plain-text")) return FALSE;
            break;

        default:
            // Only AlphaWord files can be created at present. Creating files for other applets
            // may have unpredicted consequences...
            return FALSE;
            break;
    }

    return TRUE;
}


- (BOOL)loadFromPath:(NSString*)path
{
    BOOL result = FALSE;

    id accessDelegate = [self delegate];
    BOOL shouldProceed = YES;

    if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
    {
        shouldProceed = [accessDelegate deviceWillLoad:path sender:self];
    }

    if (shouldProceed)
    {
        ASFileNode* fileNode = [ASFileNode createFileNodeOnDevice:device applet:applet path:path];
        result = (fileNode != 0);
        if (result)
        {
            [self addChild:fileNode];
            [self sortChildren];
        }

        [self refresh];

        if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
        {
            [accessDelegate deviceDidLoad:path error:(result)?nil:@"Failed to create file." sender:self];
        }
    }

    return result;
}


- (BOOL)loadPermittedFromPaths:(NSArray*)paths
{
    if ([paths count] != 1) return FALSE;
    else return [self loadPermittedFromPath:[paths objectAtIndex:0]];
}


- (unsigned)loadFromPaths:(NSArray*)paths
{
    if ([paths count] != 1) return 0;
    else return [self loadFromPath:[paths objectAtIndex:0]] ? 1 : 0;
}




#pragma mark    -------- ASDeleteProtocol  --------


/** Return TRUE if the instance of the node can accept delete operations.
 */
- (BOOL)deletePermitted
{
    // Only permit delete for files if this is an alphaword instance. Delete on
    // other applets may have unexpected (ie unknown) consequences without more information
    // from the applet vendor.
    return (applet->appletID() == kASAppletID_AlphaWord);
}



- (BOOL)deleteSelf
{
    BOOL shouldProceed = TRUE;
    BOOL didDelete = FALSE;

    id accessDelegate = [self delegate];
    if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
    {
        shouldProceed = [accessDelegate deviceWillDelete:self sender:self];
    }

    if (shouldProceed)
    {
        didDelete = device->clearAllFiles(applet);

        [self reload];

        if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
        {
            [accessDelegate deviceDidDelete:self error:(didDelete)?nil:@"Failed to delete file." sender:self];
        }
    }

    return didDelete;
}


@end
