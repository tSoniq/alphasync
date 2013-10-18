/** @file       ASSettings.cc
 *  @brief      Settings tuple management.
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
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "ASEndian.h"
#include "ASSettings.h"

namespace ts
{
#pragma mark -
#pragma mark ASSettings : Public Methods

    /** Constructor, building a writable data object.
     *
     *  @param  data            A pointer to the settings data.
     *  @param  dataSize        The size of the data in the buffer (logical size), in bytes.
     *  @param  allocationSize  The size of the data buffer itself (allocation size), in bytes.
     */
    ASSettings::ASSettings(void* data, unsigned dataSize, unsigned allocationSize)
        :
        m_bufferStart((uint8_t*)data),
        m_bufferDataLength(dataSize),
        m_bufferDataAllocation(allocationSize),
        m_newItem(0)
    {
        assert(m_bufferDataAllocation >= m_bufferDataLength);
    }


    /** Return the current buffer start address.
     */
    uint8_t* ASSettings::buffer()
    {
        return m_bufferStart;
    }


    /** Return the current logical size (number of valid data bytes present).
     */
    unsigned ASSettings::size()
    {
        return m_bufferDataLength;
    }


    /** Return the available space.
     */
    unsigned ASSettings::space()
    {
        assert(m_bufferDataAllocation >= m_bufferDataLength);
        return m_bufferDataAllocation - m_bufferDataLength;
    }


    /** Obtain a settings item.
     *
     *  @param  item    An item object to initialise
     *  @param  index   The settings index.
     *  @return         Logical true if the item exists, false if index is out of range.
     */
    bool ASSettings::getSettingsItemAtIndex(ASSettingsItem* item, unsigned index) const
    {
        const uint8_t* ptr = m_bufferStart;
        while (true)
        {
            unsigned totalBytes = loadItem(item, ptr);
            if (0 == totalBytes) return false;
            if (0 == index) break;
            index --;
            ptr += totalBytes;
        }
        return true;
    }


    /** Find a settings item with specified identifier.
     *
     *  @param  item    An item object to initialise.
     *  @param  ident   The item ident to look for.
     *  @param  type    The item type to look for.
     *  @return         Logical true if the item was found.
     */
    bool ASSettings::findSettingsItem(ASSettingsItem* item, unsigned type, unsigned ident) const
    {
        const uint8_t* ptr = m_bufferStart;
        while (true)
        {
            unsigned totalBytes = loadItem(item, ptr);
            if (0 == totalBytes) return false;
            if (item->type() == type && item->ident() == ident) return true;
            ptr += totalBytes;
        }
    }


    /** Clear all items in the buffer.
     */
    bool ASSettings::clearAllItems()
    {
        m_bufferDataLength = 0;
        return true;
    }


    /** Append a new item. The item will be created with no attached data.
     *
     *  @param  type    The type code.
     *  @param  ident   The id number.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::newItem(unsigned type, unsigned ident)
    {
        unsigned roundLength = m_bufferDataLength + (m_bufferDataLength & 1);
        if (m_bufferDataAllocation < m_bufferDataLength) return false;
        if (m_bufferDataAllocation - m_bufferDataLength < 6) return false;

        m_newItem = m_bufferStart + roundLength;

        as_EndianWriteU16(&m_newItem[0], type);
        as_EndianWriteU16(&m_newItem[2], ident);
        as_EndianWriteU16(&m_newItem[4], 0); // length

        m_bufferDataLength = 6 + roundLength;
        assert(m_bufferDataLength <= m_bufferDataAllocation);

        return true;
    }


    /** Append data to a new item.
     *
     *  @param  string  A C-string to append.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::appendItemData(const char* string)
    {
        return appendItemData(string, strlen(string) + 1);
    }


    /** Append data to a new item.
     *
     *  @param  value   The value to append.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::appendItemData(uint8_t value)
    {
        return appendItemData(&value, 1);
    }


    /** Append data to a new item.
     *
     *  @param  value   The value to append.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::appendItemData(uint16_t value)
    {
        uint16_t buf;
        as_EndianWriteU16(&buf, value);
        return appendItemData(&buf, 2);
    }


    /** Append data to a new item.
     *
     *  @param  value   The value to append.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::appendItemData(uint32_t value)
    {
        uint32_t buf;
        as_EndianWriteU32(&buf, value);
        return appendItemData(&buf, 4);
    }


    /** Append data to a new item.
     *
     *  @param  value   The value to append.
     *  @return         Logical true if the item could be added.
     */
    bool ASSettings::appendItemData(const void* data, unsigned length)
    {
        if (0 == m_newItem) return false;

        assert(m_bufferDataLength <= m_bufferDataAllocation);

        unsigned newItemSpace = m_bufferDataAllocation - m_bufferDataAllocation;
        if (newItemSpace < (length + (length & 1))) return false;

        unsigned newItemSize = as_EndianReadU16(&m_newItem[4]);
        assert(newItemSize + length < 65536);

        memcpy(m_bufferStart + m_bufferDataLength, data, length);

        as_EndianWriteU16(&m_newItem[4], newItemSize + length);
        m_bufferDataLength += length;
        return true;
    }


    /** Dump the settings to a file.
     *
     *  @param  fh      The output file handle.
     */
    void ASSettings::dump(FILE* fh) const
    {
        fprintf(fh, "Settings object %p:\n", this);

        unsigned index = 0;
        ASSettingsItem item;
        while (getSettingsItemAtIndex(&item, index++))
        {
            fprintf(fh, "  type %04x  ident %04x  length %04x  value: ", item.type(), item.ident(), item.length());
            switch (item.type())
            {
                case kASSettingsType_Label:         // c-strings
                case kASSettingsType_Password6:
                case kASSettingsType_Description:
                case kASSettingsType_FilePassword:
                    fprintf(fh, "%s", item.data());
                    break;

                case kASSettingsType_Option:        // array of uint16_t
                case kASSettingsType_AppletID:
                    fprintf(fh, "{");
                    for (unsigned i = 0; i < item.length() / 2; i++)
                    {
                        fprintf(fh, " %04x", item.dataU16(i));
                    }
                    fprintf(fh, " }");
                    break;

                case kASSettingsType_Range32:       // array of uint32_t
                    fprintf(fh, "{");
                    for (unsigned i = 0; i < item.length() / 4; i++)
                    {
                        fprintf(fh, " %04x", item.dataU32(i));
                    }
                    fprintf(fh, " }");
                    break;

                default:                            // anything else just show as raw bytes
                    fprintf(fh, "{");
                    for (unsigned i = 0; i < item.length(); i++)
                    {
                        fprintf(fh, " %02x", (unsigned) (item.dataU8(i)));
                    }
                    fprintf(fh, " }");
                    break;
            }
            fprintf(fh, "\n");
        }
    }


#pragma mark -
#pragma mark ASSettings : Private Methods

    /** Append data to a new item.
     *
     *  @param  value   The value to append.
     *  @return         The size (in bytes) of the item.
     */
    unsigned ASSettings::loadItem(ASSettingsItem* item, const uint8_t* ptr) const
    {
        assert(m_bufferDataLength <= m_bufferDataAllocation);
        assert(ptr >= m_bufferStart && ptr <= (m_bufferStart + m_bufferDataLength));

        if (m_bufferDataLength < 6) return 0;   // not even space for a single header

        unsigned type = as_EndianReadU16(&ptr[0]);
        unsigned ident = as_EndianReadU16(&ptr[2]);
        unsigned length = as_EndianReadU16(&ptr[4]);

        if (0 == ident && 0 == type && 0 == length) return 0;   // end of settings list

        const uint8_t* data = &ptr[6];
        unsigned totalLength = 6 + length + (length & 1);
        unsigned endOffset = unsigned(ptr - m_bufferStart) + totalLength;
        if (endOffset > m_bufferDataLength)
        {
            return 0;
        }
        else
        {
            item->m_type = type;
            item->m_ident = ident;
            item->m_length = length;
            item->m_data = data;
            return totalLength;
        }
    }

}   // namespace
