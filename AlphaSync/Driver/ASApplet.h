/** @file   ASApplet.h
 *  @brief  Applet information and manipulation.
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

#ifndef COM_TSONIQ_ASApplet_H
#define COM_TSONIQ_ASApplet_H   (1)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ASEndian.h"

namespace ts
{
    /* Applet field offsets. These are offsets in to the applet header data (usually located at the start
     * of the applet).
     */
    #define kASAppletHeaderOffsetSignature          (0x00)      /**< Byte offset of the signature word field. */
    #define kASAppletHeaderOffsetRomSize            (0x04)      /**< Byte offset of the ROM size field. */
    #define kASAppletHeaderOffsetRamSize            (0x08)      /**< Byte offset of the size of working RAM field. */
    #define kASAppletHeaderOffsetSettingsOffset     (0x0c)      /**< Byte offset settings parameters field. */
    #define kASAppletHeaderOffsetFlags              (0x10)      /**< Byte offset of the flags field. */
    #define kASAppletHeaderOffsetAppletID           (0x14)      /**< Byte offset of the applet ID field. */
    #define kASAppletHeaderOffsetHeaderVersion      (0x16)      /**< Byte offset of the Header version code field. */
    #define kASAppletHeaderOffsetFileCount          (0x17)      /**< Byte offset of the  file count field. */
    #define kASAppletHeaderOffsetName               (0x18)      /**< Byte offset of the display name. */
    #define kASAppletHeaderOffsetVersionMajor       (0x3c)      /**< Byte offset of the Major version number field. */
    #define kASAppletHeaderOffsetVersionMinor       (0x3d)      /**< Minor version number field. */
    #define kASAppletHeaderOffsetVersionRevision    (0x3e)      /**< Revision code (ASCII) field. */
    #define kASAppletHeaderOffsetLanguageID         (0x3f)      /**< Localised language field. */
    #define kASAppletHeaderOffsetInfo               (0x40)      /**< The info (copyright) string field. */
    #define kASAppletHeaderOffsetMinASMVersion      (0x7c)      /**< Minimum AlphaSmart Manager version required field. */
    #define kASAppletHeaderOffsetFileSpace          (0x80)      /**< The required file space field. */

    #define kASAppletNameLength                     (36)        /**< Number of characters in the applet name string. */
    #define kASAppletInfoLength                     (60)        /**< Number of characters in the info string. */
    #define kASAppletASMVersionLength               (4)         /**< Number of bytes in the ASM version code. */

    #define kASAppletHeaderSize                     (0x84)      /**< The total size of the header. */


    #define kASAppletSignature                  (0xc0ffeead)    /**< The expected value of the signature word. */


    /* Known applet flags:
     *
     *  AlphaWord:      0xff0000ce      1100.1110
     *  KAZ:            0xff000000      0000.0000
     *  Calculator:     0xff000000      0000.0000
     *  Beamer:         0xff000000      0000.0000
     *  Control Panel:  0xff000080      1000.0000
     *  Spell Check:    0xff000001      0000.0001
     *  Thesaurus:      0xff000001      0000.0001
     *  Font files:     0xff000031      0011.0001
     *  System:         0xff000011      0001.0001
     */
    #define kASAppletFlagsHidden    (0x01)          /**< If set, the applet is hidden. */



    /** Applet TLV. This class is used to represent and update TLV values.
     */
    class ASAppletTLV
    {
    public:

        ASAppletTLV() : m_data(&asAppletTLVNullData[0]) { }

        unsigned type() const           { return as_EndianReadU16(&m_data[0]); }
        unsigned ident() const          { return as_EndianReadU16(&m_data[2]); }
        unsigned length() const         { return as_EndianReadU16(&m_data[4]); }
        const uint8_t* value() const    { return &m_data[6]; }

    private:

        const uint8_t* m_data;
        static const uint8_t asAppletTLVNullData[6];

        friend class ASApplet;

        void setAddress(const uint8_t* addr) { m_data = addr; }
        void setNull() { m_data = &asAppletTLVNullData[0]; }
        const uint8_t* address() const { return m_data; }
    };






    /** Class used to process applet data.
     */
    class ASApplet
    {
    public:

        ASApplet();
        ASApplet(const ASApplet& applet);
        ~ASApplet();

        ASApplet& operator=(const ASApplet& applet);

        bool loadHeader(const uint8_t* header);
        bool loadApplet(const uint8_t* data, unsigned size);
        bool loadOS(const uint8_t* data, unsigned size);

        bool isAppletLoaded() const             { return (0 != m_appletData); }
        bool hasSettings() const                { return (0 != appletSettingsOffset()); }
        bool areSettingsLoaded() const          { return isAppletLoaded(); }

        uint32_t appletSignature() const        { return m_appletSignature; }
        uint32_t appletRomSize() const          { return m_appletRomSize; }
        uint32_t appletRamSize() const          { return m_appletRamSize; }
        uint32_t appletSettingsOffset() const   { return m_appletSettingsOffset; }
        uint32_t appletFlags() const            { return m_appletFlags; }
        uint16_t appletID() const               { return m_appletAppletID; }
        uint8_t appletHeaderVersion() const     { return m_appletHeaderVersion; }
        uint8_t appletFileCount() const         { return m_appletFileCount; }
        uint8_t appletVersionMajor() const      { return m_appletVersionMajor; }
        uint8_t appletVersionMinor() const      { return m_appletVersionMinor; }
        uint8_t appletVersionRevision() const   { return m_appletVersionRevision; }
        uint8_t appletLanguageID() const        { return m_appletLanguageID; }
        uint32_t appletMinASMVersion() const    { return m_appletMinASMVersion; }
        uint32_t appletFileSpace() const        { return m_appletFileSpace; }
        const char* appletName() const          { return m_appletName; }
        const char* appletInfo() const          { return m_appletInfo; }
        const char* appletLanguageName() const;

        bool firstTLV(ASAppletTLV*) const;
        bool nextTLV(ASAppletTLV*) const;

        void dump(FILE* fh) const;
        void dumpSettingsBuffer(FILE* fh, const uint8_t* buffer, unsigned size) const;

    protected:

        void unload();

    private:

        /* Control data */
        bool m_appletHeaderLoaded;
        bool m_appletDataLoaded;
        uint8_t *m_appletData;
        unsigned m_appletSize;
        unsigned m_appletHeaderOffset;

        /* Decoded header fields */
        uint32_t m_appletSignature;
        uint32_t m_appletRomSize;
        uint32_t m_appletRamSize;
        uint32_t m_appletSettingsOffset;
        uint32_t m_appletFlags;
        uint16_t m_appletAppletID;
        uint8_t m_appletHeaderVersion;
        uint8_t m_appletFileCount;
        uint8_t m_appletVersionMajor;
        uint8_t m_appletVersionMinor;
        uint8_t m_appletVersionRevision;
        uint8_t m_appletLanguageID;
        uint32_t m_appletMinASMVersion;
        uint32_t m_appletFileSpace;
        char m_appletName[kASAppletNameLength + 1];
        char m_appletInfo[kASAppletInfoLength + 1];

        bool loadAppletWithHeaderOffset(const uint8_t* data, unsigned size, unsigned offset);
    };


}   // namespace

#endif      // COM_TSONIQ_ASApplet_H
