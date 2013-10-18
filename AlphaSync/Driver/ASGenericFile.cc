/** @file       ASGenericFile.cc
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
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "ASGenericFile.h"

namespace ts
{
    ASGenericFile::ASGenericFile()
        :
        ASFile(),
        m_exportBuffer(0),
        m_exportSize(0),
        m_exportAllocation(0)
    {
        // Nothing.
    }


    ASGenericFile::~ASGenericFile()
    {
        if (m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = 0;
        m_exportSize = 0;
        m_exportAllocation = 0;
    }


    bool ASGenericFile::importText(const uint16_t* text, unsigned textSize)
    {
        bool result = false;

        // (crudely) convert the input to plain ASCII
        char* const input = (char*)malloc(textSize + 1);
        if (!input) return false;

        const uint16_t* src = text;
        const uint16_t* end = text + (textSize / 2);
        char* dst = input;
        while (src < end)
        {
            char c = (*src > 0 && *src < 256) ? (*src) : '?';
            *dst = c;

            src ++;
            dst ++;
        }
        *dst ++ = 0;

        // Look for the applet info and check that this matches the target applet.
        // Currently this requires an exact match on both applet ID and major version information.
        // REVIEW: should this also check the minor version?
        ASAppletID thisAppletID;
        int thisVersionMajor;
        int thisVersionMinor;
        if (getAppletInfo(&thisAppletID, &thisVersionMajor, &thisVersionMinor))
        {
            int sourceAppletID;
            int sourceVersionMajor;
            int sourceVersionMinor;
            if (!readInt(&sourceAppletID, input, "appletID") ||
                !readInt(&sourceVersionMajor, input, "appletVersionMajor") ||
                !readInt(&sourceVersionMinor, input, "appletVersionMinor"))
            {
                fprintf(stderr, "AlphaSync::ASGenericFile::importText: input file is missing identifier tags\n");
                free(input);
                return false;
            }
            else if ((ASAppletID)sourceAppletID != thisAppletID || sourceVersionMajor != thisVersionMajor)
            {
                fprintf(stderr, "AlphaSync::ASGenericFile::importText: want ID %04x version %d.%d, got ID %04x version %d.%d\n",
                    (int)thisAppletID,
                    thisVersionMajor,
                    thisVersionMinor,
                    sourceAppletID,
                    sourceVersionMajor,
                    sourceVersionMinor);
                free(input);
                return false;
            }
        }

        // Load the data.
        uint8_t* data = 0;
        unsigned size = 0;
        if (readData(&data, &size, input, "data") && data)
        {
            result = load(data, size);
        }
        if (data) free(data);
        free(input);
        return result;
    }


    bool ASGenericFile::exportText(const uint16_t** text, unsigned* textSize, bool bom) const
    {
        *text = 0;
        *textSize = 0;

        if (!fileData()) return false;

        // Estimate the total size of the character data. Total headers and tags allows 1k. The data size
        // is calculated based on the 2 charters per byte plus indent and newline characters.
        static const unsigned dataBytesEncodedPerLine = 32;
        static const unsigned dataCharactersPerLine = (2 + 1 + (dataBytesEncodedPerLine * 2));
        unsigned maxCharacterCount = 1024 + (fileSize()*2) + ((fileSize()/dataBytesEncodedPerLine + 1) * dataCharactersPerLine);

        if (0 != m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = (uint16_t*)malloc(maxCharacterCount * sizeof (uint16_t));
        if (0 == m_exportBuffer) return false;
        m_exportSize = 0;
        m_exportAllocation = maxCharacterCount;

        char buffer[256];

        if (bom)
        {
            m_exportBuffer[0] = 0xfeff;              // prepend optional BOM
            m_exportSize ++;
        }


        appendToExport("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

        ASAppletID appletID;
        int major;
        int minor;
        if (getAppletInfo(&appletID, &major, &minor))
        {
            snprintf(buffer, sizeof buffer, "<appletID>0x%04x</appletID>\n", appletID);
            appendToExport(buffer);
            snprintf(buffer, sizeof buffer, "<appletVersionMajor>%d</appletVersionMajor>\n", major);
            appendToExport(buffer);
            snprintf(buffer, sizeof buffer, "<appletVersionMinor>%d</appletVersionMinor>\n", minor);
            appendToExport(buffer);
        }

        appendToExport("<data>");
        const uint8_t* bytes = byteData();
        for (unsigned index = 0; index < fileSize(); index++)
        {
            if ((index % dataBytesEncodedPerLine) == 0)
            {
                appendToExport("\n  ");       // 3 characters overhead per line (two spaces for indent plus new line
            }

            snprintf(buffer, sizeof buffer, "%02x", (unsigned)*bytes);
            appendToExport(buffer);
            bytes ++;
        }
        appendToExport("\n</data>");

        *text = (uint16_t*)m_exportBuffer;
        *textSize = m_exportSize * sizeof (uint16_t);
        return true;
    }


    bool ASGenericFile::confirmLoad()
    {
        // We do not care about the content or size of the data for the XML representation, so any loaded data is valid.
        return true;
    }


    /** Append a C string to the export buffer, converting to UTF16.
     *
     *  @param  string      The string to append.
     */
    void ASGenericFile::appendToExport(const char* string) const
    {
        unsigned len = strlen(string);
        assert(len + m_exportSize <= m_exportAllocation);
        if (len + m_exportSize <= m_exportAllocation)
        {
            const char* src = string;
            uint16_t* dst = m_exportBuffer + m_exportSize;
            while (*src)
            {
                *dst ++ = (*src++) & 0x00ff;
            }
            m_exportSize += len;
        }
    }

    /** Locate a tag in a text string (crude XML parsing).
     *
     *  @param  buffer      The buffer to search.
     *  @param  key         The XML key.
     *  @param  stripSpace  Logical true to strip whitespace (including newlines) from the input data.
     *  @return             Returns a pointer to the start of the extracted data, or zero if not found.
     *                      If non-zero, the caller should release the returned data by calling free().
     */
    char* ASGenericFile::extractTag(const char* buffer, const char* key, bool stripSpace) const
    {
        char startTag[256];
        char endTag[256];
        snprintf(startTag, sizeof startTag, "<%s>", key);
        snprintf(endTag, sizeof endTag, "</%s>", key);

        const char* start = strstr(buffer, startTag);
        if (!start) return 0;
        start += strlen(startTag);
        while (isspace(*start)) start ++;                       // always remove white space immediately following the start tag
        const char* end = strstr(start, endTag);
        if (!end) return 0;
        while (end != start && isspace(*(end - 1))) end --;     // always remove white space immediate preceding the end tag

        unsigned len = unsigned(end - start);
        char* result = (char*)malloc(len + 1);
        if (result)
        {
            if (!stripSpace)
            {
                memcpy(result, start, len);
                result[len] = 0;
            }
            else
            {
                const char* src = start;
                char* dst = result;
                while (src != end)
                {
                    if (!isspace(*src)) *dst ++ = *src;
                    src ++;
                }
                *dst = 0;
            }
        }
        return result;
    }


    /** Read a numeric (int) tag.
     *
     *  @param  value       Returns the value, or zero if not parsed correctly.
     *  @param  buffer      The start of the string data.
     *  @return             Logical true if the value was parsed correctly.
     */
    bool ASGenericFile::readInt(int* value, const char* buffer, const char* key) const
    {
        *value = 0;
        bool result = false;
        char* data = extractTag(buffer, key, false);
        if (data)
        {
            result = (1 == sscanf(data, "%i", value));
            free(data);
        }
        return result;
    }


    /** Read a string (char) tag.
     *
     *  @param  value       Returns the value, or a zero length string if not parsed correctly.
     *  @param  size        The size of the return string buffer.
     *  @param  buffer      The start of the string data.
     *  @return             Logical true if the value was parsed correctly.
     */
    bool ASGenericFile::readString(char* value, unsigned size, const char* buffer, const char* key, bool stripSpace) const
    {
        assert(size > 0);
        value[0] = 0;
        bool result = false;
        char* data = extractTag(buffer, key, stripSpace);
        if (data)
        {
            strncpy(value, data, size);
            value[size-1] = 0;
            free(data);
            result = true;
        }
        return result;
    }


    /** Read a binary data tag.
     *
     *  @param  value       Returns the data. Caller should free this when done.
     *  @param  size        Returns the data size.
     *  @param  buffer      The start of the string data.
     *  @param  key         The key to load.
     *  @return             Logical true if the value was parsed correctly.
     */
    bool ASGenericFile::readData(uint8_t** value, unsigned* size, const char* buffer, const char* key) const
    {
        bool result = false;

        *value = 0;
        *size = 0;

        char startTag[256];
        char endTag[256];
        snprintf(startTag, sizeof startTag, "<%s>", key);
        snprintf(endTag, sizeof endTag, "</%s>", key);

        const char* start = strstr(buffer, startTag) + strlen(startTag);
        if (!start) return false;
        const char* end = strstr(start, endTag);
        if (!end) return false;

        assert(end >= start);
        unsigned len = unsigned(end - start);
        if (len) *value = (uint8_t*)malloc((len + 1) / 2);
        if (*value)
        {
            result = true;
            const char*  src = start;
            uint8_t* dst = *value;
            uint8_t assembly = 0;
            bool byteReady = true;

            while (src != end)
            {
                char ch = tolower(*src);
                if (ch >= 'a' && ch <= 'f')
                {
                    assembly = (assembly << 4);
                    assembly |= (ch - 'a' + 10);
                    byteReady = !byteReady;
                    if (byteReady) *dst++ = assembly;
                }
                else if (ch >= '0' && ch <= '9')
                {
                    assembly = (assembly << 4);
                    assembly |= (ch - '0');
                    byteReady = !byteReady;
                    if (byteReady) *dst++ = assembly;
                }
                else if (!isspace(ch))
                {
                    // unrecognised character
                    free(*value);
                    *value = 0;
                    result = false;
                    break;
                }
                src ++;
            }

            if (result) *size = unsigned(dst - *value);
        }

        return result;
    }

}   // namespace
