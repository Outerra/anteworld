#include <comm/intergen/ifc.h>
#include "ifc.jsc.h"

namespace jsc
{

static JSC g_jsc;


JSContextGroupRef jsc_get_current_group() { return g_jsc._group; }
void jsc_queue_finalize(void* ptr, void (*foo)(void*)) { g_jsc.queue_finalize(ptr, foo); } 


};