/** @file   ASApplet.cc
 *  @brief  Applet information and manipulation.
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

#include <ctype.h>
#include <assert.h>
#include "ASApplet.h"

namespace ts
{
    /** Constant block of zeros used as a default TLV data area. This mimics the
     *  end of TLV list marker in an applet.
     */
    const uint8_t ASAppletTLV::asAppletTLVNullData[6] = { 0, 0, 0, 0, 0, 0 };


    /** Class constructor.
     */
    ASApplet::ASApplet()
        :
        m_appletHeaderLoaded(false),
        m_appletDataLoaded(false),
        m_appletData(0),
        m_appletSize(0),
        m_appletHeaderOffset(0),
        m_appletSignature(0),
        m_appletRomSize(0),
        m_appletRamSize(0),
        m_appletSettingsOffset(0),
        m_appletFlags(0),
        m_appletAppletID(0),
        m_appletHeaderVersion(0),
        m_appletFileCount(0),
        m_appletVersionMajor(0),
        m_appletVersionMinor(0),
        m_appletVersionRevision(0),
        m_appletLanguageID(0),
        m_appletMinASMVersion(0),
        m_appletFileSpace(0),
        m_appletName(),
        m_appletInfo()
    {
        memset(m_appletName, 0, sizeof m_appletName);
        memset(m_appletInfo, 0, sizeof m_appletInfo);
    }


    /** Copy constructor operator.
     *
     *  @param  applet      The applet to copy.
     *  @return             A reference to this.
     */
    ASApplet::ASApplet(const ASApplet& applet)
        :
        m_appletHeaderLoaded(false),
        m_appletDataLoaded(false),
        m_appletData(0),
        m_appletSize(0),
        m_appletHeaderOffset(0),
        m_appletSignature(0),
        m_appletRomSize(0),
        m_appletRamSize(0),
        m_appletSettingsOffset(0),
        m_appletFlags(0),
        m_appletAppletID(0),
        m_appletHeaderVersion(0),
        m_appletFileCount(0),
        m_appletVersionMajor(0),
        m_appletVersionMinor(0),
        m_appletVersionRevision(0),
        m_appletLanguageID(0),
        m_appletMinASMVersion(0),
        m_appletFileSpace(0),
        m_appletName(),
        m_appletInfo()
    {
        memset(m_appletName, 0, sizeof m_appletName);
        memset(m_appletInfo, 0, sizeof m_appletInfo);

        *this = applet;
    }


    /** Destructor.
     */
    ASApplet::~ASApplet()
    {
        unload();
    }


    /** Assignment operator.
     *
     *  @param  applet      The applet to copy.
     *  @return             A reference to this.
     */
    ASApplet& ASApplet::operator=(const ASApplet& applet)
    {
        m_appletHeaderLoaded = applet.m_appletHeaderLoaded;
        m_appletDataLoaded = applet.m_appletDataLoaded;
        m_appletData = 0;
        m_appletSize = 0;
        m_appletHeaderOffset = applet.m_appletHeaderOffset;
        m_appletSignature = applet.m_appletSignature;
        m_appletRomSize = applet.m_appletRomSize;
        m_appletRamSize = applet.m_appletRamSize;
        m_appletSettingsOffset = applet.m_appletSettingsOffset;
        m_appletFlags = applet.m_appletFlags;
        m_appletAppletID = applet.m_appletAppletID;
        m_appletHeaderVersion = applet.m_appletHeaderVersion;
        m_appletFileCount = applet.m_appletFileCount;
        m_appletVersionMajor = applet.m_appletVersionMajor;
        m_appletVersionMinor = applet.m_appletVersionMinor;
        m_appletVersionRevision = applet.m_appletVersionRevision;
        m_appletLanguageID = applet.m_appletLanguageID;
        m_appletMinASMVersion = applet.m_appletMinASMVersion;
        m_appletFileSpace = applet.m_appletFileSpace;

        memcpy(m_appletName, applet.m_appletName, sizeof m_appletName);
        memcpy(m_appletInfo, applet.m_appletInfo, sizeof m_appletInfo);

        if (0 == applet.m_appletData || 0 == applet.m_appletSize)
        {
            m_appletData = 0;
            m_appletSize = 0;
        }
        else
        {
            m_appletData = (uint8_t*)malloc(applet.m_appletSize);
            if (m_appletData)
            {
                m_appletSize = applet.m_appletSize;
                memcpy(m_appletData, applet.m_appletData, m_appletSize);
            }
        }
        return *this;
    }


    /** Unload any applet data.
     */
    void ASApplet::unload()
    {
        if (m_appletData) free(m_appletData);
        m_appletData = 0;
        m_appletHeaderLoaded = false;
        m_appletDataLoaded = false;
    }



    /** Load applet data given a raw data block and a byte offset to the header.
     *
     *  @param  data        The address of the data.
     *  @param  size        The number of bytes of data present.
     *  @param  offset      The byte offset of the header.
     *  @return             Logical true if the data could be loaded, or false if not.
     */
    bool ASApplet::loadAppletWithHeaderOffset(const uint8_t* data, unsigned size, unsigned offset)
    {
        if (offset + kASAppletHeaderSize > size) return false;  // data is too small to contain a complete header

        m_appletData = (uint8_t*)malloc(size);
        if (!m_appletData) return false;

        memcpy(m_appletData, data, size);
        m_appletSize = size;
        m_appletHeaderOffset = offset;

        if (!loadHeader(&m_appletData[offset]))
        {
            unload();
            return false;
        }

        return true;
    }


    /** Load header data.
     *
     *  @param  header          A pointer to the header information (at least kASAppletHeaderSize bytes long).
     *  @return                 Logical true if the data is plausible.
     */
    bool ASApplet::loadHeader(const uint8_t* header)
    {
        m_appletSignature       =   as_EndianReadU32(&header[kASAppletHeaderOffsetSignature]);
        m_appletRomSize         =   as_EndianReadU32(&header[kASAppletHeaderOffsetRomSize]);
        m_appletRamSize         =   as_EndianReadU32(&header[kASAppletHeaderOffsetRamSize]);
        m_appletSettingsOffset  =   as_EndianReadU32(&header[kASAppletHeaderOffsetSettingsOffset]);
        m_appletFlags           =   as_EndianReadU32(&header[kASAppletHeaderOffsetFlags]);
        m_appletAppletID        =   as_EndianReadU16(&header[kASAppletHeaderOffsetAppletID]);
        m_appletHeaderVersion   =   as_EndianReadU8(&header[kASAppletHeaderOffsetHeaderVersion]);
        m_appletFileCount       =   as_EndianReadU8(&header[kASAppletHeaderOffsetFileCount]);
        m_appletVersionMajor    =   as_EndianReadU8(&header[kASAppletHeaderOffsetVersionMajor]);
        m_appletVersionMinor    =   as_EndianReadU8(&header[kASAppletHeaderOffsetVersionMinor]);
        m_appletVersionRevision =   as_EndianReadU8(&header[kASAppletHeaderOffsetVersionRevision]);
        m_appletLanguageID      =   as_EndianReadU8(&header[kASAppletHeaderOffsetLanguageID]);
        m_appletMinASMVersion   =   as_EndianReadU32(&header[kASAppletHeaderOffsetMinASMVersion]);
        m_appletFileSpace       =   as_EndianReadU32(&header[kASAppletHeaderOffsetFileSpace]);

        memcpy(m_appletName, &header[kASAppletHeaderOffsetName], kASAppletNameLength);
        m_appletName[kASAppletNameLength] = 0;

        memcpy(m_appletInfo, &header[kASAppletHeaderOffsetInfo], kASAppletInfoLength);
        m_appletInfo[kASAppletInfoLength] = 0;

        m_appletHeaderLoaded = (kASAppletSignature == m_appletSignature);       /* Validate by checking applet signature. */
        return m_appletHeaderLoaded;
    }


    /** Load applet data.
     *
     *  @param  data            A pointer to the applet data.
     *  @param  size            The size of the data, in bytes.
     *  @return                 Logical true if the data could be loaded.
     */
    bool ASApplet::loadApplet(const uint8_t* data, unsigned size)
    {
        unload();
        return loadAppletWithHeaderOffset(data, size, 0);
    }



    /** Load OS applet data.
     *
     *  @param  data            A pointer to the applet data.
     *  @param  size            The size of the data, in bytes.
     *  @return                 Logical true if the data could be loaded.
     */
    bool ASApplet::loadOS(const uint8_t* data, unsigned size)
    {
        unload();
        if (size < 0x4c) return false;
        unsigned offset = as_EndianReadU32(&data[0x48]);
        fprintf(stderr, "*** size %u, offset %u ***\n", size, offset);
        return loadAppletWithHeaderOffset(data, size, offset);
    }


    /** Return the lanuage ID as a printable ASCII name string.
     */
    const char* ASApplet::appletLanguageName() const
    {
        const char* result;

        switch (appletLanguageID())
        {
            case 1:     result = "English (US)";        break;
            case 2:     result = "English (UK)";        break;
            case 3:     result = "French";              break;
            case 4:     result = "French (CR)";         break;
            case 5:     result = "Italian";             break;
            case 6:     result = "German";              break;
            case 7:     result = "Spanish";             break;
            case 8:     result = "Dutch";               break;
            case 9:     result = "Swedish";             break;
            default:    result = "<unknown>";           break;
        }

        return result;
    }


    /** Obtain the first TLV object in the applet.
     *
     *  @param  tlv     TLV object to initialise.
     *  @return         Logical true if the TLV was initialised, false if there are no TLV items present.
     *                  Note that this method may return false if the applet has potential TLV values but
     *                  the applet data has not yet been loaded.
     */
    bool ASApplet::firstTLV(ASAppletTLV* tlv) const
    {
        if (!hasSettings() || !areSettingsLoaded())
        {
            tlv->setNull();
            return false;
        }
        else
        {
            tlv->setAddress(&m_appletData[appletSettingsOffset()]);
            return true;
        }
    }


    /** Obtain the next TLV object in the applet.
     *
     *  @param  tlv     TLV object to initialise. This should have been previously used successfully
     *                  in a call to firstTLV() or nextTLV().
     *  @return         Logical true if the TLV was initialised, false if there are no more items present.
     */
    bool ASApplet::nextTLV(ASAppletTLV* tlv) const
    {
        if (0 == tlv->type())
        {
            return false;
        }
        else
        {
            unsigned length = tlv->length();
            length += (length & 1); // round to next 16 bit boundary

            const uint8_t* nextAddress = tlv->address() + length + 6;

            tlv->setAddress(nextAddress);
            if (0 == tlv->type())
            {
                tlv->setNull();
                return false;
            }
            else
            {
                tlv->setAddress(nextAddress);
                return true;
            }
        }
    }




    /** Display the applet data.
     */
    void ASApplet::dump(FILE* fh) const
    {
        fprintf(fh, "Applet header:\n");
        fprintf(fh, "  Signature:        %08x\n", appletSignature());
        fprintf(fh, "  ROM Size:         %08x  (%4.1lfk)\n", appletRomSize(), double(appletRomSize()) / 1024.0);
        fprintf(fh, "  RAM Size:         %08x  (%4.1lfk)\n", appletRamSize(), double(appletRamSize()) / 1024.0);
        fprintf(fh, "  Settings offset:  %08x\n", appletSettingsOffset());
        fprintf(fh, "  Flags:            %08x\n", appletFlags());
        fprintf(fh, "  AppletID:             %04x\n", appletID());
        fprintf(fh, "  HeaderVersion:          %02x  (%d)\n", appletHeaderVersion(), appletHeaderVersion());
        fprintf(fh, "  File count:             %02x  (%d)\n", appletFileCount(), appletFileCount());
        fprintf(fh, "  Name:                   %s\n", appletName());
        fprintf(fh, "  Version (Major):        %02x  (%d)\n", appletVersionMajor(), appletVersionMajor());
        fprintf(fh, "  Version (Minor):        %02x  (%d)\n", appletVersionMinor(), appletVersionMinor());
        fprintf(fh, "  Version (Revision):     %02x  (%c)\n", appletVersionRevision(), appletVersionRevision());
        fprintf(fh, "  Language ID:            %02x  (%s)\n", appletLanguageID(), appletLanguageName());
        fprintf(fh, "  Info:                   %s\n", appletInfo());
        fprintf(fh, "  Min ASM Version:  %08x\n", appletMinASMVersion());
        fprintf(fh, "  File Space:       %08x  (%4.1lfk)\n", appletFileSpace(), double(appletFileSpace()) / 1024.0);
        fprintf(fh, "  Total RAM requirement is %5.1lfk bytes\n", double(appletFileSpace() + appletRamSize()) / 1024.0);
        if (!hasSettings())
        {
            fprintf(fh, "  No TLVs present\n");
        }
        else if (!areSettingsLoaded())
        {
            fprintf(fh, "  TLVs present but not loaded.\n");
        }
        else
        {
            fprintf(fh, "  TLVs:\n");

            ASAppletTLV tlv;
            bool ok = firstTLV(&tlv);
            while (ok)
            {
                fprintf(fh, "  Type %04x  Ident %04x  Length %04x  Value:", tlv.type(), tlv.ident(), tlv.length());
                for (unsigned i = 0; i < tlv.length(); i++) fprintf(fh, " %02x", tlv.value()[i]);
                fprintf(fh, "  ");
                for (unsigned i = 0; i < tlv.length(); i++)
                {
                    uint8_t byte = tlv.value()[i];
                    fprintf(fh, "%c", isprint(byte) ? (char)byte : '.');
                }
                fprintf(fh, "\n");
                ok = nextTLV(&tlv);
            }
        }
    }


    /** Interpret a block of bytes as settings data and display.
     */
    void ASApplet::dumpSettingsBuffer(FILE* fh, const uint8_t* buffer, unsigned size) const
    {
        fprintf(fh, "TLV buffer for applet ID %04x:\n", (unsigned)appletID());

        const uint8_t* ptr = buffer;
        while (ptr < buffer + size)
        {
            ASAppletTLV tlv;
            tlv.setAddress(ptr);

            fprintf(fh, "  Type %04x  Ident %04x  Length %04x  Value:", tlv.type(), tlv.ident(), tlv.length());
            for (unsigned i = 0; i < tlv.length(); i++) fprintf(fh, " %02x", tlv.value()[i]);
            fprintf(fh, "  ");
            for (unsigned i = 0; i < tlv.length(); i++)
            {
                uint8_t byte = tlv.value()[i];
                fprintf(fh, "%c", isprint(byte) ? (char)byte : '.');
            }
            fprintf(fh, "\n");

            unsigned length = tlv.length();
            length += (length & 1); // round to next 16 bit boundary
            ptr += (length + 6);
        }
    }

}   // namespace
