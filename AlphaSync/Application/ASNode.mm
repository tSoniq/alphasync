/** @file   ASNode.mm
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
#import <limits.h>
#import "ASNode.h"


@implementation ASNode

- (id)init
{
    if ((self = [super init]))
    {
        smallIcon = nil;
        largeIcon = nil;
        displayName = @"-";
        displayKind = @"-";
        displaySize = @"0";
        fileSize = 0;
        fileSpace = 0;
        sortKey = kASNodeSortKeyName;
    }
    return self;
}


- (void)dealloc
{
    [smallIcon release];
    [largeIcon release];
    [displayName release];
    [displayKind release];
    [displaySize release];
    smallIcon = nil;
    largeIcon = nil;
    displayName = nil;
    displayKind = nil;
    displaySize = nil;
    [super dealloc];
}


/** Return the delegate for the node. If no delegate is explicitly specified, we seach upwards looking
 *  for a parent with a registed delegate and use that if found. This makes it easier to specify a single
 *  delegate for an entire tree.
 *
 *  @return         The delegate, or nil if none assigned.
 */
- (id)delegate
{
    if (delegate) return delegate;

    id parent = [self parent];
    if (parent) return [parent delegate];
    else return nil;
}


/** Set the delegate for the current node.
 */
- (void)setDelegate:(id)newDelegate
{
    if (newDelegate != delegate)
    {
        [delegate release];
        delegate = [newDelegate retain];
    }
}


/** Subclasses should override this method and use it to refresh class specific information from a device.
 *  A refresh may re-read device parameters, but should not attempt to re-enumerate children.
 *  Refreshes should propagate upwards.
 */
- (void)refresh
{
    [[self parent] refresh];
}


/** Subclasses should override this method and use it to reload class specific information from a device.
 *  A reload may re-read device parameters and should re-enumerate children.
 *  Reloads must not propagate upwards.
 *  A reload should complete by calling refresh.
 */
- (void)reload
{
    [self refresh];
}


/** Return a small instance of the icon. This caches the data and does not retain the original image.
 *  This is suitable for the outline view display.
 */
- (NSImage*)smallIcon
{
    return smallIcon;
}

/** Return a large instance of the icon. This retains the original image and keeps full display quality at all sizes.
 *  However, the performance cost of this makes it unsuitable for the outline view.
 */
- (NSImage*)largeIcon
{
    return largeIcon;
}

- (NSString*)displayName
{
    return displayName;
}

- (NSString*)displayKind
{
    return displayKind;
}

- (NSString*)displaySize
{
    return displaySize;
}

- (NSString*)displayFileSpace
{
    if (0 == fileSpace) return @"unbound"; // not bound to any key
    else return [NSString stringWithFormat:@"F%d", fileSpace];
}

- (int)fileSpace
{
    return fileSpace;
}

- (int)fileSize
{
    return fileSize;
}

- (void)setIcon:(NSImage*)value
{
    [smallIcon release];
    [largeIcon release];

    smallIcon = [value copy];
    largeIcon = [value copy];

    [largeIcon setDataRetained:YES];
    [smallIcon setDataRetained:NO];
}

- (void)setDisplayName:(NSString*)value
{
    if (value != displayName)
    {
        [displayName release];
        displayName = [value retain];
    }
}

- (void)setDisplayKind:(NSString*)value
{
    if (value != displayKind)
    {
        [displayKind release];
        displayKind = [value retain];
    }
}

- (void)setDisplaySize:(NSString*)value
{
    if (value != displaySize)
    {
        [displaySize release];
        displaySize = [value retain];
    }
}

- (void)setFileSpace:(int)value
{
    fileSpace = value;
}

- (void)setFileSize:(int)size
{
    fileSize = size;
}


/** Sort method used for the display name. Preferentially places applets with
 *  file spaces earlier in the list.
 */
static NSInteger sortFunctionDisplayName(id node1, id node2, void* context)
{
    (void)context;

    NSComparisonResult result;

    ASNode* n1 = (ASNode*)node1;
    ASNode* n2 = (ASNode*)node2;

    NSString* name1 = [n1 displayName];
    NSString* name2 = [n1 displayName];
    int fileSpace1 = [n1 fileSpace];
    int fileSpace2 = [n2 fileSpace];
    int space1 = (fileSpace1 == 0) ? INT_MAX : fileSpace1;
    int space2 = (fileSpace2 == 0) ? INT_MAX : fileSpace2;

    if (space1 < space2) result = NSOrderedAscending;
    else if (space1 > space2) result = NSOrderedDescending;
    else result = [name1 caseInsensitiveCompare:name2];

    return result;
}


