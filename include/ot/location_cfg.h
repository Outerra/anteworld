
#ifndef __OT_LOCATION_CFG_H__
#define __OT_LOCATION_CFG_H__

#include <comm/str.h>
#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

#include "glm/glm_meta.h"
#include "cubeface.h"

class planet;

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

///
struct ecef_data
{
    class planet* planet = 0;

    double3 xyz;
    double radius = 0;

    quat rot;

    coid::charstr name;
    coid::charstr auth;
    coid::charstr desc;
    coid::charstr title;
    coid::timet time = 0;

    int width = 0, height = 0;          //< resize to
    int params = 0;

    bool is_location = false;

    ecef_data(class planet* planet, const double3& pos, double radius, const glm::quat& rot, coid::timet t)
        : planet(planet)
        , xyz(pos), radius(radius), rot(rot), time(t)
    {}

    ecef_data() = default;
};


///
struct location_data
{
    const static int MSECDAY = 86400000;

    double longitude = 0;
    double latitude = 0;
    float altitude = 0;
    float heading = 0;                  //< [deg]
    float pitch = 0;                    //< [deg]
    float roll = 0;                     //< [deg]

    coid::timet time = 0;

    union {
        struct {
            uint flg_valid : 1;
            uint flg_altitude : 1;
            uint flg_heading : 1;
            uint flg_heading_magnetic : 1;
            uint flg_pitch : 1;
            uint flg_roll : 1;
            uint flg_time : 1;
        };
        uint bits = 0;
    };

    location_data() = default;

    location_data(const ecef_data& ecef)
    {
        xyz_to_lonlat_degrees(&ecef.xyz.x, &longitude);

        altitude = float(glm::length(ecef.xyz) - ecef.radius);

        float3 hpr = glm::get_heading_pitch_roll(ecef.xyz, ecef.rot, true);
        heading = glm::degrees(hpr.x);
        pitch = glm::degrees(hpr.y);
        roll = glm::degrees(hpr.z);

        time = ecef.time;

        flg_valid = 1;
        flg_altitude = 1;
        flg_heading = 1;
        flg_heading_magnetic = 0;
        flg_pitch = 1;
        flg_roll = 1;
    }

    location_data(const location_info& info)
    {
        altitude = info.altitude;
        heading = info.hpr.x;
        pitch = info.hpr.y;
        roll = info.hpr.z;
        latitude = info.latitude;
        longitude = info.longitude;

        time = utc_time(info.day_of_year, info.time_of_day);

        flg_valid = 1;
        flg_altitude = 1;
        flg_heading = 1;
        flg_heading_magnetic = 0;
        flg_pitch = 1;
        flg_roll = 1;
        flg_time = 1;
    }


    void get_ecef_data(double radius, ecef_data& data) const
    {
        double r = radius;
        if (flg_altitude)
            r += altitude;

        ot::lonlat_degrees_to_xyz(longitude, latitude, &data.xyz.x, r);
        data.radius = radius;

        float3 hpr;
        hpr.x = flg_heading ? glm::radians(heading) : 0.0f;
        hpr.y = flg_pitch ? glm::radians(pitch) : 0.0f;
        hpr.z = flg_roll ? glm::radians(roll) : 0.0f;

        data.rot = glm::set_heading_pitch_roll(data.xyz, hpr, true);

        data.time = time;
    }

    //@return solar time in ms
    double solar_time(int64* day = 0) const
    {
#ifdef SYSTYPE_MSVC
        struct tm tm;
        _gmtime64_s(&tm, &time.t);
#else
        time_t tv = (time_t)time.t;
        struct tm const& tm = *gmtime(&tv);
#endif

        if (day)
            *day = tm.tm_yday;

        int secs = (tm.tm_hour * 60 + tm.tm_min) * 60 + tm.tm_sec;

        double solar_offset = MSECDAY * longitude / 360.0;
        double tday = secs * 1000.0 + solar_offset;

        return ::fmod(tday + MSECDAY, MSECDAY);
    }


    coid::timet utc_time(int64 dyear, double tdayms) const
    {
        //take current year, compute UTC
        coid::timet now;

#ifdef SYSTYPE_MSVC
        struct tm tm;
        _gmtime64_s(&tm, &now.t);
#else
        time_t tv = (time_t)now.t;
        struct tm tm = *gmtime(&tv);
#endif

        tm.tm_mon = 0;
        tm.tm_mday = 1;
        tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

        time_t dst;
#ifdef SYSTYPE_MSVC
        dst = _mkgmtime(&tm);
#else
        dst = timegm(&tm);
#endif

        dst += dyear * 86400 + int(tdayms * 0.001);

        return dst;
    }
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
