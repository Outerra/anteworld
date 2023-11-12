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
 * Brano Kemen
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "memtrack.h"
#include "../hash/hashkeyset.h"
#include "../singleton.h"
#include "../atomic/atomic.h"
#include "../sync/mutex.h"

#include "../binstream/filestream.h"

#ifdef _DEBUG
static const bool default_enabled = true;
#else
static const bool default_enabled = false;
#endif


namespace coid {


static void name_filter(charstr& dst, token name)
{
    //remove struct, class
    static const token _struct = "struct";
    static const token _class = "class";
    static const token _array = "[0]";

    do {
        token x = name.cut_left(' ');

        if (x.ends_with(_struct))
            x.shift_end(-int(_struct.len()));
        else if (x.ends_with(_class))
            x.shift_end(-int(_class.len()));
        else if (x == _array) {
            dst.append("[]");
            x.set_empty();
        }
        else if (name)
            x.shift_end(1); //give space back

        dst.append(x);
    }
    while (name);
}

///
struct memtrack_imp : memtrack {
    bool operator == (size_t k) const {
        return (size_t)name == k;
    }

    operator size_t() const {
        return (size_t)name;
    }

    //don't track this
    void* operator new(size_t size) { return ::dlmalloc(size); }
    void operator delete(void* ptr) { ::dlfree(ptr); } \
};

///
struct hash_memtrack {
    typedef size_t key_type;
    uint operator()(size_t x) const { return (uint)x; }
};

typedef hash_keyset<memtrack_imp, _Select_Copy<memtrack_imp, size_t>, hash_memtrack>
memtrack_hash_t;

///
struct memtrack_registrar
{
    volatile bool running = false;

    memtrack_hash_t* hash = 0;
    comm_mutex* mux = 0;

    bool enabled = default_enabled;
    bool ready = false;

    memtrack_registrar()
    {
        mux = new comm_mutex(500, false);
        hash = new memtrack_hash_t;

        ready = true;
    }

    virtual ~memtrack_registrar() {}

    //@note virtual methods to avoid breaking dlls when exe implementation changes

    ///Track allocation
    virtual void alloc(const std::type_info* tracking, size_t size)
    {
        static bool inside = false;
        if (inside)
            return;     //avoid stack overlow from hashmap

        const char* name = tracking ? tracking->name() : "unknown";

        GUARDTHIS(*mux);
        inside = true;
        memtrack* val = hash->find_or_insert_value_slot((size_t)name);
        inside = false;

        val->name = name;

        ++val->nallocs;
        ++val->ncurallocs;
        ++val->nlifeallocs;
        val->size += size;
        val->cursize += size;
        val->lifesize += size;
    }

    ///Track freeing
    virtual void free(const std::type_info* tracking, size_t size)
    {
        const char* name = tracking ? tracking->name() : "unknown";

        GUARDTHIS(*mux);
        memtrack_imp* val = const_cast<memtrack_imp*>(hash->find_value((size_t)name));

        if (val) {
            //val->size -= size;
            val->cursize -= size;
            --val->ncurallocs;
        }
    }

    virtual uint list(memtrack* dst, uint nmax, bool modified_only) const
    {
        GUARDTHIS(*mux);
        memtrack_hash_t::iterator ib = hash->begin();
        memtrack_hash_t::iterator ie = hash->end();

        uint i = 0;
        for (; ib != ie && i < nmax; ++ib) {
            memtrack& p = *ib;
            if (p.nallocs == 0 && modified_only)
                continue;

            dst[i++] = p;
            p.nallocs = 0;
            p.size = 0;
        }

        return i;
    }

