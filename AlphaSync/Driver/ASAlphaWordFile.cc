/** @file   ASAlphaWordFile.cc
 *  @brief  Implementation of the AlphaWord file object.
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
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "ASEndian.h"
#include "ASAlphaWordFile.h"


/* The format of an AlphaWord file is simply a byte array. Individual bytes are either direct character
 * codes or a control code of some kind. Character code data roughly conforms to CP1252 - see ASFile.cc
 * for translation tables and methods.
 *
 * Most bytes in an AlphaWord file translate directly to their Neo character code. The following is a list
 * of known exceptions to this that implement special functions in AlphaWord and which must be escaped:
 *
 *      Character   Function
 *           09     Tab (ASCII tab character code)
 *           0d     End of paragraph (ASCII carriage return)
 *           81     Line breaking space (ie a space at which the line can wrap on-screen)
 *           8d     Line breaking tab (ie a space at which the line can wrap on-screen)
 *           8f     Inserted after 16 non-breaking characters. This is a hint for line-breaking and is not displayed.
 *           a1     As 0x81 [only earlier AW versions?]
 *           a3     As 0x8d [only earlier AW versions?]
 *           a4     As 0x8f [only earlier AW versions?]
 *           a7     Fill byte (ie: effectively a non-functional pad byte inserted anywhere in the text).
 *           a8     Unknown (does not display on the Neo)
 *           a9     Unknown (appears to hide the entire line/paragraph of which it is a part)
 *           ad     Line breaking hyphen (equivalent to '-', or ASCII 0x2d).
 *           b0     Escape sequence character. Two escape characters surround a Neo character code.
 *
 *  An escape is { 0xb0, <code>, 0xb0 }. Experimentation suggests that escapes are used for the following characters:
 *
 *          09, 0a, 0d, 81, 8d, a1-bf
 *
 *  Carriage return (0x0d) signifies the end of a paragraph.
 *
 *  Codes 0x81, 0x8d, 0xa4 and 0xad appear to be used to add hints for the display of the text. The positioning of
 *  these seems to be dependent on the display font that is used on the Neo. These are easily stripped from
 *  the input when reading a file, but pose a problem when creating a new file (we do not know what font is
 *  in use, let alone details of the display size etc). The Neo seems to regenerate these as needed, but to
 *  be safe this code will create markers with a default spacing.
 *
 *  See the tables defined in ASFile.cc for mapping between Neo character data and unicode (without AlphaWord escapes).
 */


#define kASAlphaWordFileBreakHintSpacing    (8)             /**< The target interval for line-break hints. */

namespace ts
{

#pragma mark        --------  Public Methods  --------

    ASAlphaWordFile::ASAlphaWordFile()
        :
        ASFile(),
        m_minFileSize(512),
        m_maxFileSize(UINT_MAX),
        m_exportBuffer(0)
    {
        importText(0, 0);
    }


