/** @file   ASUserDictionaryFile.h
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

#ifndef COM_TSONIQ_ASUserDictionaryFile_H
#define COM_TSONIQ_ASUserDictionaryFile_H   (1)

#include <stdint.h>
#include "ASFile.h"

namespace ts
{
    #define kASUserDictionaryFileMinWordLength  (2)         /**< Smallest word that can be stored. */
    #define kASUserDictionaryFileMaxWordLength  (20)        /**< Longest word that can be stored. */
    #define kASUserDictionaryFileMinFileSize    (19*2)      /**< Smallest possible dictionary file. */
    #define kASUserDictionaryFileMaxFileSize    (64*1024)   /**< Maximum size of the dictionary file. */

    /** The dictionary class.
     */
    class ASUserDictionaryFile : public ASFile
    {
    public:

        ASUserDictionaryFile();
        virtual ~ASUserDictionaryFile();


        /** Return the number of words in the dictionary.
         */
        unsigned wordCount() const;


        /** Test if the specified word exists in the dictionary.
         *
         *  @param  word    The word to add as a C-String (NeoASCII).
         *  @return         Logical true if the word exists in the dictionary.
         */
        bool containsWord(const char* word) const;


        /** Read a word from the dictionary.
         *
         *  @param  word    Buffer used to return the word as a zero-terminated C-String (NeoASCII).
         *  @param  index   The word number (zero to @e wordCount() - 1).
         *  @return         Logical true if the word existed.
         */
        bool getWordAtIndex(char word[kASUserDictionaryFileMaxWordLength+1], unsigned index) const;


        /** Add a word to the dictionary.
         *
         *  @param  word    The word to add as a C-String (NeoASCII).
         *  @return         Logical true if the word was added.
         */
        bool addWord(const char* word);


        /** Remove a word from the dictionary.
         *
         *  @param  word    Buffer used to return the word as a zero-terminated C-String (MacASCII).
         */
        void removeWord(const char* word);


        /** Clear the contents of the dictionary.
         */
        void removeAllWords() { clearDictionary(); }


        virtual bool importText(const uint16_t* text, unsigned textSize);
        virtual bool exportText(const uint16_t** text, unsigned* textSize, bool bom=false) const;

    protected:

        virtual bool confirmLoad();

    private:

        unsigned m_offsets[kASUserDictionaryFileMaxWordLength + 2];
        mutable void* m_exportBuffer;

        bool clearDictionary();
        bool loadOffsets();
        void saveOffsets();
        bool textRegionForLength(uint8_t** start, uint8_t** end, unsigned length) const;
        uint8_t* locateWord(const uint8_t* word, unsigned length) const;

        ASUserDictionaryFile(const ASUserDictionaryFile&);              /**< Prevent the use of the copy operator. */
        ASUserDictionaryFile& operator=(const ASUserDictionaryFile&);   /**< Prevent the use of the assignment operator. */
    };

}   // namespace

#endif      // COM_TSONIQ_ASUserDictionaryFile_H
