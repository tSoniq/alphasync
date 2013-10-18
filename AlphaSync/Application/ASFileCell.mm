/** @file       ASFileCell.cc
 *  @brief
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
#include <assert.h>
#include "ASFileCell.h"

@implementation ASFileCell

- (void)dealloc
{
    [icon release];
    icon = nil;
    [super dealloc];
}

- (id)copyWithZone:(NSZone*)zone
{
    ASFileCell* cell = (ASFileCell*)[super copyWithZone:zone];
    cell->icon = [icon retain];
    return cell;
}

- (void)setIcon:(NSImage*)icn
{
    if (icn != icon)
    {
        [icon release];
        icon = [icn retain];
    }
}

- (NSImage*)icon
{
    return icon;
}

- (NSRect)imageFrameForCellFrame:(NSRect)cellFrame
{
    if (icon)
    {
        NSRect imageFrame;
        imageFrame.size = [icon size];
        imageFrame.origin = cellFrame.origin;
        imageFrame.origin.x += 3;
        imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);
        return imageFrame;
    }
    else
    {
        return NSZeroRect;
    }
}

- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [icon size].width, NSMinXEdge);
    [super editWithFrame: textFrame inView: controlView editor:textObj delegate:anObject event: theEvent];
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView*)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, 3 + [icon size].width, NSMinXEdge);
    [super selectWithFrame: textFrame inView: controlView editor:textObj delegate:anObject start:selStart length:selLength];
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView
{
    if (icon)
    {
        NSRect imageFrame;

        NSSize iconSize = [icon size];

        float availableHeight = cellFrame.size.height - 3.0;
        float iconAspectRatio = (iconSize.height > 0) ? (iconSize.width / iconSize.height) : 1.0;
        float iconWidth = (iconAspectRatio >= 1.0) ? (availableHeight * iconAspectRatio) : (availableHeight);
        float iconHeight = (iconAspectRatio > 0.0) ? (iconWidth / iconAspectRatio) : 0.0;

        // Split the display cell in to two horizontal parts, for icon and text respectively.
        NSDivideRect(cellFrame, &imageFrame, &cellFrame, iconWidth + 4.0, NSMinXEdge);
        if ([self drawsBackground])
        {
            [[self backgroundColor] set];
            NSRectFill(imageFrame);
        }
        imageFrame.origin.x += 2.0;
        imageFrame.size.height = iconHeight;
        imageFrame.size.width = iconWidth;

        if ([controlView isFlipped])    imageFrame.origin.y += ceil((cellFrame.size.height + imageFrame.size.height) / 2);
        else                            imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);

    	[icon setScalesWhenResized:YES];
        [icon setSize:imageFrame.size];
        [icon compositeToPoint:imageFrame.origin operation:NSCompositeSourceOver];
    }
    [super drawWithFrame:cellFrame inView:controlView];
}

- (NSSize)cellSize
{
    NSSize cellSize = [super cellSize];
    cellSize.width += (icon ? [icon size].width : 0) + 3;
    return cellSize;
}

@end

