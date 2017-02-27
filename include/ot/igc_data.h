#ifndef __IGC_DATA__H__
#define __IGC_DATA__H__

namespace ot {

///Run-time data for IGC interface
struct igc_data
{
    double lon, lat;                    ///< longitude & latitude [rad]

    float heading;                      ///< heading angle [rad]
    float pitch;                        ///< pitch angle [rad]
    float roll;                         ///< roll angle [rad]
    
    float speed;                        ///< approximate flight speed [m/s]

    float alt_msl;                      ///< altitude above the mean sea level [m]
    float alt_grd;                      ///< altitude above the ground [m]
};

} //namespace ot



////////////////////////////////////////////////////////////////////////////////

#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>


namespace coid {
/*
inline binstream& operator << (binstream& bin, const ot::igc_data& w) {
    return bin << w.lon << w.lat << w.heading << w.pitch << w.roll
        << w.speed << w.alt_msl << w.alt_grd;
}

inline binstream& operator >> (binstream& bin, ot::igc_data& w) {
    return bin >> w.lon >> w.lat >> w.heading >> w.pitch >> w.roll
        >> w.speed >> w.alt_msl >> w.alt_grd;
}

inline metastream& operator << (metastream& m, const ot::igc_data& w)
{
    MSTRUCT_OPEN(m,"ot::igc_data")
    MM(m,"lon",w.lon)
    MM(m,"lat",w.lat)
    MM(m,"heading",w.heading)
    MM(m,"pitch",w.pitch)
    MM(m,"roll",w.roll)
    MM(m,"speed",w.speed)
    MM(m,"alt_msl",w.alt_msl)
    MM(m,"alt_grd",w.alt_grd)
    MSTRUCT_CLOSE(m)
}*/
inline metastream& operator || (metastream& m, ot::igc_data& w)
{
    return m.compound("ot::igc_data", [&]()
    {
        m.member("lon", w.lon);
        m.member("lat", w.lat);
        m.member("heading", w.heading);
        m.member("pitch", w.pitch);
        m.member("roll", w.roll);
        m.member("speed", w.speed);
        m.member("alt_msl", w.alt_msl);
        m.member("alt_grd", w.alt_grd);
    });
}

}

#endif //__IGC_DATA__H__
