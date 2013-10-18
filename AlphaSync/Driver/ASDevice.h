/** @file   ASDevice.h
 *  @brief  ALphasmart device representation.
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

#ifndef COM_TSONIQ_ASDevice_H
#define COM_TSONIQ_ASDevice_H   (1)

#include <stdint.h>
#include "ASAppletID.h"
#include "ASMessage.h"
#include "ASFileAttributes.h"
#include "ASApplet.h"
#include "AQContainer.h"

namespace ts
{
    /** Device object. This represents a single physical instance of a Neo or similar device in comms mode.
     *
     *  Device objects may only be created or destroyed by an instance of ASDeviceFactory, which
     *  manages USB plug-and-play handling.
     *
     *  Most routines return a boolean result. If this is true, the request was executed successfully, but if
     *  false there is a communication failure with the device. A false return may indicate that the device
     *  has been unplugged, so the client should stop trying any further dialogue.
     */
    class ASDevice
    {
    public:

        ASDevice()
            :
            m_identity(0),
            m_appletHeaderData(0),
            m_appletHeaderCount(0),
            m_applets()
        {
            // Nothing
        }


        virtual ~ASDevice()
        {
            clearEnumeratedApplets();
        }

        unsigned identity() const { return m_identity; }

        bool restart();

        bool systemVersion(unsigned* major, unsigned* minor, char systemName[64], char systemDate[64]);
        bool systemMemory(unsigned* ram, unsigned* rom);

        const ASApplet* appletAtIndex(int appletIndex);
        const ASApplet* appletForID(ASAppletID appletID);

        bool getAppletResourceUsage(unsigned* fc, unsigned* ram, const ASApplet* applet);

        bool getFileAttributes(ASFileAttributes* attr, const ASApplet* applet, int fileIndex);
        bool setFileAttributes(const ASApplet* applet, int fileIndex, const ASFileAttributes* attr);

        int indexForFileWithName(const ASApplet* applet, const char* name)
        {
            (void)applet;
            (void)name;
            return -1;  // REVIEW: implement me?
        }
        unsigned fileSize(const ASApplet* applet, int fileIndex);
        bool readFile(void* buffer, unsigned size, unsigned* actual, const ASApplet* applet, int fileIndex, bool raw);
        bool createFile(const char* filename, const char* password, const void* buffer, unsigned size, const ASApplet* applet, int* fileIndex, bool raw);
        bool writeFile(const void* buffer, unsigned size, const ASApplet* applet, int fileIndex, bool raw);

        bool clearAllFiles(const ASApplet* applet);
        bool clearFile(const ASApplet* applet, int fileIndex);

        bool readSettings(void* buffer, unsigned* actual, unsigned size, const ASApplet* applet, unsigned flags);

    protected:

        unsigned m_identity;                    /**< The USB identity. */

        // Low-level OS applet commands
        bool rawReadAppletHeaders(uint8_t* buffer, int index, unsigned count, unsigned* actual);
        bool rawCreateFile(ASAppletID applet, int index);
        bool rawGetFileAttributes(uint8_t attr[kASFileAttributesSize], ASAppletID applet, int index, unsigned* actual);
        bool rawSetFileAttributes(const uint8_t attr[kASFileAttributesSize], ASAppletID applet, int index);
        bool rawReadFile(void* dest, unsigned size, unsigned* actual, ASAppletID applet, int index, bool raw=false);
        bool rawWriteFile(const void* source, unsigned size, ASAppletID applet, int index, bool raw=false);


        /** The derived class should call this once it has completed its initialisation.
         */
        void initialise();


        /** Read data from the device.
         *
         *  @param  buffer      Buffer memory to receive the data.
         *  @param  length      Specifies the number of bytes to read.
         *  @param  actual      The actual number of bytes read. This may be less than requested
         *                      if a short read was encountered. If a zero ptr is supplied then
         *                      a short read is treated as an error.
         *  @param  timeout     Specifies the timeout, in ms. If zero, a default is applied.
         *  @return             Logical true if the request succeeded, or false if it failed.
         */
        virtual bool read(void* buffer, unsigned length, unsigned* actual=0, unsigned timeout=0) = 0;


        /** Write data to the device.
         *
         *  @param  buffer      Buffer memory containing the data to write.
         *  @param  length      Specifies the number of bytes to write.
         *  @param  timeout     Specifies the timeout, in ms. If zero, a default is applied.
         *  @return             Logical true if the request succeeded, or false if it failed.
         */
        virtual bool write(const void* buffer, unsigned length, unsigned timeout=0) = 0;


    private:

        uint8_t* m_appletHeaderData;                        /**< Locally cached copy of the applet header data. */
        unsigned m_appletHeaderCount;                       /**< The number of applet headers present on the device. */
        AQContainer<ASApplet> m_applets;                    /**< Applet list for the device. */

        void clearEnumeratedApplets();
        bool sendRequest(const ASMessage* request);
        bool getResponse(ASMessage* response);
        bool sendRequestAndGetResponse(ASMessage* message);
        bool sendRequestAndGetResponse(ASMessage* message, unsigned code);
        bool readExtendedData(void *dest, unsigned size, unsigned* actual);
        bool writeExtendedData(const void* source, unsigned size);
        unsigned calculateDataChecksum(const void *data, unsigned int length) const;

        // Basic protocol
        bool hello();
        bool reset();
        bool switchApplet(ASAppletID applet=kASAppletID_System);

        // Framing for command transactions
        bool dialogueStart(ASAppletID applet=kASAppletID_System)
        {
            return hello() && reset() && switchApplet(applet);
        }

        bool dialogueEnd(bool status)
        {
            reset();
            return status;
        }


        ASDevice(const ASDevice&);              /**< Prevent the use of the copy constructor. */
        ASDevice& operator=(const ASDevice&);   /**< Prevent the use of the assignment operator. */
    };

}   // namespace

#endif      // COM_TSONIQ_ASDevice_H
