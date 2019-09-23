
#ifndef _OT_ENV_H_
#define _OT_ENV_H_

#include "glm/glm_meta.h"
#include <comm/binstream/binstream.h>
#include <comm/metastream/metastream.h>

namespace ot {

///Atmospheric parameters
struct atmospheric_params
{
    enum {
        SUN_COLOR                       = 0x01,
        RAYLEIGH                        = 0x02,
        MIE                             = 0x04,
        INTENSITY                       = 0x08,
        GROUND_REFLECTANCE              = 0x10,
        EXPOSURE                        = 0x20,
        SCATTERING                      = 0x40,
        MIN_AMBIENT                     = 0x80,
        SHADOW_LIGHT                    = 0x100,
    };

    unsigned int mask;

    float3 sun_color;
    float3 rayleigh;
    float mie;

    float intensity;
    float min_ambient_intensity;
    float ground_reflectance;
    float shadow_light;
    float exposure;
    float scattering;

    atmospheric_params()
    {
        ::memset(this, 0, sizeof(*this));
    }

    friend coid::metastream& operator || (coid::metastream& m, atmospheric_params& d)
    {
        return m.compound("atmospheric_params", [&]()
        {
            m.member("mask", d.mask, 0);
            m.member("sun_color", d.sun_color, float3(1));
            m.member("rayleigh", d.rayleigh, float3(5.8e-3f, 1.7e-2f, 4.1e-2f));
            m.member("mie", d.mie, 4e-3f);
            m.member("intensity", d.intensity, 25.f);
            m.member("ground_reflectance", d.ground_reflectance, 0.08f);
            m.member("exposure", d.exposure, 4.0f);
            m.member("shadow_light", d.shadow_light, 1.0f);
            m.member("scattering", d.scattering, 0.5f);
            m.member("min_ambient", d.min_ambient_intensity, 5e-8f);
        });
    }
};

///Water parameters
struct water_params
{
    enum {
        HALF_DEPTH                      = 0x01,
        SCATTERING                      = 0x02,
    };

    unsigned int mask;

    float3 half_depth;
    float3 scattering;

    water_params()
    {
        ::memset(this, 0, sizeof(*this));
    }

    friend coid::metastream& operator || (coid::metastream& m, water_params& d)
    {
        return m.compound("water_params", [&]()
        {
            m.member("mask", d.mask, 0);
            m.member("half_depth", d.half_depth, float3(0.33f, 1.6f, 3.8f));
            m.member("scattering", d.scattering, float3(0.005f));
        });
    }
};

///Fog parameters
struct fog_params
{
    enum {
        HALF_DEPTH                      = 0x01,
        SCATTERING                      = 0x02,
        LEVEL                           = 0x04,
    };

    unsigned int mask;

    float half_depth;
    float scattering;
    float level;

    fog_params()
    {
        ::memset(this, 0, sizeof(*this));
    }

    friend coid::metastream& operator || (coid::metastream& m, fog_params& d)
    {
        return m.compound("fog_params", [&]()
        {
            m.member("mask", d.mask, 0);
            m.member("half_depth", d.half_depth, 20);
            m.member("scattering", d.scattering, 0.005f);
            m.member("level", d.level, 0);
        });
    }
};


///Cloud params
struct cloud_params
{
    float base = 3000;                  //< cloud base elevation [m]
    float height = 4000;                //< max cloud thickness [m]

    float get_rain_alt() const { return base + height * .5f; }

    friend coid::metastream& operator || (coid::metastream& m, cloud_params& d)
    {
        return m.compound("cloud_params", [&]()
        {
            m.member("base", d.base, 3000);
            m.member("height", d.height, 4000);
        });
    }
};


///Forest parameters
struct forest_params
{
    float3 ecs_min;                     //< elevation/curvature/slope min values
    float3 ecs_max;                     //< elevation/curvature/slope max values
    float3 ecs_trans;                   //< elevation/curvature/slope transitional width values
    float threshold;                    //< forest threshold from vegetation density value (0..1)
    float aspect;                       //< aspect (sunny side) vegetation value bias 

    forest_params() {
        coid::metastream::initialize_from_defaults(this);
    }

    friend coid::metastream& operator || (coid::metastream& m, forest_params& w)
    {
        return m.compound("forest", [&]()
        {
            m.member("ecs_min", w.ecs_min,      float3(    0.0f, -0.05f, 0.03f));
            m.member("ecs_max", w.ecs_max,      float3( 6000.0f,  1.00f, 0.3f));
            m.member("ecs_trans", w.ecs_trans,  float3(  500.0f,  0.10f, 0.2f));
            m.member("threshold", w.threshold, 0.4f);
            m.member("aspect", w.aspect, 0.4f);

            m.member_obsolete<int>("render_distcoef");
            m.member_obsolete<float>("render_distance");
            m.member_obsolete<float>("shadow_distance");
            m.member_obsolete<float>("shadow_range");
            m.member_obsolete<bool>("shading");
        });
    }
};

///Virtual elevation params, used for latitude-dependent teperature computation for snow and vegetation
struct snow_params
{
    float2 virtelev;                    //< virtual elevation latitude dependency coefficients, x*sin(lat) + y*sin(lat)^2
    float virtcurv;                     //< curvature effect on virtual elevation

