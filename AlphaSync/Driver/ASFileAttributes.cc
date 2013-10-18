/** @file       ASFileAttributes.cc
 *  @brief      File information.
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
#include <stdio.h>
#include <string.h>
#include "ASFileAttributes.h"


namespace ts
{
    /** List of file space codes.
     *  REVIEW: this is presumably hard wired (since otherwise a backup and restore following an OS update
     *  would corrupt backup data unless it were changed). However, the numbers used seem to make little
     *  sense. Using values other than in this table will generally upset the Neo...
     */
    static const unsigned fileSpaceCodes[9] = { 0xff, 0x2d, 0x2c, 0x04, 0x0f, 0x0e, 0x0a, 0x01, 0x27 };



    /** Constructor.
     */
    ASFileAttributes::ASFileAttributes()
    {
        clear();
    }


    /** Constructor, passed a block of raw attribute data that will be copied.
     */
    ASFileAttributes::ASFileAttributes(const uint8_t buffer[kASFileAttributesSize])
    {
        clear();
        memcpy(m_raw, buffer, kASFileAttributesSize);
    }


    /** Copy constructor.
     */
    ASFileAttributes::ASFileAttributes(const ASFileAttributes& other)
    {
        copyFrom(&other);
    }


    /** Return the filename.
     */
    const char* ASFileAttributes::fileName() const
    {
        return (const char*)&m_raw[0];
    }


    /** Return the password.
     */
    const char* ASFileAttributes::password() const
    {
        return (const char*)&m_raw[16];
    }


    /** Set the filename.
     *
     *  @param  name        The name to use.
     *  @return             Logical true if the name was valid. If false is returned, the
     *                      name may be truncated or have had characters replaced.
     */
    bool ASFileAttributes::setFileName(const char* name)
    {
        bool ok = true;
        unsigned len = strlen(name);
        if (len > 15)
        {
            len = 15;
            ok = false;
        }
        memset(&m_raw[0], 0, 16);
        memcpy(&m_raw[0], name, len);
        return ok;
    }


    /** Set the file password.
     *
     *  @param  pass        The password to use.
     *  @return             Logical true if the password was valid. If false is returned, the
     *                      password may be truncated or have had characters replaced.
     */
    bool ASFileAttributes::setPassword(const char* pass)
    {
        bool ok = true;
        unsigned len = strlen(pass);
        if (len > 6)
        {
            len = 6;
            ok = false;
        }
        memset(&m_raw[16], 0, 8);
        memcpy(&m_raw[16], pass, len);
        return ok;
    }


    /** Set the file space.
     *
     *  @param  space   The space number. Use 0 for unbound, or 1-8 for file spaces 1 to 8 respectively.
     */
    void ASFileAttributes::setFileSpace(unsigned space)
    {
        if (space > 8) space = 0;   // Use unbound if specified space is out of range.
        m_raw[0x25] = fileSpaceCodes[space];
    }


    /** Return the filespace.
     *
     *  @return         The file space number. Zero => unbound, 1 to 8 => file spaces 1 to 8 respectively.
     */
    unsigned ASFileAttributes::fileSpace() const
    {
        unsigned code = m_raw[0x25];
        for (unsigned i = 0; i <= 8; i++)
        {
            if (fileSpaceCodes[i] == code) return i;    // matched the space code
        }

        // If no match, log the problem and pretend the space is unmapped.
        fprintf(stderr, "ASFileAttributes::fileSpace():  unrecognised file space code %02x\n", code);
        dump(stderr);
        return 0;
    }


    /** Display the raw data.
     */
    void ASFileAttributes::dump(FILE* fh) const
    {
        fprintf(fh, "%-16s  %-8s   %08x %08x [", fileName(), password(), minSize(), allocSize());
        for (unsigned i = 0x20; i < sizeof m_raw; i++)
        {
            if (0 == (i % 2)) fprintf(fh, " ");
            fprintf(fh, "%02x", m_raw[i]);
        }
        fprintf(fh, " ]\n");
    }


    /** Clear the file info.
     */
    void ASFileAttributes::clear()
    {
        memset(this->m_raw, 0, sizeof this->m_raw);
        setFileName("filename");
        setPassword("password");
        setMinSize(512);
        setAllocSize(512);
        setFlags(0);
        setFileSpace(0);
    }


    /** Copy the attributes from another object.
     */
    void ASFileAttributes::copyFrom(const ASFileAttributes* other)
    {
        memcpy(this->m_raw, other->m_raw, sizeof m_raw);
    }


    /** Copy the attributes from a byte buffer.
     */
    void ASFileAttributes::copyFrom(const uint8_t buffer[kASFileAttributesSize])
    {
        memcpy(this->m_raw, buffer, kASFileAttributesSize);
    }

};  // namespace
