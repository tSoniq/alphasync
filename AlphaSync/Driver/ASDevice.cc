/** @file   ASDevice.cc
 *  @brief  ALphaSmart device representation.
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

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include "ASDevice.h"
#include "ASMessage.h"
#include "ASApplet.h"

#define kASMProtocolVersion 0x0220          /**< Minimum ASM protocol version that the device must support. */

namespace ts
{
    /** Clear enumerated applet data.
     */
    void ASDevice::clearEnumeratedApplets()
    {
        for (unsigned i = 0; i < m_applets.count(); i++)
        {
            delete m_applets.itemAtIndex(i);
        }

        m_applets.removeAllItems();
    }


    /** Load enumerated (cached) data. This may be called explicitly to force loading of the
     *  cache. However, routines that rely on the cached data will trigger an implicit enumeration
     *  if needed.
     *
     *  @return             Logical true if the device was loaded successfully.
     */
    void ASDevice::initialise()
    {
        assert(0 == m_applets.count());

        bool result = dialogueStart();
        if (!result) return;

        /* Loop to load the applet header data. Note: do not try to read more than 7 headers at a time.
         * Reading more than this causes an internal overflow in an attached Neo (likely an internal 1k buffer size).
         */
        unsigned appletCount = 0;
        while (true)
        {
            uint8_t buffer[kASAppletHeaderSize * 7];
            unsigned headerCount;
            result = rawReadAppletHeaders(buffer, (int)appletCount, 7, &headerCount);
            if (!result) break;


            /* Append to the cached list of applets. The index in to m_enumeratedData is the index
             * used on the device.
             */
            for (unsigned i = 0; i < headerCount; i++)
            {
                ASApplet* applet = new ASApplet;
                applet->loadHeader(&buffer[i * kASAppletHeaderSize]);
                m_applets.appendItem(applet);
            }

            appletCount += headerCount;

            if (headerCount < 7) break;     // short read, so no more attributes to fetch
        }

        dialogueEnd(result);
    }



    /** Read a file.
     *
     *  @param  buffer      The buffer to read in to.
     *  @param  size        The maximum number of bytes to read.
     *  @param  actual      Returns the actual number of bytes read.
     *  @param  applet      The applet.
     *  @param  fileIndex   The file index number.
     *  @param  raw         The logical true for raw file read, false for cooked.
     *  @return             Logical false if there was an IO error. Note that appletIndex or fileIndex values
     *                      that are out of range will result in a true return value and *actual = 0. Use
     *                      @e fileExists() to test if a file physically exists on the device, since files
     *                      may have zero length.
     */
    bool ASDevice::readFile(void* buffer, unsigned size, unsigned* actual, const ASApplet* applet, int fileIndex, bool raw)
    {
        *actual = 0;
        memset(buffer, 0, size);

        bool result = dialogueStart();
        if (result) result = rawReadFile(buffer, size, actual, applet->appletID(), fileIndex, raw);
        return dialogueEnd(result);
    }


    /** Create a new file.
     *
     *  @param  buffer      The buffer to read in to.
     *  @param  size        The maximum number of bytes to read.
     *  @param  applet      The applet.
     *  @param  fileIndex   Returns the new file index number.
     *  @param  raw         The logical true for raw file read, false for cooked.
     *  @return             Logical false if there was an IO error.
     *
     *  The sequence for creating a new file is a little counter intuitive, starting with the file attributes:
     *
     *      --> REQUEST_SET_FILE_ATTRIBUTES     ; set up the attributes (see rawWriteAttributes())
     *      <-- RESPONSE_SET_FILE_ATTRIBUTES
     *      --> REQUEST_BLOCK_WRITE
     *      --> Attribute data
     *      <-- RESPONSE_BLOCK_WRITE_DONE
     *      --> REQUEST_COMMIT                  ; create the file
     *      <-- RESPONSE_COMMIT
     *      --> REQUEST_WRITE_RAW_FILE          ; the following sequence is as for writing an existing file (see rawWriteFile())
     *      <-- RESPONSE_WRITE_FILE
     *      --> REQUEST_BLOCK_WRITE
     *      --> File data
     *      <-- RESPONSE_BLOCK_WRITE_DONE
     *      --> REQUEST_CONFIRM_WRITE_FILE
     *      <-- RESPONSE_CONFIRM_WRITE_FILE
     */
    bool ASDevice::createFile(const char* filename, const char* password, const void* buffer, unsigned size, const ASApplet* applet, int* fileIndex, bool raw)
    {
        unsigned appletRam;
        unsigned appletFileCount;
        unsigned freeRam;
        unsigned freeRom;
        if (!getAppletResourceUsage(&appletFileCount, &appletRam, applet)) return false;
        if (!systemMemory(&freeRam, &freeRom)) return false;
        if (size + 1024 > freeRam) return false;        // REVIEW: arbitrarily choosing to keep at least 1k unused on the device

        *fileIndex = (int)(appletFileCount + 1);

        bool result = dialogueStart();
        if (!result) return false;

        ASFileAttributes attr;
        attr.setFileName(filename);
        attr.setPassword(password);
        attr.setAllocSize(size);
        attr.setMinSize(size);
        attr.setFileSpace(0);           // unbound
        result = rawSetFileAttributes(attr.rawData(), applet->appletID(), *fileIndex);
        if (result)
        {
            ASMessage message;
            message.init(ASMESSAGE_REQUEST_COMMIT);             // Sending this message appears to bind the attributes to a new
            message.setArgument((unsigned)*fileIndex, 4, 1);    // file - not sending it will still result in a new file, but
            message.setArgument(applet->appletID(), 5, 2);      // the attributes will not be correct.
            result = sendRequestAndGetResponse(&message);
            if (result && ASMESSAGE_RESPONSE_COMMIT == message.command())
            {
                result = rawWriteFile(buffer, size, applet->appletID(), *fileIndex, raw);
            }
        }

        return dialogueEnd(result);
    }


