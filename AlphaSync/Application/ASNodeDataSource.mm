/** @file   ASNodeDataSource.mm
 *  @brief
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

#import "ASNodeDataSource.h"
#import "ASNode.h"
#import "ASDeviceNode.h"
#import "ASAppletNode.h"
#import "ASFileNode.h"
#import "ASDevice.h"
#import "ASApplet.h"
#import "ASFileAttributes.h"
#import "ASFile.h"
#import "ASAlphaWordFile.h"
#import "ASUserDictionaryFile.h"


using namespace ts;


@implementation ASNodeDataSource


- (id)init
{
    if ((self = [super init]))
    {
        draggedNodes = nil;
    }
    return self;
}


- (void)dealloc
{
    [draggedNodes release];
    [super dealloc];
}


- (void)reloadData:(NSOutlineView*)view
{
    [view reloadData];
    NSEnumerator* deviceEnumerator = [[deviceController systemRoot] childEnumerator];
    ASNode* item;
    while ((item = [deviceEnumerator nextObject]))
    {
        [view expandItem:item expandChildren:YES];
    }
}


- (void)setDeviceController:(ASDeviceController*)controller
{
    if (deviceController != controller)
    {
        [deviceController release];
        deviceController = [controller retain];
    }
}




#pragma mark -
#pragma mark Methods that cause changes to the nodes (and hence device data)


- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
    (void)outlineView;
	ASNode* node = (item == nil) ? [deviceController systemRoot] : item;
	return (NSInteger)[node numberOfChildren];
}


- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item
{
    (void)outlineView;
	ASNode* node = (item == nil) ? [deviceController systemRoot] : item;
	return [node childAtIndex:(NSUInteger)index];
}


- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
    (void)outlineView;
	return ([item numberOfChildren] > 0);
}


- (id)outlineView:(NSOutlineView*)outlineView objectValueForTableColumn:(NSTableColumn*)tableColumn byItem:(id)item
{
    (void)outlineView;
	id result;

	NSString* key = [tableColumn identifier];
    if (NSOrderedSame == [key compare:@"Name"]) result = [item displayName];
    else if (NSOrderedSame == [key compare:@"Size"]) result = [item displaySize];
    else if (NSOrderedSame == [key compare:@"Kind"]) result = [item displayKind];
    else { assert(0); result = @"?"; }  // unknown display column

	return result;
}


- (void)outlineView:(NSOutlineView*)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn*)tableColumn byItem:(id)item
{
    (void)outlineView;
    (void)tableColumn;
    if ([item isMemberOfClass:[ASFileNode class]])
    {
        ASFileNode* node = (ASFileNode*)item;
        [node setFileName:object];
        [outlineView reloadData];
    }
}


- (BOOL)outlineView:(NSOutlineView*)outlineView isGroupItem:(id)item
{
    (void)outlineView;
    (void) item;
	return NO;
}


/** Handler for items dragged out of the view.
 */
- (NSArray*) outlineView:(NSOutlineView*)outlineView
             namesOfPromisedFilesDroppedAtDestination:(NSURL*)dropDestination
             forDraggedItems:(NSArray*)items
{
    (void)outlineView;

    NSMutableArray* filenames = [NSMutableArray array];
    NSEnumerator* enumerator = [items objectEnumerator];
    ASNode* item;
    while ((item = [enumerator nextObject]))
    {
        if ([item conformsToProtocol:@protocol(ASSaveProtocol)])
        {
            NSString* filename = [item saveUnderPath:[dropDestination path]];
            if (filename) [filenames addObject:filename];
        }
    }
    return ([filenames count] ? filenames : nil);
}


- (BOOL)outlineView:(NSOutlineView*)outlineView writeItems:(NSArray*)items toPasteboard:(NSPasteboard*)pboard
{
    (void)outlineView;
    (void)items;
    draggedNodes = [items retain];  // REVIEW: should this be retained? How long?
    [pboard declareTypes:[NSArray arrayWithObjects:NSStringPboardType, NSFilesPromisePboardType, nil] owner:self];
    [pboard setPropertyList:[NSArray arrayWithObjects:@"txt", nil] forType:NSFilesPromisePboardType];
    return YES;
}


- (void)pasteboard:(NSPasteboard*)pboard provideDataForType:(NSString*)type
{
    (void)type;
    NSString* dataString = @"";
    for (unsigned i = 0; i < [draggedNodes count]; i++)
    {
        ASFileNode* node = [draggedNodes objectAtIndex:i];
        if ([node isMemberOfClass:[ASFileNode class]])
        {
            NSString* text = [node fileData];
            if (text) dataString = [dataString stringByAppendingString:text];
        }
    }
    [draggedNodes release];
    [pboard setString:dataString forType:NSStringPboardType];
}




- (NSDragOperation)outlineView:(NSOutlineView*)outlineView validateDrop:(id<NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)childIndex
{
    (void)outlineView;
    (void)childIndex;

    NSDragOperation result = NSDragOperationNone;

    NSPasteboard* pasteboard = [info draggingPasteboard];
    NSArray* typeArray = [pasteboard types];
    if ([typeArray containsObject:NSFilenamesPboardType])
    {
        NSArray* filePaths = [pasteboard propertyListForType:NSFilenamesPboardType];
        if ([item conformsToProtocol:@protocol(ASLoadProtocol)] && [item loadPermittedFromPaths:filePaths])
        {
            result = NSDragOperationCopy;
        }
    }
    return result;
}


- (BOOL)outlineView:(NSOutlineView*)outlineView acceptDrop:(id<NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)childIndex
{
    (void)childIndex;

    BOOL result = FALSE;

    NSPasteboard* pasteboard = [info draggingPasteboard];
    NSArray* typeArray = [pasteboard types];
    if ([typeArray containsObject:NSFilenamesPboardType])
    {
        NSArray* filePaths = [pasteboard propertyListForType:NSFilenamesPboardType];
        if ([item conformsToProtocol:@protocol(ASLoadProtocol)] && [item loadPermittedFromPaths:filePaths])
        {
            result = ([item loadFromPaths:filePaths] > 0);      // return success if at least one file was loaded successfully
            if (result)
            {
                [item reload];
                [outlineView reloadData];
            }
        }
    }

    return result;
}

@end



