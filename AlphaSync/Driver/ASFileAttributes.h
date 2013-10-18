/** @file       ASFileAttributes.h
 *  @brief      File information.
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
#ifndef COM_TSONIQ_ASFileAttributes_H
#define COM_TSONIQ_ASFileAttributes_H   (1)

#include <stdint.h>
#include "ASEndian.h"

namespace ts
{
    #define kASFileAttributesSize               (40)    /**< The number of bytes in the file attributes object. */

    #define kASFileAttributesFileNameMaxSize    (15)    /**< The filename may contain only this many characters (excluding terminating null). */
    #define kASFileAttributesPasswordMaxSize    (7)     /**< The password may contain only this many characters (excluding terminating null). */

    /* Values for the flags word.
     * If you create a new AlphaWord file with a set of flags of zero, they will end up as 0x04 or 0x06
     * once the file has been opened on the Neo.
     */
    #define kASFileAttributesFlagsUnknown0      (0x01)  /**< Unknown flag (always clear). */
    #define kASFileAttributesFlagsCurrent       (0x02)  /**< Set if the file is the currently active file for the applet. */
    #define kASFileAttributesFlagsUnknown1      (0x04)  /**< Unknown flag (always set for ASWordFiles, clear for others?). */



    /** FileInfo object.
     */
    class ASFileAttributes
    {
    public:

        ASFileAttributes();
        ASFileAttributes(const uint8_t buffer[kASFileAttributesSize]);
        ASFileAttributes(const ASFileAttributes& other);

        void copyFrom(const ASFileAttributes* other);
        void copyFrom(const uint8_t buffer[kASFileAttributesSize]);

        const uint8_t* rawData() const { return m_raw; }

        void clear();

        const char* fileName() const;
        const char* password() const;
        bool setFileName(const char* name);
        bool setPassword(const char* pass);

        unsigned minSize() const            { return as_EndianReadU32(&m_raw[0x18]); }
        unsigned allocSize() const          { return as_EndianReadU32(&m_raw[0x1c]); }
        unsigned flags() const              { return as_EndianReadU32(&m_raw[0x20]); }
        unsigned unknown1() const           { return as_EndianReadU8(&m_raw[0x24]); }
        unsigned fileSpace() const;
        unsigned unknown2() const           { return as_EndianReadU16(&m_raw[0x26]); }

        void setMinSize(unsigned size)      { as_EndianWriteU32(&m_raw[0x18], size); }
        void setAllocSize(unsigned size)    { as_EndianWriteU32(&m_raw[0x1c], size); }
        void setFlags(unsigned value)       { as_EndianWriteU32(&m_raw[0x20], value); }
        void setFileSpace(unsigned space);
        void setUnknown1(unsigned value)    { as_EndianWriteU16(&m_raw[0x26], value); }

        void dump(FILE* fh) const;

    private:

        /** The raw file attribute data.
         *
         *  bytes   00-0f   Zero terminated file name string (this appears to be reported wrongly in some cases - bugs in some Neo firmware?)
         *  bytes   10-17   Zero termined file password string (max six characters?)
         *  bytes   18-1b   Minimum file allocation size
         *  bytes   1c-1f   Actual file allocation size
         *  bytes   20-23   Flags (only the lowest 3 bits are used?)
         *  byte    24-25   file space code
         *  bytes   26-27   unknown1        -   appears to be ignored on write and quasi-random on read
         */
        uint8_t m_raw[kASFileAttributesSize];

        friend class ASDevice;
    };


};  // namespace


#endif  // COM_TSONIQ_ASFileAttributes_H