    /** Write a file.
     *
     *  @param  buffer      The buffer to read in to.
     *  @param  size        The maximum number of bytes to read.
     *  @param  applet      The applet.
     *  @param  fileIndex   The file index number.
     *  @param  raw         The logical true for raw file read, false for cooked.
     *  @return             Logical false if there was an IO error. Note that appletIndex or fileIndex values
     *                      that are out of range will result in an IO error.
     */
    bool ASDevice::writeFile(const void* buffer, unsigned size, const ASApplet* applet, int fileIndex, bool raw)
    {
        bool result = dialogueStart();
        if (result) result = rawWriteFile(buffer, size, applet->appletID(), fileIndex, raw);
        return dialogueEnd(result);
    }


    /** Clear the contents of a single file.
     *
     *  @param  applet          The applet.
     *  @param  fileIndex       The file index.
     *  @return                 Logical true if succeeded.
     */
    bool ASDevice::clearFile(const ASApplet* applet, int fileIndex)
    {
        bool result = dialogueStart();
        if (!result) return false;

        ASFileAttributes attr;
        result = getFileAttributes(&attr, applet, fileIndex);
        if (result)
        {
            attr.setAllocSize(0);
            attr.setMinSize(0);
            result = rawSetFileAttributes(attr.rawData(), applet->appletID(), fileIndex);
            if (result)
            {
                ASMessage message;
                message.init(ASMESSAGE_REQUEST_COMMIT);         // Sending this message appears to bind the attributes to a new
                message.setArgument((unsigned)fileIndex, 4, 1); // file - not sending it will still result in a new file, but
                message.setArgument(applet->appletID(), 5, 2);  // the attributes will not be correct.
                result = sendRequestAndGetResponse(&message);
                if (result && ASMESSAGE_RESPONSE_COMMIT == message.command())
                {
                    result = rawWriteFile(0, 0, applet->appletID(), fileIndex, true);
                }
            }
        }

        return dialogueEnd(result);
    }


    /** Clear all files associated with an applet.
     *
     *  @param  applet          The applet index number.
     *  @return                 Logical true if the request succeeded.
     */
    bool ASDevice::clearAllFiles(const ASApplet* applet)
    {
        bool result = dialogueStart();
        if (!result) return result;

        uint8_t settings[12];
        as_EndianWriteU16(&settings[ 0], 0x0103);        // type
        as_EndianWriteU16(&settings[ 2], 0x8003);        // ident (Delete All Files?)
        as_EndianWriteU16(&settings[ 4], 0x0006);        // length
        as_EndianWriteU16(&settings[ 6], 0x1001);        // YES (this is the value that is applied)
        as_EndianWriteU16(&settings[ 8], 0x1001);        // YES
        as_EndianWriteU16(&settings[10], 0x1002);        // NO

        ASMessage message;
        message.init(ASMESSAGE_REQUEST_SET_SETTINGS);
        message.setArgument(sizeof settings, 1, 4);
        message.setArgument(calculateDataChecksum(settings, sizeof settings), 5, 2);
        result = sendRequestAndGetResponse(&message);
        if (ASMESSAGE_RESPONSE_BLOCK_WRITE != message.command())  result = false;
        if (result)
        {
            result = write(settings, sizeof settings);
            if (result)
            {
                result = getResponse(&message);
                if (ASMESSAGE_RESPONSE_BLOCK_WRITE_DONE != message.command())  result = false;
                if (result)
                {
                    message.setCommand(ASMESSAGE_REQUEST_SET_APPLET);
                    message.setArgument(0, 1, 4);
                    message.setArgument(applet->appletID(), 5, 2);
                    result = sendRequestAndGetResponse(&message);
                    if (ASMESSAGE_RESPONSE_SET_APPLET != message.command()) result = false;
                }
            }
        }

        return dialogueEnd(result);
    }



