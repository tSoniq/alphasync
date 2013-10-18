/** @file       ASSettings.h
 *  @brief      Settings tuple management.
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
#ifndef COM_TSONIQ_ASSettings_h
#define COM_TSONIQ_ASSettings_h   (1)

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ASEndian.h"

namespace ts
{
    /* Settings type values.
     */
    #define kASSettingsType_None            (0x0000)    /**< No item is present (used to mark end of data). */
    #define kASSettingsType_Label           (0x0001)    /**< Item is a null terminated string (fixed label). */
    #define kASSettingsType_Range32         (0x0102)    /**< Item is an integer numeric range: {default, min, max}. */
    #define kASSettingsType_Option          (0x0103)    /**< Item is a list of item IDs: {default, a, b, c...}. */
    #define kASSettingsType_Password6       (0x0105)    /**< Item is a password (c-string). Used for AW "File Passwords" and system "Master Password". Max 6 characters. */
    #define kASSettingsType_Description     (0x0106)    /**< Item is a null terminated string constant for descriptive purposes only. */
    #define kASSettingsType_FilePassword    (0xc001)    /**< Item is a file password (c-string). File is identified by the ident field. */
    #define kASSettingsType_AppletID        (0x8002)    /**< Item is a U16 applet ID. */

    /* Well known settings ident values.
     * Bit 31 is set if the ident is local to an applet, or clear if it is global (system applet).
     * Bit 30 is set for file passwords (possible security flag?)
     */
    #define kASSettingsIdent_None                   (0x0000)    /**< No item is present (used to mark end of data). */
    #define kASSettingsIdent_System_On              (0x1001)    /**< Setting is 'on' */
    #define kASSettingsIdent_System_Off             (0x1002)    /**< Setting is 'off' */
    #define kASSettingsIdent_System_Yes             (0x100c)    /**< Setting is 'yes' */
    #define kASSettingsIdent_System_No              (0x100d)    /**< Setting is 'no' */
    #define kASSettingsIdent_System_Password        (0x400b)    /**< Master password, as type 0x0105. */
    #define kASSettingsIdent_AlphaWord_ClearFiles   (0x8003)    /**< Clear all files, as type 0x0103. Use a value of kASSettingsIdent_System_On to trigger. */
    #define kASSettingsIdent_AlphaWord_MaxFileSize  (0x1010)    /**< Get maximum file size information. Type is Range32. */
    #define kASSettingsIdent_AlphaWord_MinFileSize  (0x1011)    /**< Get minimum file size information. Type is Range32. */

    /** Structure used to describe a TLV.
     */
    class ASSettingsItem
    {
    public:

        ASSettingsItem() : m_ident(0), m_type(0), m_length(0), m_data(0) { }

        unsigned type() const { return m_type; }
        unsigned ident() const { return m_ident; }
        unsigned length() const { return m_length; }
        const uint8_t* data() const { return m_data; }

        unsigned dataU8(unsigned index=0) const { return (index >= m_length) ? 0 : m_data[index]; }
        unsigned dataU16(unsigned index=0) const { return ((index*2+2) > m_length) ? 0 : as_EndianReadU32(m_data+(index*2)); }
        unsigned dataU32(unsigned index=0) const { return ((index*4+4) > m_length) ? 0 : as_EndianReadU32(m_data+(index*4)); }

    private:
        unsigned m_ident;
        unsigned m_type;
        unsigned m_length;
        const uint8_t* m_data;

        friend class ASSettings;
    };


    /** Class used to manage lists of tuple values.
     */
    class ASSettings
    {
    public:

        ASSettings(void* data, unsigned size);
        ASSettings(void* data, unsigned dataSize, unsigned allocationSize);

        uint8_t* buffer();
        unsigned size();
        unsigned space();

        bool clearAllItems();

        bool getSettingsItemAtIndex(ASSettingsItem* item, unsigned index) const;
        bool findSettingsItem(ASSettingsItem* item, unsigned type, unsigned ident) const;

        bool newItem(unsigned type, unsigned ident);
        bool appendItemData(const char* string);
        bool appendItemData(uint8_t value);
        bool appendItemData(uint16_t value);
        bool appendItemData(uint32_t value);
        bool appendItemData(const void* data, unsigned length);

        void dump(FILE* fh) const;


    private:

        uint8_t* m_bufferStart;
        unsigned m_bufferDataLength;
        unsigned m_bufferDataAllocation;
        uint8_t* m_newItem;

        unsigned loadItem(ASSettingsItem* item, const uint8_t* ptr) const;

        ASSettings();   // Prevent the use of the default constructor.
    };

}   // namespace

#endif  // COM_TSONIQ_ASSettings_h
