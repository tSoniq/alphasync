/** @file       ASMessage.h
 *  @brief      Command message handling for Alphasmart devices.
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
#ifndef COM_TSONIQ_ASMessage_H
#define COM_TSONIQ_ASMessage_H    (1)

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

/* Example exchanges:

  -->  bfffa943 : 00000000 : 8  =   0b 00 00 00 00 00 00 0b   ........
 <--   bfffa943 : 00000000 : 8  =   87 00 05 8f 0c 00 24 4b   ......$K      Neo displays "Error: Not enough RAM for operation"

  -->  bfffa943 : 00000000 : 8  =   16 00 00 00 00 00 00 16   ........
 <--   bfffa943 : 00000000 : 8  =   8f 00 00 00 00 00 00 8f   ........

  -->  bfffa943 : 00000000 : 8  =   17 00 00 00 00 00 00 17   ........
 <--   bfffa943 : 00000000 : 8  =   8f 00 00 00 00 00 00 8f   ........

  -->  bfffa943 : 00000000 : 8  =   19 00 00 00 00 00 00 19   ........
 <--   bfffa943 : 00000000 : 8  =   57 00 00 00 00 00 00 57   W......W

  -->  bfffa943 : 00000000 : 8  =   1a 00 00 00 00 00 00 1a   ........      (with Neo running the Small ROM)
 <--   bfffa943 : 00000000 : 8  =   92 00 00 00 00 00 00 92   ........

  -->  bfffa943 : 00000000 : 8  =   1a 00 00 00 00 00 00 1a   ........      (with Neo running the standard ROM)
 <--   bfffa943 : 00000000 : 8  =   58 00 0e ac 34 05 8f da   X...4...

  -->  bfffa943 : 00000000 : 8  =   1b 00 00 00 00 00 00 1b   ........
 <--   bfffa943 : 00000000 : 8  =   90 00 00 00 00 00 00 90   ........

  -->  bfffa943 : 00000000 : 8  =   1b 00 00 00 00 a0 00 bb   ........
 <--   bfffa943 : 00000000 : 8  =   59 00 00 80 00 00 08 e1   Y.......

  -->  bfffa943 : 00000000 : 8  =   1b ff ff ff ff a0 00 b7   ........
 <--   bfffa943 : 00000000 : 8  =   59 00 00 10 00 00 08 71   Y......q

*/

