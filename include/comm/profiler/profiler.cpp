#include "profiler.h"

#include "../atomic/atomic.h"
#include "../singleton.h"
#include "../timer.h"

namespace profiler
{

auto& g_backend = PROCWIDE_SINGLETON(backend*);

uint64 now() { return coid::nsec_timer::current_time_ns(); }

void set_thread_name(const char* name) {
    if (g_backend) g_backend->set_thread_name(name);
}

uint64 create_transient_link() {
    static volatile uint32 link;
    return atomic::inc(&link);
}

uint64 create_fixed_link() {
    static volatile uint32 link;
    return atomic::inc(&link) | ((uint64)1 << 32);
}

void push_link(uint64 link) {
    if (g_backend) g_backend->push_link(link);
}

void push_number(const char* label, uint value) {
    if (g_backend) g_backend->push_number(label, value);
}

void push_string(const char* string) {
    if (g_backend) g_backend->push_string(string);
}

void frame() {
    if (g_backend) g_backend->frame();
}

void gpu_frame() {
    if (g_backend) g_backend->gpu_frame();
}

void begin_gpu(const coid::token& name, uint64 timestamp, uint64 order)
{
    if (g_backend) g_backend->begin_gpu(name, timestamp, order);
}

void end_gpu(const coid::token& name, uint64 timestamp, uint64 order)
{
    if (g_backend) g_backend->end_gpu(name, timestamp, order);
}

uint64 get_token(const char* name) {
    return g_backend ? g_backend->get_token(name) : 0;
}

void begin(uint64 token, uint8 r, uint8 g, uint8 b) {
    if (g_backend) g_backend->begin(token, r, g, b);
}

void begin(const char* name, uint8 r, uint8 g, uint8 b) { 
    begin(get_token(name), r, g, b);
}

void end() {
    if (g_backend) g_backend->end();
}

void set_backend(backend* backend)
{
    g_backend = backend;
}

} // namespace profiler