    virtual void dump(const char* file, bool diff) const
    {
        GUARDTHIS(*mux);
        memtrack_hash_t::iterator ib = hash->begin();
        memtrack_hash_t::iterator ie = hash->end();

        bofstream bof(file);
        if (!bof.is_open())
            return;

        LOCAL_SINGLETON(charstr) dumpbuf;
        charstr& buf = *dumpbuf;

        buf.reserve(8000);
        buf.reset();

        buf << "======== bytes | #alloc |  type ======\n";

        int64 totalsize = 0;
        size_t totalcount = 0;

        for (; ib != ie; ++ib) {
            memtrack& p = *ib;
            if (diff ? (p.size == 0) : (p.cursize == 0))
                continue;

            ints size = diff ? p.size : ints(p.cursize);
            uint count = diff ? p.nallocs : p.ncurallocs;

            totalsize += size;
            totalcount += count;

            buf.append_num_thousands(size, ',', 12);
            buf.append_num(10, count, 9);
            buf << '\t';
            name_filter(buf, p.name);
            buf << '\n';

            if (buf.len() > 7900) {
                bof.xwrite_token_raw(buf);
                buf.reset();
            }
        }

        buf << "======== bytes | #alloc |  type ======\n";
        buf.append_num_metric(totalsize, 14);
        buf << 'B';
        buf.append_num_thousands(totalcount, ',', 8);
        buf << "\t (total)\n";

        mallinfo ma = mspace_mallinfo(SINGLETON(comm_array_mspace).msp);
        mallinfo md = dlmallinfo();

        buf << "\nnon-mmapped space allocated from system: " << num_metric(md.arena + ma.arena, 8);    buf << 'B';
        buf << "\nnumber of free chunks:                   " << num_metric(md.ordblks + ma.ordblks, 8);
        buf << "\nmaximum total allocated space:           " << num_metric(md.usmblks + ma.usmblks, 8);  buf << 'B';
        buf << "\ntotal allocated space:                   " << num_metric(md.uordblks + ma.uordblks, 8); buf << 'B';
        buf << "\ntotal free space:                        " << num_metric(md.fordblks + ma.fordblks, 8); buf << 'B';
        buf << "\nreleasable (via malloc_trim) space:      " << num_metric(md.keepcost + ma.keepcost, 8); buf << 'B';

        bof.xwrite_token_raw(buf);
        bof.close();
    }

    virtual uint count() const {
        GUARDTHIS(*mux);
        return (uint)hash->size();
    }

    virtual void reset() {
        GUARDTHIS(*mux);
        memtrack_hash_t::iterator ib = hash->begin();
        memtrack_hash_t::iterator ie = hash->end();

        for (; ib != ie; ++ib) {
            memtrack& p = *ib;
            p.nallocs = 0;
            p.size = 0;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
static memtrack_registrar* memtrack_register()
{
    static bool reentry = false;
    if (reentry)
        return 0;

    reentry = true;
    LOCAL_PROCWIDE_SINGLETON_DEF(memtrack_registrar) reg;
    reentry = false;

    return reg.get();
}

////////////////////////////////////////////////////////////////////////////////
bool memtrack_enable(bool en)
{
    memtrack_registrar* mtr = memtrack_register();
    bool old = mtr->enabled;

    mtr->enabled = en;
    mtr->running = en & mtr->ready;

    return old;
}

////////////////////////////////////////////////////////////////////////////////
void memtrack_shutdown()
{
    memtrack_registrar* mtr = memtrack_register();
    mtr->running = mtr->enabled = mtr->ready = false;
}

////////////////////////////////////////////////////////////////////////////////
void memtrack_alloc(const std::type_info* tracking, size_t size)
{
    memtrack_registrar* mtr = memtrack_register();
    if (!mtr || !mtr->running) return;

    mtr->alloc(tracking, size);
}

////////////////////////////////////////////////////////////////////////////////
void memtrack_free(const std::type_info* tracking, size_t size)
{
    memtrack_registrar* mtr = memtrack_register();
    if (!mtr || !mtr->running) return;

    mtr->free(tracking, size);
}

////////////////////////////////////////////////////////////////////////////////
uint memtrack_list(memtrack* dst, uint nmax, bool modified_only)
{
    memtrack_registrar* mtr = memtrack_register();
    if (!mtr || !mtr->ready)
        return 0;

    return mtr->list(dst, nmax, modified_only);
}

////////////////////////////////////////////////////////////////////////////////
void memtrack_dump(const char* file, bool diff)
{
    memtrack_registrar* mtr = memtrack_register();
    if (!mtr || !mtr->ready)
        return;

    mtr->dump(file, diff);
}

////////////////////////////////////////////////////////////////////////////////
uint memtrack_count()
{
    memtrack_registrar* mtr = memtrack_register();
    if (!mtr) return 0;

    return mtr->count();
}

////////////////////////////////////////////////////////////////////////////////
void memtrack_reset()
{
    memtrack_registrar* mtr = memtrack_register();

    mtr->reset();
}

} //namespace coid
