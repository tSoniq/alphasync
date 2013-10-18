/** @file   ASFileNode.mm
 *  @brief  Node used to construct a tree representation of the contents of a device.
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
#import "ASFileNode.h"
#import "ASFileAttributes.h"
#import "ASFile.h"
#import "ASGenericFile.h"
#import "ASAlphaWordFile.h"
#import "ASUserDictionaryFile.h"
#import "ASAppletID.h"
#import "ASDeviceAccessProtocol.h"
#import "ASPreferenceController.h"

using namespace ts;


@implementation ASFileNode


static NSImage* iconFileTxt = nil;              /**< Icon to use for text files. */
static NSImage* iconFileBin = nil;              /**< Icon to use for binary files. */



/** Class method used to create a new file on the device.
 *
 *  @param  devicePointer       The device that owns this node.
 *  @param  applet              The applet.
 *  @param  filename            The new node filename.
 *  @param  data                A pointer to the data to load to the device.
 *  @param  size                The number of bytes of data.
 *  @return                     A pointer to the created node. The node will be auto-released.
 */
+ (ASFileNode*)createFileNodeOnDevice:(ts::ASDevice*)device applet:(const ts::ASApplet*)applet filename:(NSString*)filename data:(const void*)data size:(unsigned)size
{
    // Extract from the supplied path a filename to use on the device
    char deviceFilename[kASFileAttributesFileNameMaxSize + 1];
    const char* pathName = [filename cStringUsingEncoding:NSWindowsCP1252StringEncoding];
    if (!pathName) pathName = "Untitled";
    strncpy(deviceFilename, pathName, kASFileAttributesFileNameMaxSize);
    deviceFilename[kASFileAttributesFileNameMaxSize] = 0;


    // Ensure that the filename is unique by adding a suffix if necessary
    int uniqueID = 0;
    while (true)
    {
        bool isUnique = true;
        int fileIndexNumber = 0;
        ASFileAttributes attr;
        while (device->getFileAttributes(&attr, applet, fileIndexNumber++))
        {
            if (0 == strcasecmp(deviceFilename, attr.fileName()))
            {
                isUnique = false;
                break;
            }
        }

        if (isUnique) break;
        if (++ uniqueID > 999) return nil;      // Neo will run out of space long before this...

        char buffer[8];
        snprintf(buffer, sizeof buffer, "-%d", uniqueID);

        strncpy(deviceFilename, pathName, kASFileAttributesFileNameMaxSize - strlen(buffer));
        deviceFilename[kASFileAttributesFileNameMaxSize - strlen(buffer)] = 0;
        strcat(deviceFilename, buffer);
    }


    // Create the file
    ASFileNode* node = nil;
    int fileIndexNumber;
    if (device->createFile(deviceFilename, "write", data, size, applet, &fileIndexNumber, true))
    {
        ASFileAttributes attr;
        device->getFileAttributes(&attr, applet, fileIndexNumber);
        node = [[[ASFileNode alloc] initWithDevice:device applet:applet fileIndex:fileIndexNumber fileAttributes:&attr] autorelease];
    }
    return node;
}


/** Class method used to create a new file on the device.
 *
 *  @param  devicePointer       The device that owns this node.
 *  @param  applet              The applet.
 *  @param  path                The path of a file to be loaded.
 *  @return                     A pointer to the created node.
 */
+ (ASFileNode*)createFileNodeOnDevice:(ts::ASDevice*)device applet:(const ASApplet*)applet path:(NSString*)path
{
    ASFileNode* fileNode = nil;
    ASFile* file = 0;

    NSStringEncoding encoding = NSMacOSRomanStringEncoding;
    NSError* error = nil;
    NSString* contentsOfPath = [NSString stringWithContentsOfFile:path usedEncoding:&encoding error:&error];
    if (!contentsOfPath) contentsOfPath = [NSString stringWithContentsOfFile:path encoding:NSMacOSRomanStringEncoding error:&error];  // REVIEW: what encoding should be used???
    if (contentsOfPath)
    {
        NSData* unicodeData = [contentsOfPath dataUsingEncoding:NSUnicodeStringEncoding allowLossyConversion:YES];
        if (unicodeData)
        {
            switch (applet->appletID())
            {
                case kASAppletID_AlphaWord:             file = new ASAlphaWordFile;         break;
                case kASAppletID_Dictionary:            file = new ASUserDictionaryFile;    break;
                default:                                file = new ASGenericFile;           break;
            }

            if (file->importText((const uint16_t*)[unicodeData bytes], [unicodeData length]))
            {
                NSString* deviceFilename = [[path lastPathComponent] stringByDeletingPathExtension];
                fileNode = [ASFileNode createFileNodeOnDevice:device
                                       applet:applet
                                       filename:deviceFilename
                                       data:file->fileData()
                                       size:file->fileSize()];
            }

            delete file;
        }
    }

    return fileNode;
}


