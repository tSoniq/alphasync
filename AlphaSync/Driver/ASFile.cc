/** @file   ASFile.cc
 *  @brief  Generic file handling.
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
#include "ASFile.h"


namespace ts
{
    /** Table mapping eight bit Neo character codes to their corresponding 16 bit unicode
     *  counter parts. This is an extended version of CP1252.
     */
    static const uint16_t neoToUnicodeTable[256] =
    {
#if 1   // translations of the Neo font characters 0-31
        0x25a0, 0x03b4, 0x0394, 0x222b, 0x0143, 0x0133, 0x274f, 0x2154, 0x02d9, 0x21e5, 0x2193, 0x2191, 0x2913, 0x21b5, 0x2908, 0x2909,
        0x2192, 0x2153, 0x039e, 0x03b1, 0x03c1, 0x2195, 0x21a9, 0x25a1, 0x221a, 0x2264, 0x2265, 0x03b8, 0x221e, 0x03a9, 0x03b2, 0x03a3,
#else   // standard control codes, with exception of 0x0000 (solid block)
        0x25a0, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
        0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
#endif
        0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
        0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
        0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
        0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
        0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
        0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2190,
        0x20ac, 0x00ac, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x03a6, 0x017d, 0x03a0,
        0x2035, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x03c0, 0x017e, 0x0178,
        0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
        0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
        0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
        0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
        0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
        0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
    };



#pragma mark    --------  Public Methods  --------


    ASFile::ASFile()
        :
        m_appletID(kASAppletID_Invalid),
        m_appletVersionMajor(0),
        m_appletVersionMinor(0),
        m_haveAppletInfo(false),
        m_fileData(0),
        m_fileSize(0)
    {
        // Nothing.
    }


    ASFile::~ASFile()
    {
        setFileSize(0);
    }


    bool ASFile::load(const void* data, unsigned size)
    {
        void* ptr = setFileSize(size);
        if (ptr) memcpy(ptr, data, size);
        return confirmLoad();
    }


    bool ASFile::load(ASDevice* device, const ASApplet* applet, int fileIndex, unsigned maxBytes)
    {
        setFileSize(0);                     // release any existing memory
        assert(0 == m_fileData);

        if (maxBytes != 0)
        {
            if (minimumLoadSize() > maxBytes) return false;   // requested load size is not permitted
        }

        ASFileAttributes attr;
        if (!device->getFileAttributes(&attr, applet, fileIndex)) return false;     // No such file

        unsigned rawSize = attr.allocSize();
        if (maxBytes != 0 && rawSize > maxBytes) rawSize = maxBytes;
        uint8_t* rawData = (uint8_t*)malloc(rawSize);
        if (0 == rawData) return false;     // out of memory

        unsigned actualRawSize = 0;
        if (!device->readFile(rawData, rawSize, &actualRawSize, applet, fileIndex, true))
        {
            free(rawData);
            return false;                   // error reading file from device
        }

        m_fileData = rawData;
        m_fileSize = actualRawSize;

        return confirmLoad();               // allow specialisation to determine if data was valid
    }


    bool ASFile::load(ASDevice* device, const ASApplet* applet, const char* filename)
    {
        int fileIndex = device->indexForFileWithName(applet, filename);
        if (fileIndex < 0) return -1;       // no such file

        return load(device, applet, fileIndex);
    }


    bool ASFile::save(ASDevice* device, const ASApplet* applet, int fileIndex)
    {
        if (!confirmSave(device, applet, fileIndex)) return false;     // specialisation can choose to prevent save from occuring

        return device->writeFile(m_fileData, m_fileSize, applet, fileIndex, true);
    }


    bool ASFile::save(ASDevice* device, const ASApplet* applet, const char* filename)
    {
        int fileIndex = device->indexForFileWithName(applet, filename);

        if (!confirmSave(device, applet, fileIndex)) return false;     // specialisation can choose to prevent save from occuring

        // REVIEW: should we allow the password to be specified?
        bool result;
        if (fileIndex < 0) result = device->createFile(filename, "write", m_fileData, m_fileSize, applet, &fileIndex, true);
        else               result = device->writeFile(m_fileData, m_fileSize, applet, fileIndex, true);

        return result;
    }


    bool ASFile::importText(const uint16_t* text, unsigned textSize)
    {
        (void)text;
        (void)textSize;
        return false;   // derived classed should override this method
    }


    bool ASFile::exportText(const uint16_t** text, unsigned* textSize, bool bom) const
    {
        (void)text;
        (void)textSize;
        (void)bom;
        return false;   // derived classed should override this method
    }