    /** Read the settings data for an applet.
     *
     *  Interpretatipon of the settings 'flags' are not clear. The following values appear to work:
     *
     *      0x0b    -   use to read the actual settings for an applet (without private passwords) - used by ASM
     *      0x0f    -   use to read the actual settings for an applet (with private passwords)
     *      0x10    -   use to read the system settings (use with applet ID 0x0000 only).
     *
     *  @param  buffer  The buffer to load the settings in to.
     *  @param  actual  Returns the number of bytes written in to the buffer.
     *  @param  size    The size of the buffer.
     *  @param  applet  The applet.
     *  @param  flags   The flags to select the data.
     *  @return         Logical true if the request succeeded.
     */
    bool ASDevice::readSettings(void* buffer, unsigned* actual, unsigned size, const ASApplet* applet, unsigned flags)
    {
        *actual = 0;

        bool result = dialogueStart();
        if (!result) return result;

        ASMessage message;
        message.init(ASMESSAGE_REQUEST_GET_SETTINGS);
        message.setArgument(flags, 1, 4);               // flags
        message.setArgument(applet->appletID(), 5, 2);  // applet id
        result = sendRequestAndGetResponse(&message);
        if (ASMESSAGE_RESPONSE_GET_SETTINGS != message.command())  result = false;
        if (result)
        {
            unsigned responseSize = message.argument(1, 4);
            unsigned expectedChecksum = message.argument(5, 2);
            unsigned actualBytes = 0;

            if (size < responseSize)
            {
                result = false;     // caller's buffer is too small
                unsigned remaining = responseSize;
                uint8_t tmp[1024];
                while (remaining > 0)
                {
                    unsigned blockSize = (sizeof tmp <  remaining) ? sizeof tmp : remaining;
                    if (!read(tmp, blockSize, &actualBytes)) break;
                    remaining -= actualBytes;
                }
            }
            else
            {
                result = read(buffer, responseSize, &actualBytes);
                result = result && (calculateDataChecksum(buffer, actualBytes) == expectedChecksum);
                *actual = actualBytes;
            }
        }
        return dialogueEnd(result);
    }






#pragma mark    --------  Raw access methods  --------





    /** Read a block of applet headers.
     *
     *  @param  buffer      Byte buffer of length @e count * kASAppletHeaderSize.
     *  @param  index       The index of the first header.
     *  @param  count       The number of headers to try to read (max 7).
     *  @param  actual      Returns the actual number of headers read.
     *  @return             Logical true if the operation succeeded, false if an IO error occurred.
     */
    bool ASDevice::rawReadAppletHeaders(uint8_t* buffer, int index, unsigned count, unsigned* actual)
    {
        assert(count <= 7);     // Reading more than 7 headers will cause a crash on some Neos (1k buffer overflow?)

        *actual = 0;
        memset(buffer, 0, kASAppletHeaderSize * count);

        if (count > 7) return false;    // Don't trash the Neo!

        ASMessage message(ASMESSAGE_REQUEST_LIST_APPLETS);
        message.setArgument((unsigned)index, 1, 4);
        message.setArgument(count, 5, 2);
        bool result = (sendRequestAndGetResponse(&message), ASMESSAGE_RESPONSE_LIST_APPLETS);
        if (!result) return false;                              // Error handling command

        unsigned size = message.argument(1, 4);
        unsigned expectedChecksum = message.argument(5, 2);

        if (size > (count * kASAppletHeaderSize))
        {
            fprintf(stderr, "%s: reply will return too much data!\n", __FUNCTION__);
            return false;                                       // Unexpected data overrun
        }

        if (0 == size) return true;                             // no (more) applets present

        unsigned actualBytes;
        result = read(buffer, size, &actualBytes);
        if (!result || actualBytes != size) return false;       // Unexpected read failure

        if ((actualBytes % kASAppletHeaderSize) != 0)
        {
            fprintf(stderr, "%s: warning: read returned a partial header (expected header size %u, bytes read %u)\n",
                __FUNCTION__, kASAppletHeaderSize, actualBytes);
            assert(0);
            // the partial header will be ignored rather than treated as an error (unless the checksum is also invalid)
        }

        unsigned actualChecksum = calculateDataChecksum(buffer, actualBytes);
        if (actualChecksum != expectedChecksum)
        {
            fprintf(stderr, "%s: data checksum error\n", __FUNCTION__);
            return false;
        }

        *actual = actualBytes / kASAppletHeaderSize;            // Return the actual number of headers read
        return true;
    }


    /** Return the applet object for a specified index.
     */
    const ASApplet* ASDevice::appletAtIndex(int appletIndex)
    {
        return m_applets.itemAtIndex((unsigned)appletIndex);
    }


    /** Return the applet object for a specified applet ID.
     */
    const ASApplet* ASDevice::appletForID(ASAppletID appletID)
    {
        int index = 0;
        const ASApplet* applet;
        while ((applet = m_applets.itemAtIndex((unsigned)index)))
        {
            if (applet->appletID() == appletID) return applet;
            ++ index;
        }
        return 0;
    }


