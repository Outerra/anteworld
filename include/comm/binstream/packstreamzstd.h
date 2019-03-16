#pragma once

/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is COID/comm module.
*
* The Initial Developer of the Original Code is
* PosAm.
* Portions created by the Initial Developer are Copyright (C) 2017
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
* Brano Kemen
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the MPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the MPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

#ifndef __COID_COMM_PACKSTREAMZSTD__HEADER_FILE__
#define __COID_COMM_PACKSTREAMZSTD__HEADER_FILE__

#include "../namespace.h"

#include "packstream.h"
#include "../coder/bufpack_zstd.h"

COID_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
///
class packstreamzstd : public packstream
{
public:
    virtual ~packstreamzstd()
    {
        close();
    }

    virtual uint binstream_attributes(bool in0out1) const
    {
        return 0;
    }

    virtual opcd peek_read(uint timeout) {
        if (timeout)  return ersINVALID_PARAMS;

        uints n = 0;
        return read_raw(0, n);
    }

    virtual opcd peek_write(uint timeout) { return 0; }

    virtual bool is_open() const { return _in->is_open(); }
    virtual void flush() {
        packed_flush();
        _out->flush();
    }
    virtual void acknowledge(bool eat = false) {
        packed_ack(eat);
        _in->acknowledge(eat);
    }

    virtual opcd close(bool linger = false)
    {
        if (_in)
            packed_ack(true);
        if (_out)
            packed_flush();
        return 0;
    }


    packstreamzstd()
    {}

    packstreamzstd(binstream* bin, binstream* bout) : packstream(bin, bout)
    {}

    packstreamzstd(binstream& bin) : packstream(bin)
    {}

    ///
    virtual opcd write_raw(const void* p, uints& len)
    {
        if (len == 0)
            return 0;

        ints size = _zstd.pack_stream(p, len, *_out);
        if (size < 0)
            return ersFAILED;
        len -= size;
        
        return 0;
    }

    ///
    virtual opcd read_raw(void* p, uints& len)
    {
        if (len == 0)
            return 0;

        ints size = _zstd.unpack_stream(*_in, p, len);
        if (size < 0)
            return ersFAILED;
        len -= size;

        return 0;
    }


    virtual void reset_read()
    {
        _zstd.reset_read();
    }

    virtual void reset_write()
    {
        _zstd.reset_write();
    }

protected:
    void packed_flush()
    {
        _zstd.pack_stream(0, 0, *_out);
        _zstd.reset_write();
    }

    void packed_ack(bool eat)
    {
        if (!eat && !_zstd.eof())
            throw ersIO_ERROR "data left in input buffer";

        _zstd.reset_read();
    }

    packer_zstd _zstd;
};

COID_NAMESPACE_END

#endif //__COID_COMM_packstreamzstd__HEADER_FILE__