#define ASMESSAGE_REQUEST_VERSION               (0x00)      /**< (len32, csum16): Obtain the OS version information. */
#define ASMESSAGE_REQUEST_01                    (0x01)      /**< Unknown (generates response 8f). */
#define ASMESSAGE_REQUEST_BLOCK_WRITE           (0x02)      /**< (len32, csum16): write a 1k or less block of data. */
#define ASMESSAGE_REQUEST_03                    (0x03)      /**< Unknown (generates response 0x8f). */
#define ASMESSAGE_REQUEST_LIST_APPLETS          (0x04)      /**< (first32, count16): read an array of applet headers. */
#define ASMESSAGE_REQUEST_WRITE_APPLET          (0x06)      /**< (len32, z16): write a new applet. */
#define ASMESSAGE_REQUEST_07                    (0x07)      /**< (z48): unknown - used when writing an applet. */
#define ASMESSAGE_REQUEST_RESTART               (0x08)      /**< (z48): causes the device to reset and restart as a HID device. */
#define ASMESSAGE_REQUEST_SET_BAUDRATE          (0x09)      /**< (baud32, z16). Try to set the specified baud rate. */
#define ASMESSAGE_REQUEST_0A                    (0x0a)      /**< Unknown - returns response 0x90 in tests & Neo displays nothing. */
#define ASMESSAGE_REQUEST_0B                    (0x0b)      /**< (z48): unknown - used when writing an applet. */
#define ASMESSAGE_REQUEST_GET_SETTINGS          (0x0c)      /**< (flags, applet16): read the specified file attributes. */
#define ASMESSAGE_REQUEST_SET_SETTINGS          (0x0d)      /**< (flags, applet16): write the specified file attributes. */
#define ASMESSAGE_REQUEST_SET_APPLET            (0x0e)      /**< (z32, applet16): used when setting applet properties. */
#define ASMESSAGE_REQUEST_READ_APPLET           (0x0f)      /**< (z32, applet16): used when reading an applet. */
#define ASMESSAGE_REQUEST_BLOCK_READ            (0x10)      /**< (z48): Request the next requested block of data from the device. */
#define ASMESSAGE_REQUEST_ERASE_APPLETS         (0x11)      /**< (z48): causes Neo to erase all smart applets - may take a very long time to return a reply. */
#define ASMESSAGE_REQUEST_READ_FILE             (0x12)      /**< (index32, applet16): used to read data from the specified file. */
#define ASMESSAGE_REQUEST_GET_FILE_ATTRIBUTES   (0x13)      /**< (index32, applet16): used to read the file attributes. */
#define ASMESSAGE_REQUEST_WRITE_FILE            (0x14)      /**< (index8, len24, applet16): request write of a file. */
#define ASMESSAGE_REQUEST_CONFIRM_WRITE_FILE    (0x15)      /**< (z48): used to complete writing of a file. */
#define ASMESSAGE_REQUEST_16                    (0x16)      /**< (z48?): unknown - used when adding an applet. */
#define ASMESSAGE_REQUEST_17                    (0x17)      /**< (z48?): unknown - used when adding an applet. */
#define ASMESSAGE_REQUEST_SMALL_ROM_UPDATER     (0x18)      /**< (z48?): used to enter the updater ROM when adding an applet. */
#define ASMESSAGE_REQUEST_19                    (0x19)      /**< Unknown - may be specific to AlphaHub devices? Generates response 0x57. */
#define ASMESSAGE_REQUEST_GET_AVAIL_SPACE       (0x1a)      /**< (z48): used to return the available space. */
#define ASMESSAGE_REQUEST_GET_USED_SPACE        (0x1b)      /**< (select32, applet16): used to obtain the file space used by an applet select32 is zero for the largest file, non-zero for all files. */
#define ASMESSAGE_REQUEST_READ_RAW_FILE         (0x1c)      /**< (index32, applet16): used to read a file in raw mode. */
#define ASMESSAGE_REQUEST_SET_FILE_ATTRIBUTES   (0x1d)      /**< (index32, applet16): used when setting file attributes. */
#define ASMESSAGE_REQUEST_COMMIT                (0x1e)      /**< (index32, applet16): used to commit changes following SET_FILE_ATTRIBUTES. */
#define ASMESSAGE_REQUEST_WRITE_RAW_FILE        (0x1f)      /**< (index8, len24, applet16): request write of a file. */

