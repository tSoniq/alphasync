/** @file   ASLoadSaveProtocol.h
 *  @brief  Protocol used for objects that can load or save themselves to local disk. This is used
 *          by ASNode derivatives to write themselves and their children to disk.
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
#import <Cocoa/Cocoa.h>


@protocol ASLoadProtocol

/** Test if a file can be loaded. This may be rejected if, for example, the file type is not appropriate.
 *  Note: this is intended to be a quick test based on the path. Even if this method succeeds, a subsequent
 *  call to loadFromPath may fail if the content of the file is invalid.
 *
 *  @param  path        The path of the file to load.
 *  @return             Logical true if the file can be loaded.
 */
- (BOOL)loadPermittedFromPath:(NSString*)path;


/** Try to load the specified file.
 *
 *  @param  path        The path of the file to load.
 *  @return             Logical true if the file was loaded.
 */
- (BOOL)loadFromPath:(NSString*)path;


/** Test if all files in an array may potentially be loaded.
 *
 *  @param  paths       Array of path strings.
 *  @return             Logical true if all paths in the array can be loaded.
 */
- (BOOL)loadPermittedFromPaths:(NSArray*)paths;


/** Try to load all files from an array of paths.
 *
 *  @param  paths       Array of path strings.
 *  @return             The number of items that were loaded.
 */
- (unsigned)loadFromPaths:(NSArray*)paths;

@end




@protocol ASSaveProtocol

/** Test if a call to saveUnderPath would result in an empty (zero-length) file.
 *
 *  @return             Logical true if a file created for this object would be empty.
 */
- (BOOL)saveWillBeEmpty;


/** Calculate the total number of files that will be created and their size.
 *  Directories are not included in the figures returned.
 */
- (void)saveFileCount:(unsigned*)fileCount totalBytes:(unsigned*)totalBytes;


/** Save the referenced file from the device to a local filesystem.
 *
 *  @param  path        The directory path in which to put the file. The path must already exist. The name of the
 *                      file is determined from the Neo filename, is is ammended to ensure that no existing file
 *                      will be overwritten.
 *  @return             The filename that was used, or nil if failed.
 */
- (NSString*)saveUnderPath:(NSString*)path;


@end



@protocol ASDeleteProtocol

/** Return TRUE if the instance of the node can accept delete operations.
 */
- (BOOL)deletePermitted;

/** Delete the referenced object.
 */
- (BOOL)deleteSelf;

@end



// REVIEW: these should probably be moved in to a generic file system utilities library...
extern NSString* makeFilesystemPath(NSString* rootPath, NSString* extendedPath);
extern NSFileHandle* makeUniqueFile(NSString* path, NSString* basename, NSString* extension, NSString** oFilename);
extern NSString* utiFromFilePath(NSString* filePath);
extern BOOL filePathConformsToUTI(NSString* filePath, NSString* uti);