/** Sort method used for the size field.
 */
static NSInteger sortFunctionDisplaySize(id node1, id node2, void* context)
{
    ASNode* n1 = (ASNode*)node1;
    ASNode* n2 = (ASNode*)node2;
    int size1 = [n1 fileSize];
    int size2 = [n2 fileSize];
    if (size1 < size2) return NSOrderedAscending;
    else if (size1 > size2) return NSOrderedDescending;
    else return sortFunctionDisplayName(node1, node2, context);
}


/** Sort method for the object kind.
 */
static NSInteger sortFunctionDisplayKind(id node1, id node2, void* context)
{
    ASNode* n1 = (ASNode*)node1;
    ASNode* n2 = (ASNode*)node2;
    NSString* kind1 = [n1 displayKind];
    NSString* kind2 = [n2 displayKind];
    NSComparisonResult result = [kind1 caseInsensitiveCompare:kind2];
    if (result == NSOrderedSame) result = (NSComparisonResult)sortFunctionDisplayName(node1, node2, context);
    return result;
}


/** Set the sort key.
 */
- (void)setSortKey:(int)key
{
    assert(kASNodeSortKeyName == key || kASNodeSortKeyKind == key || kASNodeSortKeySize == key);
    sortKey = key;
}


/** Sort the children of a node. Unless overridden, this sorts by the display name, using case insensitive matching.
 */
- (void)sortChildren
{
    switch (sortKey)
    {
        case kASNodeSortKeySize:
            [childNodes sortUsingFunction:sortFunctionDisplaySize context:nil];
            break;

        case kASNodeSortKeyKind:
            [childNodes sortUsingFunction:sortFunctionDisplayKind context:nil];
            break;

        case kASNodeSortKeyName:
        default:
            [childNodes sortUsingFunction:sortFunctionDisplayName context:nil];
            break;
    }

    NSEnumerator* enumerator = [childNodes objectEnumerator];
    ASNode* node;
    while ((node = [enumerator nextObject]))
    {
        [node sortChildren];
    }
}



#pragma mark ASSaveProtocol


- (BOOL)saveWillBeEmpty
{
    // We could also test if the children have any files. The current code means that a dragged applet
    // that contains several empty files will not be treated as empty.
    return (0 == [self numberOfChildren]);
}


- (void)saveFileCount:(unsigned*)fileCount totalBytes:(unsigned*)totalBytes
{
    unsigned files = 0;
    unsigned bytes = 0;

    NSEnumerator* enumerator = [self childEnumerator];
    id node;
    while ((node = [enumerator nextObject]))
    {
        unsigned childFiles;
        unsigned childBytes;
        if ([node conformsToProtocol:@protocol(ASSaveProtocol)])
        {
            [node saveFileCount:&childFiles totalBytes:&childBytes];
            files += childFiles;
            bytes += childBytes;
        }
    }

    *fileCount = files;
    *totalBytes = bytes;
}


/** The generic node implements a generic version of the save protocol that interprets the node
 *  as a 'directory' in a file system tree. The save method will create a directory using the node's
 *  display name and subsequently recursing for any contained nodes.
 *
 *  @param  path    The root path (must be an absolute path).
 *  @return         The name of the saved object (efectively the concatenation of the root path and
 *                  the node name), or nil if failed. This method will return failure if there are
 *                  no children present (there is no point in creating unneeded empty directories).
 */
- (NSString*)saveUnderPath:(NSString*)path
{
    if ([self saveWillBeEmpty]) return nil;

    NSString* name = [self displayName];
    if (!name) return nil;

    NSString* directoryName = makeFilesystemPath(path, [self displayName]);
    if (!directoryName) return nil;

    NSEnumerator* enumerator = [self childEnumerator];
    id node;
    while ((node = [enumerator nextObject]))
    {
        if ([node conformsToProtocol:@protocol(ASSaveProtocol)])  [node saveUnderPath:directoryName];
    }

    return directoryName;
}


- (ASNode*)parent
{
    id object = [super parent];
    if ([object isKindOfClass:[ASNode class]]) return object;
    else return nil;
}



@end
