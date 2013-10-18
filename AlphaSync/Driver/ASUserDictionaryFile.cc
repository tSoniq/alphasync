/** @file   ASUserDictionaryFile.cc
 *  @brief  Implementation of the user dictionary file object.
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
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "ASEndian.h"
#include "ASUserDictionaryFile.h"


/* The format of a dictionary file is symbolically:
 *
 *      struct dictionary
 *      {
 *          uint16_t m_offsets[19];
 *          uint8_t m_data[65536 - sizeof m_offsets];
 *      };
 *
 *  The m_offsets array stores the byte offsets of words of a specific length in the file, with
 *  the smallest word being two letters long. The offsets are relative to the start of the file
 *  rather than to the data itself.
 *
 *  For example, m_offsets[0] contains the offset of the 2 letter words, m_offsets[1] contains
 *  the offset of the 3 letter words and m_offsets[18] contains the offset of the 20 letter words
 *  (the longest that can be stored).
 *
 *  The maximum file size is an arbitrary limit (the file could contain a very large number of
 *  20 letter length words if the file size is not constrained to a 16 bit value).
 */

#define kASUserDictionaryFileMinWordLength  (2)                 /**< Smallest word that can be stored. */
#define kASUserDictionaryFileMaxWordLength  (20)                /**< Longest word that can be stored. */
#define kASUserDictionaryFileMinFileSize    (19*2)              /**< Smallest possible dictionary file. */
#define kASUserDictionaryFileMaxFileSize    (64*1024)           /**< Maximum size of the dictionary file. */

namespace ts
{

#pragma mark        --------  Public Methods  --------

    ASUserDictionaryFile::ASUserDictionaryFile()
        :
        ASFile(),
        m_offsets(),
        m_exportBuffer(0)
    {
        clearDictionary();
    }


