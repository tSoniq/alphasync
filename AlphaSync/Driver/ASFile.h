/** @file   ASFile.h
 *  @brief  Class providing common file handling for applet data files.
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

#ifndef COM_TSONIQ_ASFile_H
#define COM_TSONIQ_ASFile_H   (1)

#include <stdint.h>
#include "ASDevice.h"

namespace ts
{
    /** The generic file converter.
     */
    class ASFile
    {
    public:

        /** Constructor. Creates a valid but empty file object.
         */
        ASFile();


        /** Destructor. Releases all resources allocated and invalidates
         *  any memory pointer that may previously have been returned.
         */
        virtual ~ASFile();


        /** Set the applet ID and version information for the file, representing the
         *  target applet and version information for the file data. This can be used
         *  to check that the version of a file being loaded or imported is compatible
         *  with the client software.
         *
         *  @param  appletID        The applet ID.
         *  @param  versionMajor    The applet major version number.
         *  @param  versionMinor    The applet minor version number.
         */
        void setAppletInfo(ASAppletID appletID, int versionMajor, int versionMinor)
        {
            m_appletID = appletID;
            m_appletVersionMajor = versionMajor;
            m_appletVersionMinor = versionMinor;
            m_haveAppletInfo = true;
        }


        /** Return the applet info.
         *
         *  @param  appletID        Returns the applet ID.
         *  @param  versionMajor    Returns the major version number.
         *  @param  versionMinor    Returns the minor version number.
         *  @return                 Logical true if the applet info is present, or false if no applet info is present.
         */
        bool getAppletInfo(ASAppletID* appletID, int* versionMajor, int* versionMinor) const
        {
            if (m_haveAppletInfo)
            {
                *appletID = m_appletID;
                *versionMajor = m_appletVersionMajor;
                *versionMinor = m_appletVersionMinor;
                return true;
            }
            else
            {
                *appletID = kASAppletID_Invalid;
                *versionMajor = 0;
                *versionMinor = 0;
                return false;
            }
        }


        /** Load the raw file data (for example, after uploading the file
         *  from the Neo). A copy will be made of the supplied data.
         *
         *  @param  data        Pointer to the data.
         *  @param  size        The number of bytes of data present.
         *  @return             Logical true if the data was valid.
         */
        bool load(const void* data, unsigned size);


        /** Load the file directly from the device.
         *
         *  @param  device      The device handle.
         *  @param  applet      The applet.
         *  @param  fileIndex   The file index.
         *  @param  maxBytes    The maximum number of bytes to read, or zero for unlimited. An
         *                      error should be returned if limited the read to this size is
         *                      not possible.
         *  @return             Logical true if the file was loaded.
         */
        bool load(ASDevice* device, const ASApplet* applet, int fileIndex, unsigned maxBytes=0);


        /** Load the file directly from the device. Note that if two files exist
         *  with identical names, the one with the lowest index number will be
         *  loaded.
         *
         *  @param  device      The device handle.
         *  @param  applet      The applet.
         *  @param  filename    The filename (case insensitive).
         *  @return             Logical true if the file was loaded.
         */
        bool load(ASDevice* device, const ASApplet* applet, const char* filename);


        /** Save the file directly to the device. The file must already exist.
         *
         *  @param  device      The device handle.
         *  @param  applet      The applet.
         *  @param  fileIndex   the file index.
         *  @return             Logical true if the file was saved.
         */
        bool save(ASDevice* device, const ASApplet* applet, int fileIndex);


        /** Save the file directly to the device. The named file will be created
         *  if it does not already exist.
         *
         *  @param  device      The device handle.
         *  @param  applet      The applet.
         *  @param  filename    The filename. Case is preserved, but matching is case insensitive.
         *  @return             Logical true if the file was saved.
         */
        bool save(ASDevice* device, const ASApplet* applet, const char* filename);


        /** Return the number of bytes of data in the file. This remains
         *  valid until the object destructor or any non-const member
         *  method is invoked.
         *
         *  @return             The number bytes of raw file data. Use @e fileData() to access the data itself.
         */
        unsigned fileSize() const { return m_fileSize; }


        /** Return a pointer to the file's binary data. This remains
         *  valid until the object destructor or any non-const member
         *  method is invoked.
         *
         *  @return             A pointer to the raw file data. Use @e fileSize() to determine the data size.
         *                      The returned pointer may be zero if @e fileSize() is zero.
         */
        const void* fileData() const { return m_fileData; }


        /** Import the data from plain (unicode) text. By default, this will return failure.
         *  A derived class should override this with appropriate logic.
         *
         *  @param  text        Returns the address of the translated text. The text buffer will remain valid until
         *                      the destruction of the object or the next translation request. The output text is in
         *                      UTF16 form with native endian coding.
         *  @param  textSize    The size of the translated text, in bytes (the number of characters is half this value).
         *  @return             Logical true if the request could be completed.
         */
        virtual bool importText(const uint16_t* text, unsigned textSize);


        /** Export the data as plain (unicode) text. By default, this will return failure.
         *  A derived class should override this with appropriate logic.
         *
         *  @param  text        Returns the address of the translated text. The text buffer will remain valid until
         *                      the destruction of the object or the next translation request. The output text is in
         *                      UTF16 form with native endian coding.
         *  @param  textSize    The size of the translated text, in bytes (the number of characters is half this value).
         *  @param  bom         Logical true to prepend a BOM to the output. Default false.
         *  @return             Logical true if the request could be completed.
         */
        virtual bool exportText(const uint16_t** text, unsigned* textSize, bool bom=false) const;


        /** Return the minum number of bytes that are required to meaningfully interpret the file data, or
         *  zero if a partial load is not possible. This is used to implement 'preview' functionality,
         *  where only a small subset of the full file is read (because reading from the physical device is slow...).
         *
         *  @return             Zero if no partial load is possible, or non-zero for the minumum number of bytes
         *                      that must be read for the file data to be parsed.
         */
        virtual unsigned minimumLoadSize() const
        {
            return 0;           // subclasses should override this if a partial load is possible
        }


    protected:

        static const int kNeoCodeTab        =   -9;     /**< Translated character code is a TAB. */
        static const int kNeoCodeNewline    =   -10;    /**< Translated character code is a new-line. */
        static const int kNeoCodeReturn     =   -13;    /**< Translated character code is a carriage return. */
        static const int kNeoCodeUnknown    =   -256;   /**< No known translation for the character. */

        static const uint8_t kNeoUntranslatableCharacter = 0;   /**< Neo character code used for untranslatable characters. */

        /** Convert a unicode character to Neo format (CP1252).
         *
         *  @param  uni     The unicode.
         *  @return         An extended Neo format character code. Values in the range 0-255 indicate a Neo display
         *                  code. Values < 0 indicate a control or status code (eg: @e kNeoCodeTab).
         */
        int unicodeToNeo(uint16_t uni) const;


        /** Convert a Neo character to unicode.
         *
         *  @param  uni     The unicode.
         *  @return         A Neo format character.
         */
        uint16_t neoToUnicode(int neo) const;


        /** Convert a unicode string to Neo format (CP1252). Handles BOM markers and endian conversion.
         *
         *  @param  neo     The output buffer (Neo characters).
         *  @param  uni     The input buffer (unicode characters).
         *  @param  count   The number of characters to convert.
         *  @param  escape  If true, ASCII control characters 9,10, 13 are passed though and not interpreted as character codes.
         *  @return         The actual number of characters converted (may be less than @e count if a BOM was present).
         */
        unsigned unicodeToNeo(uint8_t* neo, const uint16_t* uni, unsigned count, bool escape=true) const;


        /** Convert a Neo character to unicode. Handles BOM markers and endian conversion.
         *
         *  @param  uni     The output buffer (unicode characters).
         *  @param  neo     The input buffer (Neo characters).
         *  @param  count   The number of characters to convert.
         *  @param  bom     Logical true to prepend a BOM marker to the output.
         *  @return         The actual number of characters converted (may be one more than @e count if a BOM was requested).
         */
        unsigned neoToUnicode(uint16_t* uni, const uint8_t* neo, unsigned count, bool bom=false) const;


        /** Change the size of the raw data buffer. The buffer is either
         *  truncated or padded with zero bytes as appropriate. If this
         *  routine fails, any pre-existing data will have been preserved.
         *
         *  @param  size    The new size, in bytes. Passing zero will force
         *                  any existing memory to be released.
         *  @return         The new data pointer. Returns zero if size is zero
         *                  or if the requested memory could not be allocated.
         */
        void* setFileSize(unsigned size);


        /** Append data to the file. This will try to increase the file size and copy the
         *  specified data to the end of the object.
         *
         *  @return         True if the request succeeded.
         */
        bool appendData(void* data, unsigned size);


        /** Notification to a derived class that new data has been loaded.
         *  Override this methods to perform any initial pre-processing of
         *  data or validation.
         *
         *  @return         Logical true if the data was valid, or false otherwise.
         */
        virtual bool confirmLoad();


        /** Notification to a derived class that the current data is to be saved.
         *  Override this methods to perform any final post-processing or validation.
         *
         *  @param  device      The device handle.
         *  @param  applet      The applet.
         *  @param  fileIndex   The file index number.
         *  @return             Logical true to proceed with the save or false to abort with an error.
         */
        virtual bool confirmSave(ASDevice* device, const ASApplet* applet, int fileIndex);


        /** Convenience alternative to fileData() that returns a non-const byte pointer.
         */
        uint8_t* byteData() const { return (uint8_t*) m_fileData; }


    private:

        ASAppletID m_appletID;
        int m_appletVersionMajor;
        int m_appletVersionMinor;
        bool m_haveAppletInfo;

        void* m_fileData;
        unsigned m_fileSize;

        ASFile(const ASFile&);              /**< Prevent the use of the copy operator. */
        ASFile& operator=(const ASFile&);   /**< Prevent the use of the assignment operator. */
    };

}   // namespace

#endif      // COM_TSONIQ_ASUserDictionary_H