    /** Return the file attributes for a given applet and file index.
     */
    bool ASDevice::getFileAttributes(ASFileAttributes* attr, const ASApplet* applet, int fileIndex)
    {
        bool result = dialogueStart();
        if (!result) return false;

        uint8_t abuffer[kASFileAttributesSize];
        unsigned actual;

        result = rawGetFileAttributes(abuffer, applet->appletID(), fileIndex++, &actual);
        if (result)
        {
            if (actual == 0)
            {
                result = false;         // No such file.
            }
            else
            {
                attr->copyFrom(abuffer);
            }
        }

        return dialogueEnd(result);
    }


    /** Set file attributes.
     *
     *  @param  attr        The attributes to set.
     *  @param  applet      The applet.
     *  @param  fileIndex   The file number.
     *  @return             Logical true if there was no error.
     */
    bool ASDevice::setFileAttributes(const ASApplet* applet, int fileIndex, const ASFileAttributes* attr)
    {
        if (0 == strlen(attr->fileName())) return false;        // zero length filenames will cause the Neo to crash
        if (0 == strlen(attr->password())) return false;        // presumably zero length passwords are also bad...

        if (!dialogueStart()) return false;
        bool result = rawSetFileAttributes(attr->rawData(), applet->appletID(), fileIndex);
        if (result)
        {
            ASMessage message;
            message.init(ASMESSAGE_REQUEST_COMMIT);         // Sending this message appears to bind the attributes to a new
            message.setArgument((unsigned)fileIndex, 4, 1); // file - not sending it will still result in a new file, but
            message.setArgument(applet->appletID(), 5, 2);  // the attributes will not be correct.
            result = sendRequestAndGetResponse(&message);
            result = (result && ASMESSAGE_RESPONSE_COMMIT == message.command());
        }
        return dialogueEnd(result);
    }



    /** Find the resources currently being used by an applet.
     *
     *  @param  fc          Returns the number of files used.
     *  @param  ram         Returns the amount of RAM used by the applet in bytes.
     *  @param  applet      The applet.
     *  @return             Logical true if the request succeeded.
     */
    bool ASDevice::getAppletResourceUsage(unsigned* fc, unsigned* ram, const ASApplet* applet)
    {
        *fc = 0;
        *ram = 0;

        if (!dialogueStart()) return false;

        ASMessage message;
        message.init(ASMESSAGE_REQUEST_GET_USED_SPACE);
        message.setArgument(0x00000001, 1, 4);              // set zero to get the size of the largest file, non-zero for all files
        message.setArgument(applet->appletID(), 5, 2);
        bool result = sendRequestAndGetResponse(&message);
        if (result && message.command() == ASMESSAGE_RESPONSE_GET_USED_SPACE)
        {
            *ram = message.argument(1, 4);
            *fc = message.argument(5, 2);
        }
        return dialogueEnd(result);
    }



    /** Ping the device for the ASM protocol version number. This will put the Neo in
     *  to ASM mode and also return the protocol version. It is also used as a keep-alive
     *  test.
     */
    bool ASDevice::hello()
    {
        static const uint8_t ascCommandRequestProtocol[1] = { 0x01 };
        uint8_t buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        unsigned actual = 0;
        unsigned version = 0;
        unsigned retry = 10;

        bool ok = write(ascCommandRequestProtocol, 1, 100)  &&  read(buffer, 8, &actual, 100);
        while (!ok || actual != 2)
        {
            fprintf(stderr, "%s: unexpected %u byte response: ", __FUNCTION__, actual);
            for (unsigned i = 0; i < actual; i++) fprintf(stderr, " %02x", buffer[i]);
            fprintf(stderr, "\n");

            retry --;
            if (0 == retry)
            {
                fprintf(stderr, "%s: this device doesn't look like it wants to talk to us - bailing out.\n", __FUNCTION__);
                return false;
            }

            reset();        // try to issue a protocol reset
            usleep(100000); // give the device a little time to see and handle the reset
            ok = write(ascCommandRequestProtocol, 1, 100)  &&  read(buffer, 8, &actual, 100);
        }

        version = ((unsigned)buffer[0] << 8) | ((unsigned)buffer[1] << 0);
        if (version < kASMProtocolVersion)
        {
            fprintf(stderr, "%s: ASM protocol version not supported: %0x04x\n", __FUNCTION__, version);
            ok = false;
        }

        return ok;
    }



    /** Reset the device to a known state.
     *
     *  @return Logical true if the device is supported and working correctly.
     */
    bool ASDevice::reset()
    {
        static const uint8_t ascCommandRequestReset[8] = { 0x3f, 0xff, 0x00, 0x72, 0x65, 0x73, 0x65, 0x74 };


        // Send a reset command
        bool ok = write(ascCommandRequestReset, sizeof ascCommandRequestReset);
        if (!ok)
        {
            fprintf(stderr, "%s: Failed to send reset command to Neo.\n", __FUNCTION__);
        }
        return ok;
    }


