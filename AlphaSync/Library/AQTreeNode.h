/** @file       AQTreeNode.h
 *  @brief      Generic tree structure. Tree nodes contain no user data. Derive a custom object
 *              from the tree node to provide some useful content.
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

@interface AQTreeNode : NSObject
{
    AQTreeNode* parentNode;
    NSMutableArray* childNodes;
}

+ (NSArray*)coverNodesFromArray:(NSArray*)nodes;

- (id)init;

- (AQTreeNode*)parent;

- (NSArray*)children;
- (NSUInteger)numberOfChildren;
- (AQTreeNode*)childAtIndex:(NSUInteger)index;
- (void)addChild:(AQTreeNode*)child;
- (void)removeChild:(AQTreeNode*)child;
- (void)removeAllChildren;
- (void)sortChildrenUsingFunction:(NSInteger (*)(id, id, void*))sortFunction;

- (NSEnumerator*)childEnumerator;

- (id)ancestorOfClass:(Class)aClass;

- (BOOL)isParentOf:(AQTreeNode*)candidate;
- (BOOL)isChildOf:(AQTreeNode*)candidate;

- (NSString*)description;

@end