#pragma mark    --------  Protected Methods  --------


    int ASFile::unicodeToNeo(uint16_t uni) const
    {
        if (0x0009 == uni) return kNeoCodeTab;
        else if (0x000a == uni) return kNeoCodeNewline;
        else if (0x000d == uni) return kNeoCodeReturn;
        else
        {
            for (unsigned i = 0; i < 256; i++)
            {
                if (neoToUnicodeTable[i] == uni) return (uint8_t) i;
            }
            return kNeoCodeUnknown;     // no match found
        }
    }


    uint16_t ASFile::neoToUnicode(int neo) const
    {
        if (neo >= 0 && neo <= 255) return neoToUnicodeTable[neo];
        else if (neo == kNeoCodeTab) return 9;
        else if (neo == kNeoCodeNewline) return 10;
        else if (neo == kNeoCodeReturn) return 13;
        else return 63;      // '?'
    }


    unsigned ASFile::unicodeToNeo(uint8_t* neo, const uint16_t* uni, unsigned count, bool escape) const
    {
        if (0 == count) return 0;

        bool nativeEndian;
        if (0xfeff == *uni)
        {
            nativeEndian = true;    // BOM indicating native byte order
            uni ++;
            count --;
        }
        else if (0xfffe == *uni)
        {
            nativeEndian = true;    // BOM indicating reversed byte order
            uni ++;
            count --;
        }
        else
        {
            nativeEndian = true;    // assume native endian format
        }


        uint8_t* end = neo + count;
        if (nativeEndian)
        {
            while (neo != end)
            {
                int code = unicodeToNeo(*uni ++);
                if (code < 0 || code > 255)
                {
                    if (escape)     // If escaping characters, pass through tab, newline and return
                    {
                        if (code == kNeoCodeTab) code = 9;
                        else if (code == kNeoCodeNewline) code = 10;
                        else if (code == kNeoCodeReturn) code = 11;
                        else code = kNeoUntranslatableCharacter;
                    }
                    else
                    {
                        code = kNeoUntranslatableCharacter;
                    }
                }
                *neo ++ = code & 0xff;
            }
        }
        else
        {
            while (neo != end)
            {
                unsigned code = *uni++;
                *neo ++ = unicodeToNeo(((code >> 8) & 0xff) | ((code << 8) & 0xff00));
            }
        }

        return count;
    }


    unsigned ASFile::neoToUnicode(uint16_t* uni, const uint8_t* neo, unsigned count, bool bom) const
    {
        uint16_t* end = uni + count;

        if (bom)
        {
            *uni ++ = 0xfeff;   // prepend optional BOM
            end ++;
            count ++;
        }

        while (uni != end)
        {
            *uni ++ = neoToUnicodeTable[*neo ++];
        }

        return count;
    }


    void* ASFile::setFileSize(unsigned size)
    {
        void* newRawData = (size > 0) ? realloc(m_fileData, size) : 0;
        if (0 == newRawData)
        {
            // Either size was zero or allocation failed - clear existing buffers
            if (0 != m_fileData) free(m_fileData);
            m_fileData = 0;
            m_fileSize = 0;
        }
        else
        {
            // Have successfully resized to a non-zero capacity
            m_fileData = newRawData;
            if (size > m_fileSize) memset(((uint8_t*)m_fileData) + m_fileSize, 0, size - m_fileSize);
            m_fileSize = size;
        }
        return m_fileData;
    }


    bool ASFile::appendData(void* data, unsigned size)
    {
        unsigned previousSize = fileSize();
        uint8_t* newData = (uint8_t*)setFileSize(previousSize + size);
        if (!newData)
        {
            return false;
        }
        else
        {
            memcpy(newData + previousSize, data, size);
            return true;
        }
    }


    bool ASFile::confirmLoad()
    {
        return true;        // derived classed should override this method
    }


    bool ASFile::confirmSave(ASDevice* device, const ASApplet* applet, int fileIndex)
    {
        (void)device;
        (void)applet;
        (void)fileIndex;
        return true;        // derived class should override this method
    }



#pragma mark    --------  Private Methods  --------


}   // namespace