    /** Switch communication to a specific applet.
     *
     *  @return Logical true if the device is supported and working correctly.
     */
    bool ASDevice::switchApplet(ASAppletID applet)
    {
        static const uint8_t ascCommandRequestSwitch[8] = { 0x3f, 0x53, 0x77, 0x74, 0x63, 0x68, 0x00, 0x00 };
        static const uint8_t ascCommandResponseSwitched[8] = { 0x53, 0x77, 0x69, 0x74, 0x63, 0x68, 0x65, 0x64 };
        uint8_t buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        bool ok;

        memcpy(buffer, ascCommandRequestSwitch, sizeof buffer);
        buffer[6] = (applet >> 8) & 0xff;
        buffer[7] = (applet >> 0) & 0xff;
        ok = write(buffer, sizeof buffer) && read(buffer, sizeof buffer);
        if (!ok)
        {
            fprintf(stderr, "%s: failed to send switch command to device\n", __FUNCTION__);
            return ok;
        }

        ok = (0 == memcmp(buffer, ascCommandResponseSwitched, 8));
        if (!ok)
        {
            fprintf(stderr, "%s: failed to switch to applet %04x:  ", __FUNCTION__, applet);
            for (unsigned i = 0; i < 8; i++) fprintf(stderr, "%c", isprint(buffer[i]) ? buffer[i] : '.');
            fprintf(stderr, "\n");
        }

        return ok;
    }



    /** Send a command.
     *
     *  @param  request     The command message.
     *  @return             Logical true if the message was sent.
     */
    bool ASDevice::sendRequest(const ASMessage* request)
    {
        bool result = write(request->rawData(), request->rawSize());
        if (!result) fprintf(stderr, "%s: error sending to device\n", __FUNCTION__);
        return result;
    }


    /** Get a response to a command.
     *
     *  @param  response    Message to receive the response.
     *  @return             Logical true if the operation completed.
     */
    bool ASDevice::getResponse(ASMessage* response)
    {
        bool result = read(response->rawData(), response->rawSize());
        if (!result) fprintf(stderr, "%s: error reading from device\n", __FUNCTION__);
        return result;
    }


    /** Send a message and get the response.
     *
     *  @param  message     On entry, holds the command message to send. On return, contains the reply.
     *  @return             Logical true if the operation completed successfully.
     */
    bool ASDevice::sendRequestAndGetResponse(ASMessage* message)
    {
        return sendRequest(message) && getResponse(message);
    }


    /** Send a message and get the response, checking for an explicit response code. If the response
     *  code is not matched and error will be returned.
     *
     *  @param  message     On entry, holds the command message to send. On return, contains the reply.
     *  @param  code        The message code to validate.
     *  @return             Logical true if the operation completed successfully.
     */
    bool ASDevice::sendRequestAndGetResponse(ASMessage* message, unsigned code)
    {
        unsigned sendCode = message->command();
        bool result = sendRequest(message) && getResponse(message);
        if (result && (code != message->command()))
        {
            fprintf(stderr, "%s: unexpected message response: send %02x, received %02x, expected %02x\n", __FUNCTION__, sendCode, message->command(), code);
            result = false;
        }
        return result;
    }



    /** Calculate a data checksum from a block of data.
     *
     *  @param  data        The data to checksum.
     *  @param  length      The number of bytes present.
     *  @return             The checksum.
     */
    unsigned ASDevice::calculateDataChecksum(const void* data, unsigned length) const
    {
        const uint8_t* ptr = (const uint8_t*) data;
        const uint8_t* end = ptr + length;

        unsigned result = 0;
        while (ptr != end)
        {
            result += ((unsigned)(*ptr++)) & 0xff;
        }

        return result & 0xffff;
    }



    /** Read binary data blocks in response to some other command, handling segmentation
     *  and checksum validation.
     *
     *  The command sequence is:
     *
     *      While data left to read
     *          OUT:    0x10    ASMESSAGE_REQUEST_BLOCK_READ
     *          IN:     0x4d    ASMESSAGE_RESPONSE_BLOCK_READ
     *          OUT:    data
     *
     *  @param  dest        Where to put the data.
     *  @param  size        The number of bytes that are expected to be delivered.
     *  @param  actual      Used to return the number of bytes actually read.
     *  @return             The actual number of bytes obtained.
     */
    bool ASDevice::readExtendedData(void* dest, unsigned size, unsigned* actual)
    {
        uint8_t* ptr = (uint8_t*) dest;
        unsigned bytesread = 0;
        bool ok = true;

        const ASMessage request(ASMESSAGE_REQUEST_BLOCK_READ);
        ASMessage response;

        unsigned remaining = size;
        while (remaining > 0)
        {
            if (!sendRequest(&request) || !getResponse(&response))
            {
                fprintf(stderr, "Error sending commands\n");
                ok = false;
                break;
            }
            else if (response.command() == ASMESSAGE_RESPONSE_BLOCK_READ_EMPTY)
            {
                // No more data to return
                break;
            }
            else if (response.command() == ASMESSAGE_RESPONSE_BLOCK_READ)
            {
                unsigned blocksize = response.argument(1, 4);
                unsigned checksum = response.argument(5, 2);
                ok = read(ptr, blocksize);
                if (!ok)
                {
                    fprintf(stderr, "%s: error reading data\n", __FUNCTION__);
                    ok = false;
                    break;
                }
                else if (calculateDataChecksum(ptr, blocksize) != checksum)
                {
                    fprintf(stderr, "%s: bad checksum: expected %04x, got %04x\n", __FUNCTION__, checksum, calculateDataChecksum(ptr, blocksize));
                    ok = false;
                    break;
                }
                else
                {
                    ptr += blocksize;
                    bytesread += blocksize;
                }
            }
            else
            {
                fprintf(stderr, "%s: unexpected response\n", __FUNCTION__);
                ok = false;
                break;
            }
        }

        assert(bytesread <= size);
        if (actual) *actual = bytesread;

        if (!ok) fprintf(stderr, "%s: error. last request %02x, response %02x\n", __FUNCTION__, request.command(), response.command());
        return ok;
    }






