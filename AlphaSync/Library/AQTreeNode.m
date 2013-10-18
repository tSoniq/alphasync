/** @file       AQTreeNode.m
 *  @brief      Generic tree structure.
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
#include "AQTreeNode.h"

@implementation AQTreeNode


/** Class method that, given an array of ASTreeNode objects, returns the minimum set of covering nodes.
 *
 *  @param  nodes       Array of source nodes.
 *  @return             An array of covering nodes (ie: the members of @e nodes that are not themselves a descendant
 *                      of a member of @e nodes).
 */
+ (NSArray*)coverNodesFromArray:(NSArray*)nodes
{
    NSUInteger inputNodeCount = [nodes count];
    NSMutableArray* result = [NSMutableArray arrayWithCapacity:inputNodeCount];
    for (NSUInteger i = 0; i != inputNodeCount; i++)
    {
        AQTreeNode* candidate = [nodes objectAtIndex:i];
        for (NSUInteger j = i + 1; candidate && j != inputNodeCount; j++)
        {
            if ([candidate isChildOf:[nodes objectAtIndex:j]]) candidate = nil;
        }

        if (candidate) [result addObject:candidate];
    }

    return result;
}


- (id)init
{
    if ((self = [super init]))
    {
        parentNode = nil;
        childNodes = [[NSMutableArray alloc] initWithCapacity:16];
    }
    return self;
}


- (void)dealloc
{
    [childNodes release];
    [super dealloc];
}


/** Return the parent node.
 */
- (AQTreeNode*)parent
{
    return parentNode;
}


/** Set the parent node. This just sets the parent reference - the parent node's child list is not
 *  updated.
 */
- (void)setParent:(AQTreeNode*)newParent
{
    parentNode = newParent;
}


/** Return an array containing all the children. This creates a local copy of the
 *  array data.
 */
- (NSArray*)children
{
    return [NSArray arrayWithArray:childNodes];
}


/** Return the number of children.
 */
- (NSUInteger)numberOfChildren
{
    return [childNodes count];
}


/** Return the n'th child, or nil if the index is out of range.
 */
- (AQTreeNode*)childAtIndex:(NSUInteger)idx
{
    if (idx < [childNodes count]) return [childNodes objectAtIndex:idx];
    else return nil;
}


/** Return an enumerator for the children.
 */
- (NSEnumerator*)childEnumerator
{
    return [childNodes objectEnumerator];
}


/** Make the specified node a child of this parent.
 */
- (void)addChild:(AQTreeNode*)child
{
    AQTreeNode* oldParent = [child parent];
    if (oldParent == self)
    {
        // Child is already assigned to this parent.
        assert([childNodes containsObject:child]);
    }
    else
    {
        // (re)assign child to this parent.
        if (oldParent) [oldParent removeChild:child];
        [child setParent:self];
        [childNodes addObject:child];
    }
}


/** Remove the specified child.
 */
- (void)removeChild:(AQTreeNode*)child
{
    if ([child parent] == self)
    {
        assert([childNodes containsObject:child]);
        [child setParent:nil];
        [childNodes removeObjectIdenticalTo:child];
    }
}


/** Remove all children from this node.
 */
- (void)removeAllChildren
{
    AQTreeNode* child;
    while ((child = [childNodes lastObject]))
    {
        assert([[child class] isSubclassOfClass:[AQTreeNode class]]);
        [self removeChild:child];
    }
}


/** Sort using the specified sort function.
 */
- (void)sortChildrenUsingFunction:(NSInteger (*)(id, id, void*))sortFunction
{
    [childNodes sortUsingFunction:sortFunction context:0];
}


/** Traverse upwards through the tree until the first instance of the specified class is
 *  found.
 *
 *  @param  aClass  The class type to find.
 *  @return         A pointer to the first parent that matches the class type, or nil if not found.
 *                  The search will match the current object if its class satisfies aClass.
 */
- (id)ancestorOfClass:(Class)aClass
{
    AQTreeNode* node = self;
    while (node && ![node isMemberOfClass:aClass])
    {
        node = [node parent];
    }
    return node;
}

/** Return logical true if this node is a parent of @e candidate.
 */
- (BOOL)isParentOf:(AQTreeNode*)candidate
{
    AQTreeNode* search = [candidate parent];
    while (search && search != self) search = [search parent];
    return (0 != search);
}


/** Return logical true if this node is a child of @e candidate.
 */
- (BOOL)isChildOf:(AQTreeNode*)candidate;
{
    AQTreeNode* search = [self parent];
    while (search && search != candidate) search = [search parent];
    return (0 != search);
}


/** Description for the node. This recurses down the tree to show all descendants.
 */
- (NSString*)descriptionWithPrefix:(NSString*)prefixString
{
    NSString* classDescription = [[self class] description];
    NSString* myDescription = [NSString stringWithFormat:@"%@ASTreeNode @%p: %@", prefixString, self, classDescription];

    prefixString = [NSString stringWithFormat:@"--%@", prefixString];
    NSEnumerator* enumerator = [self childEnumerator];
    AQTreeNode* node;
    while ((node = [enumerator nextObject]))
    {
        NSString* siblingDescription = [node descriptionWithPrefix:prefixString];
        myDescription = [NSString stringWithFormat:@"%@\n%@", myDescription, siblingDescription];
    }
    return myDescription;
}


/** Description for the node.
 */
- (NSString*)description
{
    return [self descriptionWithPrefix:@""];
}

@end