    ASUserDictionaryFile::~ASUserDictionaryFile()
    {
        if (0 != m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = 0;
        for (unsigned i = 0; i < sizeof m_offsets / sizeof m_offsets[0]; i++) m_offsets[i] = kASUserDictionaryFileMaxFileSize + 1;
    }


    unsigned ASUserDictionaryFile::wordCount() const
    {
        unsigned total = 0;

        if (0 != byteData())    // Handle any earlier memory allocation failure.
        {
            for (unsigned length = kASUserDictionaryFileMinWordLength; length <= kASUserDictionaryFileMaxWordLength; length++)
            {
                total += (m_offsets[length + 1] - m_offsets[length]) / length;
            }
        }

        return total;
    }


    bool ASUserDictionaryFile::containsWord(const char* word) const
    {
        unsigned length = strlen(word);
        return (0 != locateWord((uint8_t*)word, length));
    }


    bool ASUserDictionaryFile::getWordAtIndex(char word[kASUserDictionaryFileMaxWordLength+1], unsigned index) const
    {
        memset(word, 0, kASUserDictionaryFileMaxWordLength+1);  // default is to return an empty string

        if (0 == byteData()) return false;

        for (unsigned wordLength = kASUserDictionaryFileMinWordLength; wordLength <= kASUserDictionaryFileMaxWordLength; wordLength ++)
        {
            uint8_t* start;
            uint8_t* end;

            textRegionForLength(&start, &end, wordLength);
            assert(end >= start);
            unsigned wordsThisLength = unsigned(end - start) / wordLength;

            if (index < wordsThisLength)
            {
                memcpy(word, start + (index * wordLength), wordLength);
                return true;
            }

            index -= wordsThisLength;
        }

        return false;   // word not present.
    }


    bool ASUserDictionaryFile::addWord(const char* word)
    {
        // Handle any earlier memory allocation failure.
        if (0 == byteData()) clearDictionary();
        if (0 == byteData()) return false;

        unsigned wordLength = strlen(word);

        uint8_t* start;
        uint8_t* end;

        if (!textRegionForLength(&start, &end, wordLength)) return false;           // no place to put the word

        uint8_t* search = start;
        while (search < end)
        {
            int cmp = memcmp(search, word, wordLength);
            if (0 == cmp) return true;              // word already present
            if (cmp > 0) break;                     // insert at the current point

            search += wordLength;
        }

        if (wordLength + fileSize() > kASUserDictionaryFileMaxFileSize) return false;   // file full

        assert(search > byteData());
        unsigned offset = unsigned(search - byteData());            // byte offset at which to insert the new word
        unsigned bytesToMove = fileSize() - offset;                 // number of bytes of data that need to be moved to create space for the word

        if (!setFileSize(fileSize() + wordLength)) return false;    // out of memory

        uint8_t* position = byteData() + offset;
        if (bytesToMove > 0) memmove(position + wordLength, position, bytesToMove);
        memcpy(position, word, wordLength);

        for (unsigned i = wordLength+1; i < sizeof m_offsets / sizeof m_offsets[0]; i++)
        {
            m_offsets[i] += wordLength;
        }

        saveOffsets();

        return true;
    }


    void ASUserDictionaryFile::removeWord(const char* word)
    {
        // Handle any earlier memory allocation failure.
        if (0 == byteData()) return;

        size_t wordLength = strlen(word);
        uint8_t* location = locateWord((uint8_t*)word, wordLength);
        if (location)
        {
            assert(location > byteData());
            assert((location + wordLength) <= (byteData() + fileSize()));

            memmove(location, location + wordLength, fileSize() - wordLength - (size_t)(location - byteData()));

            for (unsigned index = wordLength+1; index < sizeof m_offsets / sizeof m_offsets[0]; index++)
            {
                m_offsets[index] -= wordLength;
            }

            saveOffsets();

            setFileSize(fileSize() - wordLength);
            assert(fileSize() == m_offsets[kASUserDictionaryFileMaxWordLength+1]);  // last offset should track the file size...
        }
    }


    bool ASUserDictionaryFile::importText(const uint16_t* text, unsigned textSize)
    {
        if (!clearDictionary()) return false;

        unsigned characterCount = textSize / 2;

        uint8_t* neoText = (uint8_t*)malloc(characterCount * sizeof (uint8_t) + 1);   // +1 to allow additional zero terminator at end
        if (!neoText) return false;
        unicodeToNeo(neoText, text, characterCount);

        uint8_t* ptr = neoText;
        uint8_t* end = ptr + characterCount;

        while (ptr < end)
        {
            while (ptr < end && (isspace(*ptr) || ispunct(*ptr))) ptr ++;
            if (ptr == end) break;

            uint8_t* start = ptr;
            while (ptr < end && !isspace(*ptr) && !ispunct(*ptr)) ptr++;
            *ptr = 0;
            addWord((const char*)start);         // ignore errors adding specific words
            if (ptr < end) ptr++;
        }

        free(neoText);

        return true;
    }


    bool ASUserDictionaryFile::exportText(const uint16_t** text, unsigned* textSize, bool bom) const
    {
        *text = 0;
        *textSize = 0;

        if (!fileData()) return false;
        if (0 == wordCount()) return true;

        // the total number of characters is the number of actual text characters plus wordCount() - 1 for the inter-word spaces that will be added.
        unsigned charCount = m_offsets[kASUserDictionaryFileMaxWordLength+1] - m_offsets[kASUserDictionaryFileMinWordLength] + wordCount() - 1;
        if (bom) charCount++;

        if (0 != m_exportBuffer) free(m_exportBuffer);
        m_exportBuffer = malloc(charCount * sizeof (uint16_t));
        if (0 == m_exportBuffer) return false;

        char word[kASUserDictionaryFileMaxWordLength+1];
        uint16_t* ptr = (uint16_t*)m_exportBuffer;

        if (bom) *ptr ++ = 0xfeff;              // prepend optional BOM

        unsigned index = 0;
        while (getWordAtIndex(word, index++))
        {
            if (index > 1) *ptr++ = 0x0020;     // prepend a separating space character
            char* cptr = word;
            while (0 != *cptr) *ptr++ = neoToUnicode(*cptr++);
            assert(ptr <= ((uint16_t*)m_exportBuffer) + charCount);
        }

        assert(ptr == ((uint16_t*)m_exportBuffer) + charCount);  // we should have got the size exactly correct...

        *text = (uint16_t*)m_exportBuffer;
        *textSize = charCount * sizeof (uint16_t);
        return true;
    }




#pragma mark        --------  Protected Methods  --------


    /** Check as much of the dictionary content as is feasible. We could do more here,
     *  such as verifying the textual content, but this is difficult without a precise
     *  definition of the file format...
     *
     *  @return     Logical true if the data appears valid and has been accepted.
     */
    bool ASUserDictionaryFile::confirmLoad()
    {
        return (loadOffsets());
    }


#pragma mark        --------  Private Methods  --------



    /** Clear the contents of the dictionary.
     */
    bool ASUserDictionaryFile::clearDictionary()
    {
        unsigned size = (1 + kASUserDictionaryFileMaxWordLength - kASUserDictionaryFileMinWordLength) * 2;
        uint8_t* data = (uint8_t*)setFileSize(size);
        assert(data);   // we really should never run out of memory...
        if (data)
        {
            for (unsigned i = 0; i < sizeof m_offsets / sizeof m_offsets[0]; i++)
            {
                if (i < kASUserDictionaryFileMinWordLength || i > kASUserDictionaryFileMaxWordLength+1) m_offsets[i] = 0;
                else m_offsets[i] = size;
            }
            saveOffsets();
        }

        return (0 != data);
    }


    /** Read the offsets from the input file, handling endian conversion and
     *  adding one additional 'offset' at the end for the end of all data.
     *
     *  @return     Logical true if all ok, or false if the file data appears to be invalid.
     */
    bool ASUserDictionaryFile::loadOffsets()
    {
        for (unsigned i = 0; i < sizeof m_offsets / sizeof m_offsets[0]; i++)  m_offsets[i] = 0;

        bool ok = true;

        if (fileSize() > kASUserDictionaryFileMaxFileSize)
        {
            assert(0);
            return false;
        }

        if (fileSize() < (kASUserDictionaryFileMaxWordLength - kASUserDictionaryFileMinWordLength + 1) * 2)
        {
            assert(0);
            return false;
        }

        m_offsets[kASUserDictionaryFileMaxWordLength + 1] = fileSize();

        for (unsigned i = kASUserDictionaryFileMinWordLength; i <= kASUserDictionaryFileMaxWordLength; i++)
        {
            m_offsets[i] = as_EndianReadU16(byteData() + ((i - kASUserDictionaryFileMinWordLength) * 2));
            assert(m_offsets[i] <= m_offsets[kASUserDictionaryFileMaxWordLength+1]);
            if (m_offsets[i] > fileSize())
            {
                m_offsets[i] = fileSize(); // catch corrupt input
                ok = false;
            }
        }

        return ok;
    }


    /** Write the offsets to raw data, handling endian conversion.
     */
    void ASUserDictionaryFile::saveOffsets()
    {
        for (unsigned i = kASUserDictionaryFileMinWordLength; i <= kASUserDictionaryFileMaxWordLength; i++)
        {
            assert(m_offsets[i] >= ((kASUserDictionaryFileMaxWordLength - kASUserDictionaryFileMinWordLength + 1) * 2));
            assert(m_offsets[i] <= kASUserDictionaryFileMaxFileSize);

            as_EndianWriteU16(byteData() + ((i - kASUserDictionaryFileMinWordLength) * 2), m_offsets[i]);
        }
    }


    /** Find the memory region containing text for a given word length.
     *
     *  @param  start   Returns the start address of the region (inclusive).
     *  @param  end     Returns the end address of the region (exclusive).
     *  @param  length  The word length, in characters.
     *  @return         Logical true if the region exists, false if no region exists for the word length.
     */
    bool ASUserDictionaryFile::textRegionForLength(uint8_t** start, uint8_t** end, unsigned length) const
    {
        if (0 == byteData() || length < kASUserDictionaryFileMinWordLength || length > kASUserDictionaryFileMaxWordLength)
        {
            *start = 0;
            *end = 0;
            return false;
        }
        else
        {
            assert(m_offsets[length+1] >= m_offsets[length]);
            assert(m_offsets[length+1] <= fileSize());
            assert(((m_offsets[length+1] - m_offsets[length]) % length) == 0);  // no fractional words

            *start = byteData() + m_offsets[length];
            *end = byteData() + m_offsets[length + 1];

            return true;
        }
    }



    /** Find the address of a word in the dictionary.
     *
     *  @param  word    The word to find (NeoASCII).
     *  @param  length  The length of the word.
     *  @return         A pointer to the start of the word, or zero if not present.
     */
    uint8_t* ASUserDictionaryFile::locateWord(const uint8_t* word, unsigned length) const
    {
        uint8_t* start;
        uint8_t* end;

        textRegionForLength(&start, &end, length);

        for (uint8_t* search = start; search < end; search += length)
        {
            if (0 == memcmp(search, word, length)) return search;
        }

        return 0;
    }

}   // namespace