/** Raise an error if the default constructor is used.
 */
- (id)init
{
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}


/** Designated initialiser. This will automatically try to enumerate the applets contained in the device.
 *
 *  @param  dev                 The device that owns this node.
 *  @param  app                 The applet.
 *  @param  fin                 The file index.
 *  @param  attr                The file attributes.
 *  @return                     A pointer to self.
 */
- (id)initWithDevice:(ts::ASDevice*)devicePtr applet:(const ts::ASApplet*)app fileIndex:(int)fin fileAttributes:(const ts::ASFileAttributes*)attr;
{
    if ((self = [super init]))
    {
        if (!iconFileTxt)
        {
            NSBundle* bundle = [NSBundle mainBundle];
            iconFileTxt = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-file-txt" ofType:@"pdf"]];
            [iconFileTxt setDataRetained:YES];
        }

        if (!iconFileBin)
        {
            NSBundle* bundle = [NSBundle mainBundle];
            iconFileBin = [[NSImage alloc] initWithContentsOfFile:[bundle pathForResource:@"icon-file-bin" ofType:@"pdf"]];
            [iconFileBin setDataRetained:YES];
        }

        fileIndex = fin;
        device = devicePtr;
        applet = new ASApplet(*app);
        fileAttributes = new ASFileAttributes(*attr);

        isFirstRefresh = YES;
        [self reload];
    }
    return self;
}


- (void)dealloc
{
    [filePreview release];
    delete fileData;
    delete fileAttributes;
    delete applet;
    [super dealloc];
}


/** Refresh the tree.
 */
- (void)refresh
{
    // On the first call we already have valid attributes, so avoid the reload overhead
    if (isFirstRefresh || device->getFileAttributes(fileAttributes, applet, fileIndex))
    {
        isFirstRefresh = NO;

        NSImage* fileIcon;

        switch (applet->appletID())
        {
            case kASAppletID_AlphaWord:
                fileData = new ASAlphaWordFile;
                fileExtension = @"txt";
                fileIcon = iconFileTxt;
                fileIsPlainText = YES;
                break;

            case kASAppletID_Dictionary:
                fileData = new ASUserDictionaryFile;
                fileExtension = @"txt";
                fileIcon = iconFileTxt;
                fileIsPlainText = YES;
                break;

            default:
                fileData = new ASGenericFile;
                fileExtension = @"xml";
                fileIcon = iconFileBin;
                fileIsPlainText = NO;
                break;
        }

        fileData->setAppletInfo(applet->appletID(), applet->appletVersionMajor(), applet->appletVersionMinor());

        NSString* fileName = [NSString stringWithCString:fileAttributes->fileName() encoding:NSWindowsCP1252StringEncoding];
        int size = (int)fileAttributes->allocSize();

        NSString* displaySizeString;
        if (size < 1024) displaySizeString = [NSString stringWithFormat:@"%d bytes", size];
        else displaySizeString = [NSString stringWithFormat:@"%3.1f kbytes", float(size) / 1024.0];

        [self setIcon:fileIcon];
        [self setDisplayName:fileName];
        [self setDisplayKind:[NSString stringWithFormat:@"File"]];
        [self setDisplaySize:displaySizeString];
        [self setFileSize:(int)fileAttributes->allocSize()];
        [self setFileSpace:(int)fileAttributes->fileSpace()];

        if (!fileIsSynchronised)
        {
            [filePreview release];
            filePreview = nil;

            if (fileIsPlainText && fileData->minimumLoadSize() != 0 && fileData->minimumLoadSize() <= 128)
            {
                if (fileData->load(device, applet, fileIndex, 128))
                {
                    const uint16_t* unicodeData;
                    unsigned unicodeSize;
                    if (!fileData->exportText(&unicodeData, &unicodeSize))
                    {
                        NSLog(@"Error translating text data from file %d for applet ID %04x", fileIndex, applet->appletID());
                    }
                    else
                    {
                        if (unicodeSize > 0) filePreview = [[NSString stringWithCharacters:unicodeData length:unicodeSize/2] retain];
                        else filePreview = @"";
                    }
                }
            }
        }
    }
    else
    {
        [self setIcon:nil];
        [self setDisplayName:@""];
        [self setDisplayKind:@"Dead"];
        [self setDisplaySize:@""];
        [self setFileSize:0];
        [self setFileSpace:0];
        [filePreview release];
        filePreview = nil;
    }

    [[self parent] refresh];
}


/** Reload only reloads the file attributes. Any existing cached file data is discarded.
 */
