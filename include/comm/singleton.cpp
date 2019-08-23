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
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#include "singleton.h"
#include "sync/mutex.h"
#include "hash/hashkeyset.h"

#include "binstream/filestream.h"
#include "binstream/txtstream.h"

namespace coid {

void memtrack_shutdown();


////////////////////////////////////////////////////////////////////////////////
static thread_key _t_creator_key;

static void* local_creator()
{
    return _t_creator_key.get();
}

fn_singleton_creator singleton_local_creator( void* p )
{
    _t_creator_key.set(p);
    return &local_creator;
}


////////////////////////////////////////////////////////////////////////////////
///Global singleton registrator
class global_singleton_manager
{
    struct killer
    {
        COIDNEWDELETE_NOTRACK

        void* ptr;
        void (*fn_destroy)(void*);
        const char* type_name;
        const char* file;
        int line;

        killer* next;

        token type;
        bool invisible;

        void destroy() {
            fn_destroy(ptr);
            ptr = 0;
        }

        killer( void* ptr, void (*fn_destroy)(void*), const token& type, const char* file, int line, bool invisible )
            : ptr(ptr), fn_destroy(fn_destroy), type_name(type.ptr()), type(type), file(file), line(line), invisible(invisible)
        {
            DASSERT(ptr);
        }
    };

public:

    global_singleton_manager() : mx(500, false)
    {
        last = 0;
        count = 0;
        shutting_down = false;
    }

    void* find_or_add_singleton(
        fn_singleton_creator create,
        fn_singleton_destroyer destroy,
        fn_singleton_initmod initmod,
        const token& type, const char* file, int line, bool invisible )
    {
        RASSERT( !shutting_down );
        comm_mutex_guard<_comm_mutex> mxg(mx);

        //look for singletons registered in different module
        killer* k = invisible ? 0 : last;
        while(k && (k->invisible || k->type != type))
            k = k->next;

        if(!k) {
            k = new killer(create(), destroy, type, file, line, invisible);
            k->next = last;

            last = k;
            ++count;
        }
        else if(initmod)
            initmod(k->ptr);

        _t_creator_key.set(0);

        return k->ptr;
    }

    void destroy()
    {
        uint n = count;

		if(last) {
#ifdef _DEBUG
            memtrack_shutdown();

			bofstream bof("singleton.log");
			txtstream tof(bof);
#endif

            shutting_down = true;

			while(last) {
				killer* tmp = last->next;

#ifdef _DEBUG
				tof << (n--) << " destroying '" << last->type << "' singleton created at "
					<< last->file << ":" << last->line << "\r\n";
				tof.flush();
#endif

				last->destroy();
				delete last;

				last = tmp;
			}
		}
    }

    ~global_singleton_manager() {
        //destroy();
    }

    static global_singleton_manager& get_global();
    static global_singleton_manager& get_local();

private:
    _comm_mutex mx;
    killer* last;

    uint count;

    bool shutting_down;
};


////////////////////////////////////////////////////////////////////////////////
void* singleton_register_instance(
    fn_singleton_creator fn_create,
    fn_singleton_destroyer fn_destroy,
    fn_singleton_initmod fn_initmod,
    const char* type,
    const char* file,
    int line,
    bool invisible )
{
    auto& gsm = invisible
        ? global_singleton_manager::get_local()
        : global_singleton_manager::get_global();

    return gsm.find_or_add_singleton(
        fn_create, fn_destroy, fn_initmod,
        type, file, line, invisible);
}

////////////////////////////////////////////////////////////////////////////////
void singletons_destroy()
{
    auto& gsm = global_singleton_manager::get_local();

    memtrack_shutdown();
    gsm.destroy();
}

////////////////////////////////////////////////////////////////////////////////
//code for process-wide singletons

#define GLOBAL_SINGLETON_REGISTRAR sglo_registrar

#ifdef SYSTYPE_WIN
typedef int (__stdcall *proc_t)();

extern "C"
__declspec(dllimport) proc_t __stdcall GetProcAddress (
    void* hmodule,
    const char* procname
    );

extern "C"
_declspec(dllimport) void* __stdcall GetModuleHandleA(const char * lpModuleName
);


typedef global_singleton_manager* (*ireg_t)();

#define MAKESTR(x) STR(x)
#define STR(x) #x

extern "C" __declspec(dllexport) global_singleton_manager* GLOBAL_SINGLETON_REGISTRAR()
{
    static global_singleton_manager _gsm;
    return &_gsm;
}

global_singleton_manager& global_singleton_manager::get_global()
{
    static global_singleton_manager* _this=0;

    if(!_this) {
        //retrieve process-wide singleton from exported fn
        const char* s = MAKESTR(GLOBAL_SINGLETON_REGISTRAR);
        ireg_t p = (ireg_t)GetProcAddress(GetModuleHandleA(NULL), s);
        if(!p) {
            //entry point for global singleton not found in exe
            //probably a 3rd party exe
            _this = GLOBAL_SINGLETON_REGISTRAR();
        }
        else
            _this = p();
    }

    return *_this;
}

#else
/*
extern "C" __attribute__ ((visibility("default"))) interface_register_impl* INTERGEN_GLOBAL_REGISTRAR();
{
    return &SINGLETON(interface_register_impl);
}*/

global_singleton_manager& global_singleton_manager::get_global()
{
    static global_singleton_manager _this;

    return _this;
}

#endif

global_singleton_manager& global_singleton_manager::get_local()
{
    static global_singleton_manager _this;

    return _this;
}

} //namespace coid