    /** Write a block of binary data in response to a command.
     *
     *  This manages the following sequence of commands:
     *
     *      While data left to send:
     *          OUT:    0x02    ASMESSAGE_REQUEST_BLOCK_WRITE
     *          IN:     0x42    ASMESSAGE_RESPONSE_BLOCK_WRITE
     *          OUT:    data
     *          IN:     0x43    ASMESSAGE_RESPONSE_BLOCK_WRITE_DONE
     *
     *  @param  dest        The data.
     *  @param  size        The number of bytes that are to be written.
     *  @return             Logical true if the operation succeeded, false otherwise.
     */
    bool ASDevice::writeExtendedData(const void* source, unsigned size)
    {
        ASMessage request;
        ASMessage response;

        unsigned remaining = size;
        const uint8_t* ptr = (const uint8_t*) source;

        while (remaining > 0)
        {
            unsigned blocksize = (remaining < 1024) ? remaining : 1024;
            unsigned checksum = calculateDataChecksum(ptr, blocksize);
            request.init(ASMESSAGE_REQUEST_BLOCK_WRITE);
            request.setArgument(blocksize, 1, 4);
            request.setArgument(checksum, 5, 2);
            if (!sendRequest(&request)) goto error;
            if (!getResponse(&response)) goto error;
            if (ASMESSAGE_RESPONSE_BLOCK_WRITE != response.command()) goto error;
            if (!write(ptr, blocksize)) goto error;
            if (!getResponse(&response)) goto error;
            if (ASMESSAGE_RESPONSE_BLOCK_WRITE_DONE != response.command()) goto error;

            remaining -= blocksize;
            ptr += blocksize;
        }

        return true;

    error:

        fprintf(stderr, "Error in %s: last request: %02x, last response: %02x\n", __FUNCTION__, request.command(), response.command());
        return 0;
    }



    #pragma mark    ---------------- Public API providing access to the protocol ----------------



    /** Send a restart request to the Neo. The should cause the Neo to reset and revert back to its HID state.
     *
     *  @return     Logical true if the request completed successfully.
     */
    bool ASDevice::restart()
    {
        bool result = dialogueStart();
        if (!result) return result;

        ASMessage message(ASMESSAGE_REQUEST_RESTART);
        result = (sendRequestAndGetResponse(&message), ASMESSAGE_RESPONSE_RESTART);

        return dialogueEnd(result);
    }


    /** Obtain the OS version information.
     *
     *  @param  major       Returns the OS major version number.
     *  @param  minor       Returns the OS minor version number.
     *  @param  systemName  The build system name.
     *  @param  systemDate  The build system date.
     *  @return             Logical true if the request was successful.
     *
     *  @bug    Later UK Neo systems appear to report 3.6 on the device but 3.4 in response to this command.
     *          There also appear to be additional non-zero bytes at the end of the version information
     *          and the checksum is wrong by one.
     */
    bool ASDevice::systemVersion(unsigned* major, unsigned* minor, char systemName[64], char systemDate[64])
    {
        *major = 0;
        *minor = 0;
        systemName[0] = 0;
        systemDate[0] = 0;

        bool result = dialogueStart();
        if (!result) return result;


        ASMessage message(ASMESSAGE_REQUEST_VERSION);
        result = (sendRequestAndGetResponse(&message), ASMESSAGE_RESPONSE_VERSION);
        if (result)
        {
            unsigned size = message.argument(1, 4);
            unsigned expectedChecksum = message.argument(5, 2);

            uint8_t buffer[1024];
            if (size >= sizeof buffer) size = sizeof buffer - 1;
            memset(buffer, 0, sizeof buffer);

            unsigned actual;
            if (!read(buffer, size, &actual)) return false;

            buffer[actual] = 0;
            unsigned actualChecksum = calculateDataChecksum(buffer, actual);
            if (actualChecksum != expectedChecksum)
            {
                // OS 3.6 Neo device appear to calculate the checksum wrongly (off by one error?)
                fprintf(stderr, "%s: ignoring data checksum error: wanted %04x, got %04x\n", __FUNCTION__, expectedChecksum, actualChecksum);
            }
            /* The returned data appears to contain:
             *
             *  bytes   purpose
             *   0-3    unknown (appears to be a number calculated at run-time in the Neo)
             *   4-5    OS version number as major.minor (eg: 0x0301 for version 3.1).
             *   6-24   Human readable version as ASCII and zero terminator.
             *  25-63   Build date and time.
             */
            uint32_t unknown = uint32_t(buffer[0] << 24) | uint32_t(buffer[1] << 16) | uint32_t(buffer[2] << 8) | uint32_t(buffer[3] << 0);
            *major = buffer[4];
            *minor = buffer[5];
            const char* name = (const char*)&buffer[6];
            const char* date = name + strlen(name) + 1;
            strncpy(systemName, name, 64);
            strncpy(systemDate, date, 64);
            systemName[63] = 0;
            systemDate[63] = 0;
            for (long i = (long)strlen(systemName)-1; i >= 0 && isspace(systemName[i]); i--) systemName[i] = 0;
            for (long i = (long)strlen(systemDate)-1; i >= 0 && isspace(systemDate[i]); i--) systemDate[i] = 0;

            fprintf(stderr, "OS Revision %u.%u   %08x    '%s'  '%s'\n", *major, *minor, unknown, systemName, systemDate);
        }

        return dialogueEnd(result);
    }