    float snowmin;                      //< virtual elevation at which snow starts to appear
    float snowsat;                      //< virtual elevation at which snow covers everything but steep rock faces

    snow_params() {
        coid::metastream::initialize_from_defaults(this);
    }

    friend coid::metastream& operator || (coid::metastream& m, snow_params& w)
    {
        return m.compound("virtual_elevation", [&]()
        {
            m.member("virtelev", w.virtelev, float2(3500,0));
            m.member("virtcurv", w.virtcurv, 2500.0f);
            m.member("snowmin", w.snowmin, 4900.0f);
            m.member("snowsat", w.snowsat, 6000.0f);
        });
    }
};


///Weather params
struct weather_params
{
    float wind_heading;                 //< wind heading in degrees, north 0, east 90
    float wind_speed;                   //< wind speed at the gradient height, in m/s
    float wind_gradient_height;         //< gradient height: 457m large cities, 366m suburbs, 274m open terrain, 213m open sea
    float wind_stability;               //< Hellmann exponent, 0.06 .. 0.60, default 1/7
    float wind_turbulence;              //< 0..7, http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19980028448_1998081596.pdf
	
    float cloud_density;                //< cloud cover density, 0..1
    float rain_density;                 //< rain density, 0..1
    float snow_density;                 //< snow density, 0..1
    float lightning_per_kmsqmin;         //< lightning bolts count per sqare kilometer and minute

    float auto_weather_period;          //< period in which weather changes if auto weather enabled
    bool auto_weather;                  //< automatic weather change

    weather_params() {
        coid::metastream::initialize_from_defaults(this);
    }

    explicit weather_params(int) {
    }

    float wind_speed_at_height( float h ) const {
        return pow(glm::max(1e-6f, h) / wind_gradient_height, wind_stability) * wind_speed;
    }

    //@return distance travelled from launch point at 0 when wind-carried particle is at h
    float distance_at_height( float h ) const {
        return h * pow(glm::max(1e-6f, h) / wind_gradient_height, wind_stability) / (1 + wind_stability);
    }

    friend coid::metastream& operator || (coid::metastream& m, weather_params& w)
    {
        return m.compound("weather_params", [&]()
        {
            m.member("wind_heading", w.wind_heading, 90.0f);
            m.member("wind_speed", w.wind_speed, 10.0f);
            m.member("wind_gradient_height", w.wind_gradient_height, 270.0f);
            m.member("wind_stability", w.wind_stability, 1.0f/7);
            m.member("wind_turbulence", w.wind_turbulence, 0.0f);

            m.member("cloud_density", w.cloud_density, 0.05f);
            m.member("rain_density", w.rain_density, 0);
            m.member("snow_density", w.snow_density, 0);
            m.member("lightning_per_kmsqmin", w.lightning_per_kmsqmin, 0.0f);
            m.member_obsolete<float>("lightning_probability_multiplier");

            m.member("auto_weather_period", w.auto_weather_period, 100);
            m.member("auto_weather", w.auto_weather, true);
        });
    }
};

struct water_state_params
{
	float sea_dominant_wave_length;
	float sea_wave_amplitude_multiplier;
	float sea_foam_multiplier;
	float sea_current_heading;
	float sea_current_speed;
	float sea_wind_contribution;
    float sea_surf_amplitude_multiplier;

    water_state_params() {
        coid::metastream::initialize_from_defaults(this);
    }

	friend coid::metastream& operator || (coid::metastream& m, water_state_params& ws)
	{
		return m.compound("water_state_params", [&]()
		{
			m.member("sea_dominant_wave_length", ws.sea_dominant_wave_length, 10.0f);
			m.member("sea_wave_amplitude_multiplier", ws.sea_wave_amplitude_multiplier, 1.0f);
			m.member("sea_foam_multiplier", ws.sea_foam_multiplier, 1.0f);
			m.member("sea_current_heading", ws.sea_current_heading, 0.0f);
			m.member("sea_current_speed", ws.sea_current_speed, 0.0f);
			m.member("sea_wind_contribution", ws.sea_wind_contribution, 0.0f);
            m.member("sea_surf_amplitude_multiplier", ws.sea_surf_amplitude_multiplier, 1.0f);
        });
	}
};


} //namespace ot

#endif //_OT_ENV_H_