    ASAlphaWordFile::~ASAlphaWordFile()
    {
        if (0 != m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = 0;
    }


    void ASAlphaWordFile::setMinFileSize(unsigned size)
    {
        if (size < 256) size = 256;
        m_minFileSize = size;
        if (size > m_maxFileSize) m_maxFileSize = size;
    }


    void ASAlphaWordFile::setMaxFileSize(unsigned size)
    {
        if (size < 256) size = 256;
        m_maxFileSize = size;
        if (size < m_minFileSize) m_minFileSize = size;
    }


    bool ASAlphaWordFile::importText(const uint16_t* text, unsigned textSize)
    {
        if (textSize > 1024*1024*1024) return false;                        // Implausibly large text (may cause overflow later)

        unsigned inputCharacterCount = (textSize / 2);                      // Number of input characters
        unsigned maxNeoSize = m_minFileSize + (inputCharacterCount * 4);    // worst case estimated neo data size (everything escaped)
        uint8_t* neoText = (uint8_t*)malloc(maxNeoSize);
        if (!neoText) return false;
        memset(neoText, 0xa7, maxNeoSize);                                  // fill buffer with 'unused space' pad byte


        // Loop to translate characters.
        uint8_t* neo = neoText;
        uint8_t* endn = neo + maxNeoSize;
        const uint16_t* uni = text;
        const uint16_t* endu = uni + inputCharacterCount;
        unsigned softBreakCount = 0;
        unsigned hardBreakCount = 0;
        static const unsigned softBreakInterval = 40;
        static const unsigned hardBreakInterval = 24;
        uint8_t* lastBreakOpportunity = 0;
        while (uni != endu)
        {
            assert((endn - neo) >= 4);

            bool escape;
            int code;

            uint16_t unicode = *uni ++;
            if (0xfeff == unicode) continue;          // Ignore byte order marks
            int c = unicodeToNeo(unicode);

            // Miscellaneous re-mapping.
            if (c == 0x81) c = 0xac;        // Re-map the "not" alternate character (to not clash with line-break hint)

            // REVIEW: should we explicitly handle DOS CR+LF endings?
            if (c >= 0xa1 && c <= 0xbf)     { escape = true;    code = c; }
            else if (0x09 == c)             { escape = true;    code = c; }
            else if (0x0a == c)             { escape = true;    code = c; }
            else if (0x0d == c)             { escape = true;    code = c; }
            else if (kNeoCodeTab == c)      { escape = false;   code = 0x09; }
            else if (kNeoCodeNewline == c)  { escape = false;   code = 0x0d; }  // newline (mapped to return - end of paragraph)
            else if (kNeoCodeReturn == c)   { escape = false;   code = 0x0d; }
            else if (c >= 0 && c <= 0xff)   { escape = false;   code = c; }
            else                            { escape = false;   code = kNeoUntranslatableCharacter; }

            bool isBreak = (!escape) && (0x0d == code);
            bool isBreakable = (!escape) && (0x2d == code || 0x20 == code || 0x09 == code);

            hardBreakCount ++;
            softBreakCount ++;

            if (isBreak)
            {
                // The current character is an implicit break.
                lastBreakOpportunity = 0;
                softBreakCount = 0;
                hardBreakCount = 0;
            }
            else if (isBreakable)
            {
                lastBreakOpportunity = neo;
                hardBreakCount = 0;
            }
            else if (hardBreakCount >= hardBreakInterval)
            {
                *neo ++ = 0x8f;                 // insert a hard-break character
                hardBreakCount = 0;
                softBreakCount = 0;             // REVIEW: should we do this?
                lastBreakOpportunity = 0;       // REVIEW: should we do this?
            }

            if (escape) *neo ++ = 0xb0;
            *neo ++ = code;
            if (escape) *neo ++ = 0xb0;

            if (softBreakCount >= softBreakInterval && lastBreakOpportunity)
            {
                // Substitute breakable characters with their breaking equivalents
                if (0x2d == *lastBreakOpportunity) *lastBreakOpportunity = 0xad;
                else if (0x20 == *lastBreakOpportunity) *lastBreakOpportunity = 0x81;
                else if (0x09 == *lastBreakOpportunity) *lastBreakOpportunity = 0x8d;
                else
                {
                    fprintf(stderr, "%s: failed to encode break character\n", __FUNCTION__);
                    assert(0);  // mismatch between this code and the assignment of isBreakable
                }
                lastBreakOpportunity = 0;
                softBreakCount = 0;
                hardBreakCount = 0;
            }
        }

        unsigned totalSize = (unsigned)(neo - neoText);
        if (totalSize < m_minFileSize) totalSize = m_minFileSize;
        if (totalSize > m_maxFileSize)
        {
            fprintf(stderr, "ASAlphaWordFile: import translation exceeds max file size of %u bytes\n", m_maxFileSize);
            free(neoText);
            return false;
        }

        uint8_t* data = (uint8_t*)setFileSize(totalSize);
        if (!data)
        {
            free(neoText);
            return false;
        }

        memcpy(data, neoText, totalSize);       // Copy translated text to the object

        free(neoText);
        return true;
    }


    bool ASAlphaWordFile::exportText(const uint16_t** text, unsigned* textSize, bool bom) const
    {
        *text = 0;
        *textSize = 0;

        if (0 == fileSize()) return true;

        unsigned maxCharacterCount = fileSize() + 1;    // plus one allows for optional BOM
        if (0 != m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = (uint16_t*) malloc(maxCharacterCount * sizeof (uint16_t));
        if (0 == m_exportBuffer) return false;

        const uint8_t *ptr = byteData();
        const uint8_t *end = byteData() + fileSize();

        unsigned characterCount = 0;

        if (bom) m_exportBuffer[characterCount++] = 0xfeff;

        while (ptr != end)
        {
            int code = *ptr ++;
            if (0xa4 == code) continue;                 // unused code
            if (0xa7 == code) continue;                 // unused code
            if (0x09 == code) code = kNeoCodeTab;       // pass code through the character set translation
            if (0x0a == code) code = kNeoCodeNewline;   // pass code through the character set translation
            if (0x0d == code) code = kNeoCodeReturn;    // pass code through the character set translation
            if (0x81 == code) code = 0x20;              // line-breaking space
            if (0x8d == code) code = kNeoCodeTab;       // line-breaking tab
            if (0x8f == code) continue;                 // period break in a run of contiguous characters
            if (0xa1 == code) code = 0x20;              // line-breaking space (older software versions)
            if (0xa3 == code) code = kNeoCodeTab;       // line-breaking tab (older software versions)
            if (0xad == code) code = 0x2d;              // line-breaking hyphen
            if (0xb0 == code)                           // escape sequence
            {
                if ((end - ptr) < 2)
                {
                    fprintf(stderr, "ASAlphaWordText: %s: Unexpectedly truncated escape sequence\n", __FUNCTION__);
                }
                else
                {
                    code = *ptr ++;             // get the interpreted code directly
                    if (*ptr == 0xb0) ptr ++;   // skip over a following escape code (if present)
                }
            }
            else
            {
                if (0xa1 <= code && code <= 0xbf)
                {
                    fprintf(stderr, "ASAlphaWordText: %s: Possible untrapped escape: %02x\n", __FUNCTION__, code);
                    continue;
                }
            }

            m_exportBuffer[characterCount++] = neoToUnicode(code);
        }

        assert(characterCount <= maxCharacterCount);

        *text = m_exportBuffer;
        *textSize = characterCount * 2;
        return true;
    }


#if 0
    /** Method used to test the encoding of a file. This generates a known byte code that can then be viewed on the Neo.
     */
    bool ASAlphaWordFile::generateTestFile()
    {
        setFileSize(0);

        for (unsigned i = 0; i < 256; i++)
        {
            char buffer[128];
            sprintf(buffer, "Code %02x: =", i);
            unsigned len = strlen(buffer);
            buffer[len++] = i;
            buffer[len++] = '=';
            buffer[len++] = ' ';
            buffer[len++] = '=';
            buffer[len++] = i;
            buffer[len++] = i;
            buffer[len++] = '=';
            buffer[len++] = ' ';
            buffer[len++] = '=';
            buffer[len++] = i;
            buffer[len++] = i;
            buffer[len++] = i;
            buffer[len++] = i;
            buffer[len++] = '=';
            buffer[len++] = '\r';

            if (!appendData(buffer, len)) return false;
        }

        for (unsigned i = 0; i < 256; i++)
        {
            char buffer[128];
            sprintf(buffer, "Escape %02x: =", i);
            unsigned len = strlen(buffer);
            buffer[len++] = 0xb0;
            buffer[len++] = i;
            buffer[len++] = 0xb0;
            buffer[len++] = '=';
            buffer[len++] = '\r';

            if (!appendData(buffer, len)) return false;
        }

        return true;
    }
#endif


#pragma mark        --------  Protected Methods  --------


    /** In theory, we can check if the input file that we have been given is sensible. However,
     *  any sequence of bytes of any length (within reason) is likely usable as an AlphaWord file.
     *
     *  @return     Logical true if the data appears valid and has been accepted.
     */
    bool ASAlphaWordFile::confirmLoad()
    {
        return true;
    }



}   // namespace