- (void)reload
{
    // Don't immediately reload the file data here, as this can take a (very) long time. The data will
    // be loaded on demand from the fileData method.
    fileIsSynchronised = FALSE;
    [self refresh];
}


- (BOOL)fileIsPlainText
{
    return fileIsPlainText;
}

- (const ts::ASFileAttributes*)fileAttributes
{
    return fileAttributes;
}


/** Return the parent applet node.
 */
- (ASAppletNode*)appletNode
{
    return (ASAppletNode*) [self parent];
}


/** Return the file index.
 */
- (int)fileIndex
{
    return fileIndex;
}


/** Return the file password.
 */
- (NSString*)password
{
    return [NSString stringWithCString:fileAttributes->password() encoding:NSWindowsCP1252StringEncoding];
}



/** Try to set a new filename.
 *
 *  @param  newName     The new name to try to use.
 *  @return             The actual name used (which may be completely different if the input made little sense).
 */
- (NSString*)setFileName:(NSString*)newName
{
    const char* newCName = [newName cStringUsingEncoding:NSWindowsCP1252StringEncoding];
    if (newCName && strlen(newCName) > 0)
    {
        fileAttributes->setFileName(newCName);
        device->setFileAttributes(applet, fileIndex, fileAttributes);   // Set the values
        device->getFileAttributes(fileAttributes, applet, fileIndex);   // Re-read them in case the device changes something
    }

    [self refresh];

    return [self displayName];
}


/** Find the basename and extension that would be used when saving the file to disk.
 *
 *  @param  basename    Returns the basename (filename without extension).
 *  @param  extension   Returns the file extension.
 */
- (void)getFileNameBase:(NSString**)basename extension:(NSString**)extension
{
    *basename = @"";
    *extension = fileExtension;

    // parse the device file name to remove any potentially problematic prefix characters
    char buffer[256];
    strncpy(buffer, fileAttributes->fileName(), sizeof buffer);
    buffer[255] = 0;
    char* deviceName = buffer;
    while (*deviceName != 0 && (*deviceName == '.' || *deviceName == '/' || *deviceName == ':'))
    {
        deviceName ++;
    }

    *basename = [NSString stringWithCString:deviceName encoding:NSWindowsCP1252StringEncoding];
    if (!*basename) *basename = @"";

    // If the device filename already has an appropriate extension remove it (to prevent a double extension).
    if (NSOrderedSame == [*extension caseInsensitiveCompare:[*basename pathExtension]])
    {
        *basename = [*basename stringByDeletingPathExtension];
    }
}


/** Return the file preview string, or nil if no preview text is available.
 */
- (NSString*)filePreview
{
    return filePreview;
}


/** Read the file, returning it as a string object.
 *
 *  @return                 The file data as a string, or zero if failed.
 */
- (NSString*)fileData
{
    NSString* result = nil;

    if (!fileIsSynchronised)
    {
        fileIsSynchronised = fileData->load(device, applet, fileIndex);
    }

    if (fileIsSynchronised)
    {
        const uint16_t* unicodeData;
        unsigned unicodeSize;
        if (!fileData->exportText(&unicodeData, &unicodeSize))
        {
            NSLog(@"Error translating text data from file %d for applet ID %04x", fileIndex, applet->appletID());
        }
        else
        {
            if (unicodeSize > 0) result = [NSString stringWithCharacters:unicodeData length:unicodeSize/2];
            else result = @"";
        }

        [filePreview release];
        if (fileIsPlainText) filePreview = [result retain];
        else filePreview = nil;
    }

    return result;
}


/** Set the file data, writing it to the device.
 *
 *  @param  data    The new file data.
 *  @param  error   Returns an error string if a problem occurs, or nil if successful. Pass NULL if not interested.
 *  @return         Logical true if the request succeeded.
 */
- (BOOL)setFileData:(NSString*)data error:(NSString**)error
{
    BOOL result = FALSE;
    if (error) *error = nil;

    fileIsSynchronised = FALSE;
    NSData* unicodeData = [data dataUsingEncoding:NSUnicodeStringEncoding allowLossyConversion:YES];
    if (!unicodeData)
    {
        if (error) *error = @"Unable to read file contents.";
    }
    else
    {
        result = fileData->importText((const uint16_t*)[unicodeData bytes], [unicodeData length]);
        if (!result)
        {
            if (error) *error = @"Incompatible file.";
        }
        else
        {
            unsigned maxFileSize = [[self appletNode] maxFileSize];
            if (fileData->fileSize() > maxFileSize)
            {
                if (error) *error = [NSString stringWithFormat:@"File is too large (maximum file size is %u bytes).", maxFileSize];
            }
            else
            {
                result = fileData->save(device, applet, fileIndex);
                if (!result && error) *error = @"Error writing file to the device.";
            }
        }
    }
    [self reload];
    return result;
}




