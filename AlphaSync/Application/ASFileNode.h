/** @file   ASFileNode.h
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
#import "Cocoa/Cocoa.h"
#import "ASDevice.h"
#import "ASApplet.h"
#import "ASFileAttributes.h"
#import "ASFile.h"
#import "ASNode.h"
#import "ASAppletNode.h"
#import "ASLoadSaveProtocol.h"

@interface ASFileNode : ASNode <ASLoadProtocol, ASSaveProtocol, ASDeleteProtocol>
{
    BOOL isFirstRefresh;
    int fileIndex;
    ts::ASDevice* device;
    const ts::ASApplet* applet;
    ts::ASFileAttributes* fileAttributes;
    ts::ASFile* fileData;
    NSString* filePreview;
    NSString* fileExtension;
    BOOL fileIsSynchronised;
    BOOL fileIsPlainText;
}

+ (ASFileNode*)createFileNodeOnDevice:(ts::ASDevice*)device applet:(const ts::ASApplet*)applet filename:(NSString*)filename data:(const void*)data size:(unsigned)size;
+ (ASFileNode*)createFileNodeOnDevice:(ts::ASDevice*)device applet:(const ts::ASApplet*)applet path:(NSString*)path;

- (id)initWithDevice:(ts::ASDevice*)devicePtr applet:(const ts::ASApplet*)app fileIndex:(int)fin fileAttributes:(const ts::ASFileAttributes*)attr;
- (void)dealloc;

- (int)fileIndex;
- (ASAppletNode*)appletNode;
- (const ts::ASFileAttributes*)fileAttributes;
- (NSString*)password;
- (NSString*)setFileName:(NSString*)newName;
- (void)getFileNameBase:(NSString**)basename extension:(NSString**)extension;

- (NSString*)filePreview;

- (BOOL)fileIsPlainText;
- (NSString*)fileData;
- (BOOL)setFileData:(NSString*)data error:(NSString**)error;

@end