#define ASMESSAGE_RESPONSE_VERSION              (0x40)      /**< (len32, csum16): returns version information. */
#define ASMESSAGE_RESPONSE_41                   (0x41)      /**< Unknown. */
#define ASMESSAGE_RESPONSE_BLOCK_WRITE          (0x42)      /**< (z48): reply to block write request. */
#define ASMESSAGE_RESPONSE_BLOCK_WRITE_DONE     (0x43)      /**< (z43): reply to block write request. */
#define ASMESSAGE_RESPONSE_LIST_APPLETS         (0x44)      /**< (len32, csum16): returns array of applet headers. */
#define ASMESSAGE_RESPONSE_45                   (0x41)      /**< Unknown. */
#define ASMESSAGE_RESPONSE_WRITE_APPLET         (0x46)      /**< (z48?): sent in response to ASMESSAGE_REQUEST_WRITE_APPLET */
#define ASMESSAGE_RESPONSE_47                   (0x47)      /**< (z48?): unknown: sent in response to ASMESSAGE_REQUEST_0B - possibly an ok to proceed check? */
#define ASMESSAGE_RESPONSE_48                   (0x48)      /**< (z48?): unknown: sent in response to ASMESSAGE_REQUEST_07 */
#define ASMESSAGE_RESPONSE_49                   (0x49)      /**< Unknown. */
#define ASMESSAGE_RESPONSE_SET_BAUDRATE         (0x4a)      /**< (baud32, z16): response to ASMESSAGE_REQUEST_BAUDRATE. */
#define ASMESSAGE_RESPONSE_GET_SETTINGS         (0x4b)      /**< (len32, csum16): returns file attribute data. */
#define ASMESSAGE_RESPONSE_SET_APPLET           (0x4c)      /**< (z48?): reply to ASMESSAGE_REQUEST_SET_APPLET. */
#define ASMESSAGE_RESPONSE_BLOCK_READ           (0x4d)      /**< (len32, csum16): reply to  ASMESSAGE_REQUEST_BLOCK_READ. */
#define ASMESSAGE_RESPONSE_BLOCK_READ_EMPTY     (0x4e)      /**< ? */
#define ASMESSAGE_RESPONSE_4F                   (0x4f)      /**< (z48?): reply to ASMESSAGE_REQUEST_ERASE_APPLETS. */
#define ASMESSAGE_RESPONSE_WRITE_FILE           (0x50)      /**< (z48): */
#define ASMESSAGE_RESPONSE_CONFIRM_WRITE_FILE   (0x51)      /**< (z48): */
#define ASMESSAGE_RESPONSE_RESTART              (0x52)      /**< (z48): */
#define ASMESSAGE_RESPONSE_READ_FILE            (0x53)      /**< (length32, ?16): */
#define ASMESSAGE_RESPONSE_CCC                  (0x54)      /**< (z48?): send in response to ASMESSAGE_REQUEST_16. */
#define ASMESSAGE_RESPONSE_DDD                  (0x55)      /**< (z48?): send in response to ASMESSAGE_REQUEST_17. */
#define ASMESSAGE_RESPONSE_SMALL_ROM_UPDATER    (0x56)      /**< (z48): reply to ASMESSAGE_REQUEST_SMALL_ROM_UPDATER, indicating using small ROM. */
#define ASMESSAGE_RESPONSE_57                   (0x57)      /**< Unknown. Sent in response to 0x19. */
#define ASMESSAGE_RESPONSE_GET_AVAIL_SPACE      (0x58)      /**< (flash32, ram16): reply to ASMESSAGE_REQUEST_GET_AVAIL_SPACE. ram size should be multiplied by 256. */
#define ASMESSAGE_RESPONSE_GET_USED_SPACE       (0x59)      /**< (ram32, files16): returns the number of bytes of RAM and the number of files used by an applet. */
#define ASMESSAGE_RESPONSE_GET_FILE_ATTRIBUTES  (0x5a)
#define ASMESSAGE_RESPONSE_SET_FILE_ATTRIBUTES  (0x5b)
#define ASMESSAGE_RESPONSE_COMMIT               (0x5c)

#define ASMESSAGE_ERROR_INVALID_BAUDRATE        (0x86)      /**< (z48): Sent if a bad BAUD rate is given. */
#define ASMESSAGE_ERROR_87                      (0x87)      /**< (unknown): Unknown (seen in response to a bogus cmd 0x0b). */
#define ASMESSAGE_ERROR_INVALID_APPLET          (0x8a)      /**< (z48): Specified Applet ID is not recognised. */
#define ASMESSAGE_ERROR_PROTOCOL                (0x8f)      /**< (z48): Sent in response to command block checksum errors or invalid command codes. */
#define ASMESSAGE_ERROR_PARAMETER               (0x90)      /**< (error32, z16): appears to return an error number (usually negative). */
#define ASMESSAGE_ERROR_OUTOFMEMORY             (0x91)      /**< May be seen if trying to write too large a file. */
#define ASMESSAGE_ERROR_94                      (0x94)      /**< Seen in response to sending command code 0x20 */


namespace ts
{