#pragma mark    --------  ASSaveProtocol  --------


- (BOOL)saveWillBeEmpty
{
    return [self fileSize] == 0;
}


- (void)saveFileCount:(unsigned*)fileCount totalBytes:(unsigned*)totalBytes
{
    *fileCount = 1;
    *totalBytes = fileAttributes->allocSize();
}

// REVIEW: this can fail after having created a file. Need to add cleanup if this happens...
- (NSString*)saveUnderPath:(NSString*)path
{
    NSString* result = nil;

    if (![path isAbsolutePath]) return nil;     // invalid path supplied

    NSString* basename;
    NSString* extension;
    [self getFileNameBase:&basename extension:&extension];

    if (0 == [basename length]) basename = @"unspecified";     // can't make sense of device filename, so use a default

    NSString* targetFilename;
    NSFileHandle* fileHandle =  makeUniqueFile(path, basename, extension, &targetFilename);
    if (fileHandle)
    {
        NSData* data = nil;

        id accessDelegate = [self delegate];
        BOOL shouldProceed = YES;

        if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
        {
            shouldProceed = [accessDelegate deviceWillSave:self to:path sender:self];
        }

        if (shouldProceed)
        {
            NSString* stringData = [self fileData];
            NSString* error = nil;
            if (!stringData)
            {
                error = @"Unable to read the file data from the device.";
            }
            else
            {
                NSStringEncoding encoding = [ASPreferenceController getDefaultTextEncoding];
                data = [stringData dataUsingEncoding:encoding allowLossyConversion:YES];
                if (!data)
                {
                    error = @"Unable to make sense of the data from the device.";
                }
                else
                {
                    [fileHandle writeData:data];
                    result = targetFilename;
                }
            }

            if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
            {
                [accessDelegate deviceDidSave:self error:error sender:self];
            }

        }   // shouldProceed

        [fileHandle release];
        fileHandle = nil;
    }

    return result;
}









#pragma mark    --------  ASLoadProtocol  --------


- (BOOL)loadPermittedFromPath:(NSString*)path
{
    NSFileManager* fm = [NSFileManager defaultManager];
    if (![fm fileExistsAtPath:path]) return FALSE;

    switch (applet->appletID())
    {
        case kASAppletID_AlphaWord:
        case kASAppletID_Dictionary:
            if (!filePathConformsToUTI(path, @"public.plain-text")) return FALSE;
            break;

        default:
            if (!filePathConformsToUTI(path, @"public.xml")) return FALSE;
            break;
    }

    return TRUE;
}


/** Try to load the specified file.
 *
 *  @param  path        The path of the file to load.
 *  @return             Logical true if the file was loaded.
 */
- (BOOL)loadFromPath:(NSString*)path
{
    BOOL result = FALSE;

    NSError* err;
    NSStringEncoding encoding;
    NSString* contentsOfPath = [NSString stringWithContentsOfFile:path usedEncoding:&encoding error:&err];
    if (!contentsOfPath) contentsOfPath = [NSString stringWithContentsOfFile:path encoding:NSMacOSRomanStringEncoding error:&err];  // REVIEW: what encoding should be used???
    if (contentsOfPath)
    {
        BOOL shouldProceed = TRUE;

        id accessDelegate = [self delegate];
        if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
        {
            shouldProceed = [accessDelegate deviceWillReplace:self with:path sender:self];
        }

        if (shouldProceed)
        {
            NSString* error;
            result = [self setFileData:contentsOfPath error:&error];

            if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
            {
                [accessDelegate deviceDidReplace:self error:error sender:self];
            }
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




#pragma mark    --------  ASDeleteProtocol  --------

/** Return TRUE if the instance of the node can accept delete operations.
 */
- (BOOL)deletePermitted
{
    // Only permit delete for files if the applet allows it.
    ASAppletNode* appletNode = [self ancestorOfClass:[ASAppletNode class]];
    return [appletNode deletePermitted];
}


/** REVIEW: this was intended to remove the entire file. However, it is not clear if this is even
 *          possible with current Neo firmware, so it simply clears the content.
 */
- (BOOL)deleteSelf
{
    BOOL shouldProceed = TRUE;
    BOOL didDeleteFile = FALSE;

    id accessDelegate = [self delegate];
    if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
    {
        shouldProceed = [accessDelegate deviceWillDelete:self sender:self];
    }

    if (shouldProceed)
    {
        NSString* error = nil;
        [self setFileData:@"" error:&error];
        [self reload];

        if ([accessDelegate conformsToProtocol:@protocol(ASDeviceAccessProtocol)])
        {
            [accessDelegate deviceDidDelete:self error:error sender:self];
        }
    }

    return didDeleteFile;
}



@end
