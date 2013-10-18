/** @file       ASGenericFile.h
 *  @brief      Generic file handling. This will allow the file to be converted to/from
 *              a generic XML format, allowing the file to be backed up and restored regardless
 *              of its binary content.
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
#ifndef COM_TSONIQ_ASGenericFile_h
#define COM_TSONIQ_ASGenericFile_h    (1)

#include <stdint.h>
#include <assert.h>
#include "ASFile.h"

namespace ts
{
    /** Generic file class.
     */
    class ASGenericFile : public ASFile
    {
    public:

        ASGenericFile();
        virtual ~ASGenericFile();

        virtual bool importText(const uint16_t* text, unsigned textSize);
        virtual bool exportText(const uint16_t** text, unsigned* textSize, bool bom=false) const;

    protected:

        virtual bool confirmLoad();

    private:

        mutable uint16_t* m_exportBuffer;       /**< Export result storage. */
        mutable unsigned m_exportSize;          /**< The number of UTF16 characters in the export buffer. */
        mutable unsigned m_exportAllocation;    /**< The maximum number of UTF16 characters that can be placed in the export buffer. */

        void appendToExport(const char* string) const;

        char* extractTag(const char* buffer, const char* key, bool stripSpace) const;
        bool readInt(int* value, const char* buffer, const char* key) const;
        bool readString(char* value, unsigned size, const char* buffer, const char* key, bool stripSpace=false) const;
        bool readData(uint8_t** value, unsigned* size, const char* buffer, const char* key) const;

        ASGenericFile(const ASGenericFile&);              /**< Prevent the use of the copy operator. */
        ASGenericFile& operator=(const ASGenericFile&);   /**< Prevent the use of the assignment operator. */
    };

}   // namespace


#endif  //COM_TSONIQ_ASGenericFile_h