    /** Generalised command packet handling.
     *
     *  Commands have the following general format:
     *
     *      Byte  0     Command byte
     *      Bytes 1-6   Command specific data
     *      Byte  7     Sum of bytes 0-6 modulo 8 bits.
     *
     *  The message data may contain big-endian numeric values of vary sizes:
     *
     *  @verbatim
     *      <length32> <applet16>           Most common format (typically a length and applet code)
     *      <length24> <file8> <applet16>   Used for operations that select an applet.
     *      <size24> <size24>               Used to return memory sizes.
     *  @endverbatim
     *
     *  It is possible that other forms may be used that are not known to this code. As such,
     *  this class has to provide generalised set/get methods for the payload that are largely
     *  independent of the specific command code.
     */
    class ASMessage
    {
    public:

        /** Default constructor. Implicitly initialises all fields in the message to zero except
         *  for the command code.
         *
         *  @param  code        The command code for the message. Default ASMESSAGE_ERROR_PROTOCOL.
         */
        ASMessage(unsigned code=ASMESSAGE_ERROR_PROTOCOL)
        {
            init(code);
        }


        /** Initialise the command block. All arguments are cleared except for the (specified) command code.
         *
         *  @param  code        The command code for the message.
         */
        void init(unsigned code)
        {
            for (unsigned int i = 1; i < sizeof m_data / sizeof m_data[0]; i++) m_data[i] = 0;
            setCommand(code);
        }


        /** Test the checksum.
         *
         *  @return             Logical true if the checksum is ok, false otherwise.
         */
        bool valid() const
        {
            unsigned sum = 0;
            for (unsigned i = 0; i < 7; i++)  sum += m_data[i];
            sum = sum & 0xff;
            return sum == m_data[7];
        }


        /** Return the command code.
         *
         *  @param  code        The command code.
         */
        unsigned command() const
        {
            return (unsigned) m_data[0];
        }


        /** Set command code.
         *
         *  @param  command     The command block.
         *  @param  code        The command code.
         */
        void setCommand(unsigned code)
        {
            m_data[0] = code & 0xff;
            setChecksum();
        }


        /** Get a value from the command block.
         *
         *  @param  command     The command block.
         *  @param  offset      The byte offset in to the command block.
         *  @param  width       The width of the field (bytes).
         *  @return             The value
         */
        unsigned argument(int offset, int width) const
        {
            assert(width >= 1 && width <= 4);
            assert(offset >= 1 && offset + width <= 7);

            unsigned value = 0;
            for (int i = 0; i < width; i++)
            {
                value = value << 8;
                value = value | (((unsigned) m_data[offset+i]) & 0xff);
            }

            return value;
        }



        /** Set an value in to the command block.
         *
         *  @param  value       The value to set.
         *  @param  offset      The byte offset in to the command block.
         *  @param  width       The width of the field (bytes).
         */
        void setArgument(unsigned value, int offset, int width)
        {
            assert(width >= 1 && width <= 4);
            assert(offset >= 1 && offset + width <= 7);
            for (int i = width - 1; i >= 0; i--)
            {
                m_data[offset+i] = value & 0xff;
                value = value >> 8;
            }
            setChecksum();
        }


        /** Return the raw data buffer pointer.
         */
        const void *rawData() const
        {
            return &m_data[0];
        }


        /** Return the raw data buffer pointer.
         */
        void *rawData()
        {
            return &m_data[0];
        }


        /** Return the raw data buffer size.
         */
        unsigned rawSize() const
        {
            return sizeof m_data;
        }


        /** Print the message data.
         */
        void dump(FILE* fh) const
        {
            for (unsigned i = 0; i < 8; i++)
            {
                if (0 == i) fprintf(fh, "[");
                else fprintf(fh, ", ");
                fprintf(fh, "%02x", m_data[i]);
            }
            fprintf(fh, "]");
        }




    private:

        uint8_t m_data[8];


        /** Set a valid checksum.
         */
        void setChecksum()
        {
            unsigned sum = 0;
            for (unsigned i = 0; i < 7; i++)  sum += m_data[i];
            m_data[7] = sum;
        }

    };



};  // namespace


#endif      // COM_TSONIQ_ASMessage_H
