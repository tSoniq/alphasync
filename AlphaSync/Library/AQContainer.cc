/** @file   AQContainer.cc
 *  @brief
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
#include <stdlib.h>
#include <string.h>
#include "AQContainer.h"


#define kContainerInitialCapacity   (64)    /**< Initial capacity for the object. */
#define kContainerGrowth            (256)   /**< Increase the memory allocation by this much whenever increasing the memory allocation. */


namespace ts
{
    /** Class constructor.
     */
    AQUntypedContainer::AQUntypedContainer()
        :
        m_itemStore(0),
        m_itemCount(0),
        m_itemStoreMaxCount(0)
    {
        setCapacity(kContainerInitialCapacity);
    }


    /** Class destructor.
     */
    AQUntypedContainer::~AQUntypedContainer()
    {
        if (0 != m_itemStoreMaxCount)
        {
            free(m_itemStore);
            m_itemStore = 0;
            m_itemCount = 0;
            m_itemStoreMaxCount = 0;
        }
    }


    /** Try to set the storage capacity of the array.
     *
     *  @param  cnt     The number of items that the container can contain. The actual allocation
     *                  will be set to the larger of @e count and the number of items currently
     *                  in the container (ie: calling this method is just a hint to allocate or
     *                  release storage - it will not change the content of the container).
     */
    void AQUntypedContainer::setCapacity(unsigned cnt)
    {
        unsigned newMaxCount = (cnt > m_itemStoreMaxCount) ? cnt : m_itemStoreMaxCount;
        void** newItemStore = (void**) realloc(m_itemStore, newMaxCount * sizeof (void*));
        if (0 != newItemStore)
        {
            m_itemStore = newItemStore;
            m_itemStoreMaxCount = newMaxCount;
        }
    }


    /** Return the number of items stored.
     *
     *  @return         The number of items present.
     */
    unsigned AQUntypedContainer::count() const
    {
        return m_itemCount;
    }


    /** Check if the container holds at least one reference to the item.
     *
     *  @param  item    The item to look for.
     *  @return         Logical true if the item is present.
     */
    bool AQUntypedContainer::containsItem(const void* item) const
    {
        void** search = m_itemStore;
        void** end = m_itemStore + m_itemCount;

        while (search < end)
        {
            if (*search == item) return true;
            search ++;
        }

        return false;
    }


    /** Count the number of instances of an item in the container.
     *
     *  @param  item    The item to look for.
     *  @return         The number of instances present.
     */
    unsigned AQUntypedContainer::countOfItem(const void* item) const
    {
        unsigned cnt = 0;
        void** search = m_itemStore;
        void** end = m_itemStore + m_itemCount;

        while (search != end)
        {
            if (*search == item) ++ cnt;
            search ++;
        }

        return cnt;
    }


    /** Return the item at the specified index.
     *
     *  @param  index   The index of the item. Use @e count() to find the number of items present.
     *  @return         The item at the specified index, or zero if the index is out of range.
     */
    void* AQUntypedContainer::itemAtIndex(unsigned index) const
    {
        if (index < m_itemCount) return m_itemStore[index];
        else return 0;
    }


    /** Remove all occurences (if any) of the specified item. This can be quite slow for large
     *  containers as it requires a memory copy for each item removed.
     *
     *  @param  item    The item to be removed.
     */
    void AQUntypedContainer::removeItem(const void* item)
    {
        unsigned index = 0;

        while (index != m_itemCount)
        {
            if (m_itemStore[index] == item)
            {
                removeItemAtIndex(index);
            }
            else
            {
                index ++;
            }
        }
    }


    /** Remove an item at the specified index.
     *
     *  @param  index   The index of the item to remove.
     */
    void AQUntypedContainer::removeItemAtIndex(unsigned index)
    {
        if (index < m_itemCount)
        {
            if (index + 1 < m_itemCount)
            {
                unsigned itemsToMove = m_itemCount - index - 1;
                memmove(&m_itemStore[index], &m_itemStore[index + 1], itemsToMove*sizeof(void*));
            }
            m_itemCount --;
        }
    }


    /** Append an item to the container.
     *
     *  @param  item    The item to add. This may be any pointer, including zero. The same item may legitimately
     *                  be added several times to the container.
     *  @return         Logical true if the item could be added.
     */
    bool AQUntypedContainer::appendItem(const void* item)
    {
        if (m_itemCount + 1 > m_itemStoreMaxCount)  setCapacity(m_itemStoreMaxCount + kContainerGrowth);
        if (m_itemCount + 1 > m_itemStoreMaxCount)  return false;   // Unable to allocate memory

        m_itemStore[m_itemCount ++] = (void*)item;  // stricly, this is violating const correctness...
        return true;
    }


    /** Remove all items in the container. This will not necessarily release any allocated memory: use
     *  setCapacity() to reduce the memory footprint if necessary.
     */
    void AQUntypedContainer::removeAllItems()
    {
        m_itemCount = 0;
    }





}   // namespace