    /** Query the remaining memory on the system.
     *
     *  @param  ram     Returns the free RAM space.
     *  @param  rom     Returns the free ROM space.
     *  @return         Logical true unless there was an IO error.
     */
    bool ASDevice::systemMemory(unsigned* ram, unsigned* rom)
    {
        *ram = 0;
        *rom = 0;

        if (!dialogueStart()) return false;

        ASMessage message(ASMESSAGE_REQUEST_GET_AVAIL_SPACE);
        bool result = sendRequestAndGetResponse(&message);
        if (result && message.command() == ASMESSAGE_RESPONSE_GET_AVAIL_SPACE)
        {
            *rom = message.argument(1, 4);
            *ram = (message.argument(5, 2) * 256);
        }
        return dialogueEnd(result);

    }






    /** Initialise a new file.
     *
     *  The command sequence is:
     *
     *      OUT:    0x1e    ASMESSAGE_REQUEST_COMMIT
     *      IN:     0x5c    ASMESSAGE_RESPONSE_COMMIT
     *
     *  @param  index       The file number.
     *  @param  applet      The applet ID.
     *  @return             Logical true if the operation succeeded, false otherwise.
     */
    bool ASDevice::rawCreateFile(ASAppletID applet, int index)
    {
        bool result = dialogueStart();
        if (!result) return false;

        ASMessage message(ASMESSAGE_REQUEST_COMMIT);
        message.setArgument((unsigned)index, 4, 1);
        message.setArgument(applet, 5, 2);
        result = sendRequestAndGetResponse(&message, ASMESSAGE_RESPONSE_COMMIT);

        return dialogueEnd(result);
    }





    /** Read the file information (attributes).
     *
     *  The command sequence is:
     *
     *      OUT:    0x13    ASMESSAGE_REQUEST_GET_FILE_ATTRIBUTES
     *      IN:     0x5a    ASMESSAGE_RESPONSE_GET_FILE_ATTRIBUTES
     *      IN:     data
     *
     *  @param  attr        Returns the file attribute data.
     *  @param  applet      The applet ID.
     *  @param  index       The file number.
     *  @param  actual      The actual number of headers read (either zero or one). If zero, no header
     *                      exists.
     *  @return             Logical true if all ok, or false if an IO error occurred.
     */
    bool ASDevice::rawGetFileAttributes(uint8_t attr[kASFileAttributesSize], ASAppletID applet, int index, unsigned* actual)
    {
        assert(index < 256);

        *actual = 0;
        memset(attr, 0, kASFileAttributesSize);

        ASMessage message;
        message.init(ASMESSAGE_REQUEST_GET_FILE_ATTRIBUTES);
        message.setArgument((unsigned)index, 4, 1);
        message.setArgument(applet, 5, 2);
        bool result = sendRequestAndGetResponse(&message);
        if (!result) return false;

        if (ASMESSAGE_ERROR_PARAMETER == message.command())
        {
            // Entry not found. This probably just means that the iteration has exceeded the number of files available.
            return true;
        }

        if (ASMESSAGE_RESPONSE_GET_FILE_ATTRIBUTES != message.command())
        {
            fprintf(stderr, "%s: unexpected response:", __FUNCTION__);
            message.dump(stderr);
            fprintf(stderr, "\n");
            return false;
        }

        unsigned length = message.argument(1, 4);
        unsigned checksum = message.argument(5, 2);

        if (length != kASFileAttributesSize)
        {
            fprintf(stderr, "%s: unexpected size %d for attribute data.\n", __FUNCTION__, length);
            return false;
        }

        if (!read(attr, kASFileAttributesSize))
        {
            fprintf(stderr, "%s: unexpected error reading attribute data.\n", __FUNCTION__);
            return false;
        }

        unsigned actualChecksum = calculateDataChecksum(attr, kASFileAttributesSize);
        if (!(actualChecksum == checksum))
        {
            fprintf(stderr, "%s: data checksum error: wanted %04x, got %04x.\n", __FUNCTION__, actualChecksum, checksum);
            return false;
        }

        *actual = 1;        // If here, we managed to successfully read the attributes
        return true;
    }



