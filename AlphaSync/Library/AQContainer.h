/** @file   AQContainer.h
 *  @brief  Generic object container class, using dynamic memory allocation.
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

#ifndef COM_TSONIQ_AQContainer_H
#define COM_TSONIQ_AQContainer_H   (1)

namespace ts
{
    /** Container implementation for void* pointers. This is used as the underlying base class for the
     *  type safe AQContainer implementation (to avoid code bloat). It is not recommended that this be
     *  used directly.
     */
    class AQUntypedContainer
    {
    public:

        AQUntypedContainer();
        virtual ~AQUntypedContainer();

        void setCapacity(unsigned cnt);

        unsigned count() const;
        unsigned countOfItem(const void* item) const;
        bool containsItem(const void* item) const;

        void removeItem(const void* item);
        void removeItemAtIndex(unsigned index);
        void removeAllItems();

        bool appendItem(const void* item);

        void* itemAtIndex(unsigned index) const;

    private:
        void** m_itemStore;
        unsigned m_itemCount;
        unsigned m_itemStoreMaxCount;

        AQUntypedContainer(const AQUntypedContainer&);              /**< Prevent the use of the copy constructor. */
        AQUntypedContainer& operator=(const AQUntypedContainer&);   /**< Prevent the use of the assignment operator. */
    };

    /** Class used to store an ordered collection of pointers. The storage expands automatically as
     *  new items are added. The client is responsible for ensuring that pointers that are contained
     *  are valid.
     *
     *  @param  T   The type of object being pointed to.
     */
    template <typename T> class AQContainer : public AQUntypedContainer
    {
    public:

        AQContainer<T>() : AQUntypedContainer() { }

        unsigned countOfItem(const T* item) const { return AQUntypedContainer::countOfItem((const void*) item); }
        bool containsItem(const T* item) const { return AQUntypedContainer::containsItem((const void*) item); }

        void removeItem(const T* item) { AQUntypedContainer::removeItem((const void*) item); }
        void removeItemAtIndex(unsigned index) { AQUntypedContainer::removeItemAtIndex(index); }

        bool appendItem(const T* item) { return AQUntypedContainer::appendItem((const void*) item); }

        T* itemAtIndex(unsigned index) const { return (T*) AQUntypedContainer::itemAtIndex(index); }

    private:

        AQContainer<T>(const AQContainer<T>&);              /**< Prevent the use of the copy constructor. */
        AQContainer<T>& operator=(const AQContainer<T>&);   /**< Prevent the use of the assignment operator. */
    };

}   // namespace

#endif      // COM_TSONIQ_AQContainer_H
