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
 * Outerra.
 * Portions created by the Initial Developer are Copyright (C) 2020
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Mikulas Florek
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

#include "../commtypes.h"
#include "../sync/mutex.h"

namespace profiler
{

class backend {
public:
    virtual void frame() = 0;
    virtual void gpu_frame() = 0;
    virtual uint64 get_token(const char* name) = 0;
    virtual void begin(uint64 token, uint8 r, uint8 g, uint8 b) = 0;
    virtual void end() = 0;
    virtual void begin_gpu(const coid::token& name, uint64 timestamp, uint64 order) = 0;
    virtual void end_gpu(const coid::token& name, uint64 timestamp, uint64 order) = 0;
    virtual void set_thread_name(const char* name) = 0;
    virtual void push_string(const char* string) = 0;
    virtual void push_number(const char* label, uint value) = 0;
    virtual void push_link(uint64 link) = 0;
};

void set_backend(backend* backend);

void frame();
void gpu_frame();
uint64 get_token(const char* name);
void begin(uint64 token, uint8 r = 0xa0, uint8 g = 0xa0, uint8 b = 0xa0);
void begin(const char* name, uint8 r = 0xa0, uint8 g = 0xa0, uint8 b = 0xa0);
void push_string(const char* string);
void push_number(const char* label, uint value);
void push_link(uint64 link);
void end();
void set_thread_name(const char* name);
void begin_gpu(const coid::token& name, uint64 timestamp, uint64 order);
void end_gpu(const coid::token& name, uint64 timestamp, uint64 order);

// returns unique value in 0-0xffFFffFF range
// use this for shortlived stuff,
uint64 create_transient_link();
uint64 create_fixed_link(); // returns unique value in 0-0xffFFffFF range + 33rd bit set

uint64 now();

struct scope final
{
    scope(uint64 token, uint8 r = 0xa0, uint8 g = 0xa0, uint8 b = 0xa0) { begin(token, r, g, b); }
    ~scope() { end(); }
};

} // namespace profiler

#define CPU_PROFILE_CONCAT2(a, b) a ## b
#define CPU_PROFILE_CONCAT(a, b) CPU_PROFILE_CONCAT2(a, b)

#define CPU_PROFILE_BEGIN(name) static uint64 CPU_PROFILE_CONCAT(g_mp, __LINE__) = profiler::get_token(name); profiler::begin(CPU_PROFILE_CONCAT(g_mp, __LINE__));
#define CPU_PROFILE_END() profiler::end();
#define CPU_PROFILE_SCOPE(name) static uint64 CPU_PROFILE_CONCAT(g_mp, __LINE__) = profiler::get_token(name); profiler::scope CPU_PROFILE_CONCAT(foo, __LINE__)(CPU_PROFILE_CONCAT(g_mp, __LINE__));
#define CPU_PROFILE_SCOPE_COLOR(name, r, g, b) static uint64 CPU_PROFILE_CONCAT(g_mp, __LINE__) = profiler::get_token(name); profiler::scope CPU_PROFILE_CONCAT(foo, __LINE__)(CPU_PROFILE_CONCAT(g_mp, __LINE__), r, g, b);
#define CPU_PROFILE_FUNCTION() static uint64 CPU_PROFILE_CONCAT(g_mp, __LINE__) = profiler::get_token(__FUNCTION__); profiler::scope CPU_PROFILE_CONCAT(foo, __LINE__)(CPU_PROFILE_CONCAT(g_mp, __LINE__));
