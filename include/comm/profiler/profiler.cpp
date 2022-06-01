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

#include "profiler.h"

#include "../atomic/atomic.h"
#include "../singleton.h"
#include "../timer.h"

namespace profiler
{

auto& g_backend = PROCWIDE_SINGLETON(backend*);

uint64 now()
{
    return coid::nsec_timer::current_time_ns();
}

void set_thread_name(const char* name)
{
    if (g_backend)
        g_backend->set_thread_name(name);
}

uint64 create_transient_link()
{
    static volatile uint32 link;
    return atomic::inc(&link);
}

uint64 create_fixed_link()
{
    static volatile uint32 link;
    return atomic::inc(&link) | ((uint64)1 << 32);
}

void push_link(uint64 link)
{
    if (g_backend)
        g_backend->push_link(link);
}

void push_number(const char* label, uint value)
{
    if (g_backend)
        g_backend->push_number(label, value);
}

void push_string(const char* string)
{
    if (g_backend)
        g_backend->push_string(string);
}

void frame()
{
    if (g_backend)
        g_backend->frame();
}

void gpu_frame()
{
    if (g_backend)
        g_backend->gpu_frame();
}

void begin_gpu(const coid::token& name, uint64 timestamp, uint64 order)
{
    if (g_backend)
        g_backend->begin_gpu(name, timestamp, order);
}

void end_gpu(const coid::token& name, uint64 timestamp, uint64 order)
{
    if (g_backend)
        g_backend->end_gpu(name, timestamp, order);
}

uint64 get_token(const char* name)
{
    return g_backend ? g_backend->get_token(name) : 0;
}

void begin(uint64 token, uint8 r, uint8 g, uint8 b)
{
    if (g_backend)
        g_backend->begin(token, r, g, b);
}

void begin(const char* name, uint8 r, uint8 g, uint8 b)
{ 
    begin(get_token(name), r, g, b);
}

void end()
{
    if (g_backend)
        g_backend->end();
}

void set_backend(backend* backend)
{
    g_backend = backend;
}

} // namespace profiler
