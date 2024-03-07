#pragma once

// Include necessary header files
#include "ot/aircraft_script.h"
#include "ot/script_module.h"

// Define a constant for PI
const double PI = 3.14159265358979323846;

// Declare struct for bone members
struct bones
{
	static int propeller;
	static int wheel_front;
	static int wheel_right;
	static int wheel_left;
	static int elevator_right;
	static int elevator_left;
	static int rudder;
	static int aileron_right;
	static int aileron_left;
	static int rudder_pedal_right;
	static int rudder_pedal_left;
	static int brake_pedal_left;
	static int brake_pedal_right;
	static int throttle_handle;
};

// Declare struct for mesh members
struct meshes
{
	static int prop_blur;
	static int blade_one;
	static int blade_two;
	static int blade_three;
};

// Declare struct for sound members
struct sounds
{
	static int rumble;
	static int eng_exterior;
	static int eng_interior;
};

// Declare struct for sound source members
struct sources
{
	static int exterior;
	static int interior;
	static int rumble;
};

// Declare class DA40 derived from ot::aircraft_script
class DA40 : public ot::aircraft_script
{
	// Function to initialize chassis parameters
	ot::chassis_params init_chassis( const coid::charstr& params ) override;
	// Function to initialize the aircraft (called on startup or reload)
	void initialize(bool reload) override;
	// Function to handle functionality each frame
	void update_frame( float dt ) override;

	// Function to handle events, when engine starts/stop
	void engine(int flags, uint code, uint channel, int handler_id);
	// Utility function to clamp a value within a specified range
	float clamp(float val, float minval, float maxval);
	// Function to handle landing lights events
	void landing_lights(float val, uint code, uint channel, int handler_id);
	// Function to handle navigation lights events
	void navigation_lights(float val, uint code, uint channel, int handler_id);

	// Flag indicating whether the aircraft has started
	bool started;
	// Parameters for lights
	ot::light_params light_params;
	// Offset for navigation lights
	uint nav_light_offset;

	// Reference to the JSBSim object
	iref <ot::jsb> jsbsim = nullptr;
	// Reference to the geometry object
	iref<ot::geomob> geom = nullptr;
	// Reference to the sound group
	iref<ot::sndgrp> snd = nullptr;
};
