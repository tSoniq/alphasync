/** @file   ASLoadSaveProtocol.mm
 *  @brief  Protocol used for objects that can save themselves to local disk. This is used
 *          by ASNode derivatives to write themselves and their children to disk.
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
#import "ASLoadSaveProtocol.h"

#define kMaxUniqueRetry (999)       /**< Try this many times to create a unique name, before giving up. */


/** Helper function used to construct directory paths. This may be useful to classes
 *  that implement ASSaveProtocol. The path is created on disk and its name is returned.
 *
 *  @bug    Needs to clean up any partially created directories if a late failure occurs. In practise this
 *          is very unlikely (eg: it would need the disk to become full partway through creating the path).
 *
 *  @param  rootPath        The base path. This must be an absolute path name and must exist for this
 *                          function to complete successfully.
 *  @param  extendedPath    The path extension. This is appended to the root path. If the requested
 *                          components already exist, their names will be modified to ensure that a
 *                          new unique path component results. Can pass nil if no extendedPath is required.
 *  @return                 The actual path that resulted (autorelease string), or nil if failed.
 */
NSString* makeFilesystemPath(NSString* rootPath, NSString* extendedPath)
{
    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL isDir;

    if (![rootPath isAbsolutePath]) return nil;                                     // root path must be absolute
    if (extendedPath && [extendedPath isAbsolutePath]) return nil;                  // extended path must not be absolute
    if (![fm fileExistsAtPath:rootPath isDirectory:&isDir] || !isDir) return nil;   // root path does not exist or is a file

	NSString* path = rootPath;

	NSArray* extendedPathComponents = [extendedPath pathComponents];
    NSEnumerator* enumerator = [extendedPathComponents objectEnumerator];
    NSString* component;
    while ((component = [enumerator nextObject]))
    {
        int uniqueID = 1;
        path = [rootPath stringByAppendingPathComponent:component];

        while ([fm fileExistsAtPath:path] || ![fm createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:nil])
        {
            if (uniqueID > kMaxUniqueRetry) return nil;             // bailing out - unable to make a unique name?
            NSString* uniqueSuffix = [NSString stringWithFormat:@"%@ (%d)", component, uniqueID];
            path = [rootPath stringByAppendingPathComponent:uniqueSuffix];
            uniqueID ++;
        }
    }

    return path;
}



/** Create a new file, using a unique filename. If successful, the result is an empty file on disk
 *  which is opened for writing by using the returned file handle.
 *
 *  @param  path        The directory in which to create the file.
 *  @param  basename    The name for the file, excluding the extension.
 *  @param  extension   The file extension (eg: @"txt").
 *  @param  oFilename   Returns the resulting filename.
 *  @return             If successful, a file handle that can write to the file, or nil if failed.
 */
NSFileHandle* makeUniqueFile(NSString* path, NSString* basename, NSString* extension, NSString** oFilename)
{
    NSString* filename = nil;
    int fdForWriting = -1;
    int uniqueID = 0;
    do
    {
        filename = [NSString stringWithFormat:@"%@%@.%@", basename, (uniqueID ? [NSString stringWithFormat:@" (%d)", uniqueID] : @""), extension];
        fdForWriting = open([[NSString stringWithFormat:@"%@/%@", path, filename] UTF8String], O_WRONLY | O_CREAT | O_EXCL, 0666);
        uniqueID ++;
    } while (fdForWriting < 0 && errno == EEXIST && uniqueID <= kMaxUniqueRetry);

    NSFileHandle *fileHandle = nil;
    if (fdForWriting > 0)
    {
        fileHandle = [[NSFileHandle alloc] initWithFileDescriptor:fdForWriting closeOnDealloc:YES];
    }

    if (oFilename) *oFilename = (fileHandle ? filename : nil);
    return fileHandle;
}



/** Given a file path, obtain its UTI description.
 *
 *  @param  filePath    The file to query.
 *  @return             The UTI string. Returns an empty string if the filePath could not be queried.
 */
NSString* utiFromFilePath(NSString* filePath)
{
    NSString* utiString = @"";

    FSRef fileRef;
    Boolean isDirectory;

    if (FSPathMakeRef((const UInt8 *)[filePath fileSystemRepresentation], &fileRef, &isDirectory) == noErr)
    {
        // get the content type (UTI) of this file
        CFDictionaryRef values = NULL;
        CFStringRef attrs[1] = { kLSItemContentType };
        CFArrayRef attrNames = CFArrayCreate(NULL, (const void **)attrs, 1, NULL);

        if (LSCopyItemAttributes(&fileRef, kLSRolesViewer, attrNames, &values) == noErr)
        {
            // verify that this is a file that we can use
            if (values != NULL)
            {
                CFTypeRef uti = (CFStringRef)CFDictionaryGetValue(values, kLSItemContentType);
                if (uti != NULL)
                {
                    utiString = [NSString stringWithString:(NSString*)uti];
                }
                CFRelease(values);
            }
        }

        CFRelease(attrNames);
    }

    return utiString;
}


/** Return logical true if a given file path confirms to a UTI.
 *
 *  @param  filePath    The path.
 *  @param  uti         The type to check.
 *  @return             Logical true if the file conforms.
 */
BOOL filePathConformsToUTI(NSString* filePath, NSString* uti)
{
    NSString* filePathUTI = utiFromFilePath(filePath);
    if (UTTypeConformsTo((CFStringRef)filePathUTI, (CFStringRef)uti)) return TRUE;
    else return FALSE;
}

