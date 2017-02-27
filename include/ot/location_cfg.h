
#ifndef __OT_LOCATION_CFG_H__
#define __OT_LOCATION_CFG_H__

#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"

namespace ot {

///Location/camera info
struct location_info
{
    double longitude;                   //< longitude in degrees
    double latitude;                    //< latitude in degrees
    double time_of_day;                 //< time of day [msec]
    int64 day_of_year;                  //< days since epoch
    float3 hpr;                         //< heading/pitch/roll angles in degrees
    float altitude;                     //< altitude above mean sea level
};

}

namespace coid {
inline metastream& operator || (metastream& m, ot::location_info& v) {
    return m.compound("ot::location_info", [&]() {
        m.member("longitude", v.longitude);
        m.member("latitude", v.latitude);
        m.member("time_of_day", v.time_of_day);
        m.member("day_of_year", v.day_of_year);
        m.member("hpr", v.hpr);
        m.member("altitude", v.altitude);
    });
}
}

#endif //__OT_LOCATION_CFG_H__