    /** Set the file information (attributes).
     *
     *  The command sequence is:
     *
     *      OUT:    0x1d    ASMESSAGE_REQUEST_SET_FILE_ATTRIBUTES
     *      IN:     0x5b    ASMESSAGE_RESPONSE_SET_FILE_ATTRIBUTES
     *      OUT:    0x02    ASMESSAGE_REQUEST_BLOCK_WRITE
     *      IN:     0x42    ASMESSAGE_RESPONSE_BLOCK_WRITE
     *      OUT:    data
     *      IN:     0x43    ASMESSAGE_RESPONSE_BLOCK_WRITE_DONE
     *
     *  @param  attr        The attribute data to be written.
     *  @param  applet      The applet ID.
     *  @param  index       The file number.
     *  @return             Logical true if the operation succeeded, false otherwise.
     */
    bool ASDevice::rawSetFileAttributes(const uint8_t attr[kASFileAttributesSize], ASAppletID applet, int index)
    {
        assert(index < 256);

        ASMessage message;

        message.init(ASMESSAGE_REQUEST_SET_FILE_ATTRIBUTES);
        message.setArgument((unsigned)index, 1, 4);
        message.setArgument(applet, 5, 2);

        bool result = sendRequestAndGetResponse(&message, ASMESSAGE_RESPONSE_SET_FILE_ATTRIBUTES);
        if (!result)
        {
            fprintf(stderr, "%s: unexpected response: ", __FUNCTION__);
            message.dump(stderr);
            fprintf(stderr, "\n");
        }
        else
        {
            result = writeExtendedData(attr, kASFileAttributesSize);
        }

        return result;
    }



    /** Read a file.
     *
     *  Transfer sequence:
     *
     *      OUT:    0x12|0x1c   ASMESSAGE_REQUEST_READ_FILE | ASMESSAGE_REQUEST_READ_RAW_FILE
     *      IN:     0x53        ASMESSAGE_RESPONSE_READ_FILE
     *      [block read sequence]
     *
     *  @param  dest        Where to put the data.
     *  @param  size        The maximum number of bytes to read.
     *  @param  actual      Used to return the actual number of bytes read.
     *  @param  applet      The applet ID.
     *  @param  index       The file number.
     *  @param  raw         Logical true to use WRITE-RAW rather than plain WRITE. Default false.
     *  @return             Logical true if the operation succeeded, false otherwise.
     */
    bool ASDevice::rawReadFile(void* dest, unsigned size, unsigned* actual, ASAppletID applet, int index, bool raw)
    {
        *actual = 0;

        ASMessage request;
        ASMessage response;

        request.init(raw ? ASMESSAGE_REQUEST_READ_RAW_FILE : ASMESSAGE_REQUEST_READ_FILE);
        request.setArgument(size, 1, 3);
        request.setArgument((unsigned)index, 4, 1);
        request.setArgument(applet, 5, 2);

        if (!sendRequest(&request)) goto error;
        if (!getResponse(&response)) goto error;
        if (!readExtendedData(dest, size, actual)) goto error;

        return true;

    error:
        fprintf(stderr, "Error in %s: last request: %02x, last response: %02x\n", __FUNCTION__, request.command(), response.command());
        return false;
    }



    /** Write a file.
     *
     *  @param  dest        The data.
     *  @param  size        The number of bytes that are to be written.
     *  @param  applet      The applet ID.
     *  @param  index       The file number.
     *  @param  raw         Logical true to use WRITE-RAW rather than plain WRITE. Default false.
     *  @return             Logical true if the operation succeeded, false otherwise.
     */
    bool ASDevice::rawWriteFile(const void* source, unsigned size, ASAppletID applet, int index, bool raw)
    {
        ASMessage request;
        ASMessage response;

        request.init(raw ? ASMESSAGE_REQUEST_WRITE_RAW_FILE : ASMESSAGE_REQUEST_WRITE_FILE);
        request.setArgument((unsigned)index, 1, 1);
        request.setArgument(size, 2, 3);
        request.setArgument(applet, 5, 2);
        if (!sendRequest(&request)) goto error;
        if (!getResponse(&response)) goto error;
        if (ASMESSAGE_RESPONSE_WRITE_FILE != response.command()) goto error;
        if (!writeExtendedData(source, size)) goto error;
        request.init(ASMESSAGE_REQUEST_CONFIRM_WRITE_FILE);
        if (!sendRequest(&request)) goto error;
        if (!getResponse(&response)) goto error;
        if (ASMESSAGE_RESPONSE_CONFIRM_WRITE_FILE != response.command()) goto error;

        return true;


    error:
        fprintf(stderr, "Error in %s: last request: %02x, last response: %02x\n", __FUNCTION__, request.command(), response.command());
        return false;
    }



}   // namespace
