#pragma once

#include "tutorial_aircraft_plugin.hpp"

// Define bone static members (better to define to -1 for debug purpose)
int bones::propeller = -1;
int bones::wheel_front = -1;
int bones::wheel_right = -1;
int bones::wheel_left = -1;
int bones::elevator_right = -1;
int bones::elevator_left = -1;
int bones::rudder = -1;
int bones::aileron_right = -1;
int bones::aileron_left = -1;
int bones::flap_right = -1;
int bones::flap_left = -1;
int bones::rudder_pedal_right = -1;
int bones::rudder_pedal_left = -1;
int bones::brake_pedal_left = -1;
int bones::brake_pedal_right = -1;
int bones::throttle_handle = -1;

// Define mesh static members
int meshes::prop_blur = -1;
int meshes::blade_one = -1;
int meshes::blade_two = -1;
int meshes::blade_three = -1;

// Define sound static members
int sounds::rumble = -1;
int sounds::eng_exterior = -1;
int sounds::eng_interior = -1;
int sounds::prop_exterior = -1;
int sounds::prop_interior = -1;

// Define sound source static members
int sources::rumble_exterior = -1;
int sources::rumble_interior = -1;
int sources::eng_exterior = -1;
int sources::eng_interior = -1;
int sources::prop_exterior = -1;
int sources::prop_interior = -1;

// Register the DA40 class as a client
namespace
{
	IFC_REGISTER_CLIENT(tutorial_aircraft_plugin);
};

// Utility function to clamp a value within a specified range
float tutorial_aircraft_plugin::clamp(float val, float minval, float maxval)
{
	if (val < minval)
	{
		return minval;
	}
	else if (val > maxval)
	{
		return maxval;
	}
	else
	{
		return val;
	}
}

// Function to handle events on engine start/stop
void tutorial_aircraft_plugin::engine(int flags, uint code, uint channel, int handler_id)
{
	//Safety checks for jsbsim pointer to avoid crashes
	if (!this->jsbsim)
	{
		return;
	}

	// Toggle the engine state every time this function is called 
	this->started ^= 1;

	// Start or stop the engine based on the state
	if (this->started == 1)
	{
		//For JSBSim aircraft to start, the starter and magneto needs to be activated
		//Turn on engine (0 - turn off, 1 - turn on)
		this->jsbsim->operator()("propulsion/starter_cmd", 1);
	/* "this->jsbsim->operator()" uses jsbsim overloaded operator() function, where
		1.parameter - Jsbsim property, you want to get / set
		2.parameter - value to be assigned to the property(only if the property can be set)*/
		this->jsbsim->operator()("propulsion/magneto_cmd", 1);
		//debug tip: when the magneto isn't activated, it results in decreased piston engine power, with the rpm not exceeding 1000
		
	}
	else
	{
		this->jsbsim->operator()("propulsion/starter_cmd", 0);
		this->jsbsim->operator()("propulsion/magneto_cmd", 0);
	}
}


void tutorial_aircraft_plugin::brakes(float val, uint code, uint channel, int handler_id)
{
	this->braking = val;
	this->jsbsim->operator()("fcs/center-brake-cmd-norm", val);
	this->jsbsim->operator()("fcs/left-brake-cmd-norm", val);
	this->jsbsim->operator()("fcs/right-brake-cmd-norm", val);
};

// Function to handle landing lights events
void tutorial_aircraft_plugin::landing_lights(float val, uint code, uint channel, int handler_id)
{
	// Toggle landing lights based on the input value
	light_mask(0x3, val > 0);
};

// Function to handle navigation lights events
void tutorial_aircraft_plugin::navigation_lights(float val, uint code, uint channel, int handler_id)
{
	// Toggle navigation lights based on the input value
	light_mask(0x3, val > 0, nav_light_offset);
}

// Function to handle ailerons events
void tutorial_aircraft_plugin::ailerons(float val, uint code, uint channel, int handler_id)
{
	this->jsbsim->operator()("fcs/aileron-cmd-norm", val);
}

// Function to handle elevator events
void tutorial_aircraft_plugin::elevator(float val, uint code, uint channel, int handler_id)
{
	this->jsbsim->operator()("fcs/elevator-cmd-norm", -val);
}

ot::chassis_params tutorial_aircraft_plugin::init_chassis(const coid::charstr& params)
{
	// Define bone members
	bones::propeller = get_joint_id("propel");
	bones::wheel_front = get_joint_id("front_wheel");
	bones::wheel_right = get_joint_id("right_wheel");
	bones::wheel_left = get_joint_id("left_wheel");
	bones::elevator_right = get_joint_id("elevator_right");
	bones::elevator_left = get_joint_id("elevator_left");
	bones::rudder = get_joint_id("rudder");
	bones::aileron_right = get_joint_id("aileron_right");
	bones::aileron_left = get_joint_id("aileron_left");
	bones::flap_right = get_joint_id("flap_right");
	bones::flap_left = get_joint_id("flap_left");
	bones::rudder_pedal_right = get_joint_id("rudder_pedal_right");
	bones::rudder_pedal_left = get_joint_id("rudder_pedal_left");
	bones::brake_pedal_left = get_joint_id("brake_pedal_left");
	bones::brake_pedal_right = get_joint_id("brake_pedal_right");
	bones::throttle_handle = get_joint_id("throttle_lever");

	//Get mesh id, and define mesh members.
	//For that use function get_mesh_id() 
	// parameter - mesh name
	meshes::prop_blur = get_mesh_id("propel_blur");
	meshes::blade_one = get_mesh_id("main_blade_01");
	meshes::blade_two = get_mesh_id("main_blade_02");
	meshes::blade_three = get_mesh_id("main_blade_03");

	// Define sounds
	//engine rumble - in this case it's used for the outside sounds and also the inside
	sounds::rumble = load_sound("sounds/engine/engn1.ogg");
	//Interior sounds - will be heard from inside the plane
	 //engine
	sounds::eng_interior = load_sound("sounds/engine/engn1_inn.ogg");
	//propeller
	sounds::prop_interior = load_sound("sounds/engine/prop1_inn.ogg");
	//Exterior sounds - will be heard from outside the plane 
	 //engine
	sounds::eng_exterior = load_sound("sounds/engine/engn1_out.ogg");
	//propeller
	sounds::prop_exterior = load_sound("sounds/engine/prop1_out.ogg");

	//Add sound emitters
	//Interior emitters
	 //For interior rumbling sound
	sources::rumble_interior = add_sound_emitter_id(bones::propeller, -1, 0.5f);
	//For interior engine sounds
	sources::eng_interior = add_sound_emitter_id(bones::propeller, 0, 0.5f);
	//For interior propeller sounds
	sources::prop_interior = add_sound_emitter_id(bones::propeller, 0, 0.5f);
	//Exterior emitters
	 //For exterior rumbling sound
	sources::rumble_exterior = add_sound_emitter_id(bones::propeller, 1, 3.0f);
	//For exterior engine sounds 
	sources::eng_exterior = add_sound_emitter_id(bones::propeller, 0, 3.0f);
	//For exterior propeller sounds 
	sources::prop_exterior = add_sound_emitter_id(bones::propeller, 0, 3.0f);

	// Set up landing lights
	ot::light_params light_parameters = { .size = 0.1f, .angle = 100.f, .edge = 0.25f, .intensity = 5.f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .fadeout = 0.05f, };
	add_spot_light({ 4.5f, 1.08f, 0.98f }, { -0.1f, 1.f, 0.3f }, light_parameters);
	add_spot_light({ -4.5f, 1.08f, 0.98f }, { 0.1f, 1.f, 0.3f }, light_parameters);

	// Set up navigation lights
	light_parameters = { .size = 0.035f, .angle = 100.f, .edge = 1.f, .intensity = 20.f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .range = 0.0001f, .fadeout = 0.1f, };
	nav_light_offset = add_point_light({ 5.08f, 0.18f, 1.33f }, light_parameters);

	light_parameters.color = { 0.f, 1.f, 0.f, 0.f };
	add_point_light({ -5.05f, 0.18f, 1.33f }, light_parameters);

	//Register event handler
	register_event_handler("air/engines/on", &tutorial_aircraft_plugin::engine);
	//Register axis handlers
	register_axis_handler("air/controls/brake", &tutorial_aircraft_plugin::brakes, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.5f, .vel = 1.f, .positions = 0 });
	register_axis_handler("air/lights/landing_lights", &tutorial_aircraft_plugin::landing_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("air/lights/nav_lights", &tutorial_aircraft_plugin::navigation_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("air/controls/aileron", &tutorial_aircraft_plugin::ailerons, { .minval = -1.f, .maxval = 1.f, .cenvel = 0.5f, .vel = 0.5f, .positions = 0 });
	register_axis_handler("air/controls/elevator", &tutorial_aircraft_plugin::elevator, { .minval = -1.f, .maxval = 1.f, .cenvel = 0.5f, .vel = 0.5f, .positions = 0 });

	//Return chassis parameters
	return {
		.mass = 1310,
		.com_offset = {0.0f, 0.0f, 0.2f},
	};
}

void tutorial_aircraft_plugin::initialize(bool reload)
{
	//Get JSBSim interface
	this->jsbsim = jsb();
	//Get geomob interface
	this->geom = get_geomob(0);
	//Get sound interface
	this->snd = sound();

	// Set initial fps position
	set_fps_camera_pos({ 0.f, 1.f, 1.4f });

	// Set initial values for the aircraft
	this->started = false;
	this->braking = 0;

	//Safety check for jsbsim pointer to avoid crash
	if (!this->jsbsim)
	{
		return;
	}

	// Set initial JSBSim properties (optional)
	//Turn off engine (JSBSim commands are normalized)
	this->jsbsim->operator()("propulsion/starter_cmd", 0);
	//Set initial right brake value to 0 
	this->jsbsim->operator()("fcs/right-brake-cmd-norm", 0);
	//Set initial left brake value to 0 
	this->jsbsim->operator()("fcs/left-brake-cmd-norm", 0);
	//Set initial throttle value to 0 
	this->jsbsim->operator()("fcs/throttle-cmd-norm[0]", 0);
	//Set initial mixture value to 0 
	this->jsbsim->operator()("fcs/mixture-cmd-norm[0]", 0);

	//Set initial pitch value for sound emitters
	this->snd->set_pitch(sources::rumble_exterior, 1);
	this->snd->set_pitch(sources::rumble_interior, 1);
	this->snd->set_pitch(sources::eng_exterior, 1);
	this->snd->set_pitch(sources::eng_interior, 1);
	this->snd->set_pitch(sources::prop_exterior, 1);
	this->snd->set_pitch(sources::prop_interior, 1);

	//Set initial gain value for sound emitters
	this->snd->set_pitch(sources::rumble_exterior, 1);
	this->snd->set_pitch(sources::rumble_interior, 1);
	this->snd->set_pitch(sources::eng_exterior, 1);
	this->snd->set_pitch(sources::eng_interior, 1);
	this->snd->set_pitch(sources::prop_exterior, 1);
	this->snd->set_pitch(sources::prop_interior, 1);
}

void tutorial_aircraft_plugin::update_frame(float dt)
{
	//Safety check for pointers to avoid crashes
	if (!this->jsbsim || !this->geom || !this->snd)
	{
		return;
	}

	//Store JSBSim data
	float propeller_rpm = static_cast<float>(this->jsbsim->operator()("propulsion/engine[0]/propeller-rpm"));
	float wheel_speed = static_cast<float>(this->jsbsim->operator()("gear/unit[0]/wheel-speed-fps"));
	float elevator_pos_rad = static_cast<float>(this->jsbsim->operator()("fcs/elevator-pos-rad"));


	// Update the orientation of different components
	//rotate_joint - incremental, bone rotates by given angle every time, this function is called (given angle is added to joint current orientation)
	this->geom->rotate_joint(bones::propeller, dt * (2 * static_cast<float>(PI)) * propeller_rpm, { 0.f, 1.f, 0.f });
	this->geom->rotate_joint(bones::wheel_front, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::wheel_right, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::wheel_left, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	//rotate_joint_orig - rotate to given to angle (from default orientation)
	this->geom->rotate_joint_orig(bones::elevator_right, elevator_pos_rad, { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::elevator_left, elevator_pos_rad, { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::aileron_right, static_cast<float>(this->jsbsim->operator()("fcs/right-aileron-pos-rad")), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::aileron_left, static_cast<float>(this->jsbsim->operator()("fcs/left-aileron-pos-rad")), { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::flap_right, static_cast<float>(this->jsbsim->operator()("fcs/flap-pos-rad")), { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::flap_left, static_cast<float>(this->jsbsim->operator()("fcs/flap-pos-rad")), { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::rudder, static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")), { 0.f, 0.f, -1.f });
	this->geom->rotate_joint_orig(bones::brake_pedal_left, static_cast<float>(this->jsbsim->operator()("fcs/left-brake-cmd-norm")) * 0.7f, { -1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::brake_pedal_right, static_cast<float>(this->jsbsim->operator()("fcs/right-brake-cmd-norm")) * 0.7f, { -1.f, 0.f, 0.f });

	//Rudder has 2 pedals for turning, therefore depending on the rudder position value from JSBSim, the left or right pedal will move
	if (static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) > 0)
	{
		this->geom->move_joint_orig(bones::rudder_pedal_left, { 0, static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) / 5, 0 });
	}
	else if (static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) < 0)
	{
		this->geom->move_joint_orig(bones::rudder_pedal_right, { 0, -static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) / 5,0 });
	}

	//Move throttle handle, depending on the throttle command value 
	this->geom->move_joint_orig(bones::throttle_handle, { 0, static_cast<float>(this->jsbsim->operator()("fcs/throttle-cmd-norm[0]")) / 7, 0 });

	//Note: in this case the pitch/roll handle will rotate without the need of coding, because it's binded with inputs in the model

	// Update the visibility of propeller meshes, based on the engine rpm (we want the propeller blades to not be visible, when propeller_rpm is bigger than 300, so that only the propeller blur is visible)
	this->geom->set_mesh_visible_id(meshes::prop_blur, propeller_rpm >= 200.f);
	this->geom->set_mesh_visible_id(meshes::blade_one, propeller_rpm <= 300.f);
	this->geom->set_mesh_visible_id(meshes::blade_two, propeller_rpm <= 300.f);
	this->geom->set_mesh_visible_id(meshes::blade_three, propeller_rpm <= 300.f);

	// When engine runs, control sounds, based on the camera position & sound effects, based on engine RPM
	if (propeller_rpm > 0)
	{
		//get_camera_mode returns 0, when the first person view, inside the plane is active
		//Interior
		if (get_camera_mode() == 0)
		{
			//Turn off the exterior emitters, when player camera is inside the plane
			this->snd->stop(sources::rumble_exterior);
			this->snd->stop(sources::eng_exterior);
			this->snd->stop(sources::prop_exterior);

			//Set pitch for interior engine emitter and clamp it between 0 and 1, in these cases, the pitch & gain values are calculated denepding on the engine rpm.
			this->snd->set_pitch(sources::eng_interior, clamp(1 + propeller_rpm / 4000.f, 1.f, 2.f));
			//Set gain for interior engine emitter and clamp it between 0 and 0.5
			this->snd->set_gain(sources::eng_interior, clamp(propeller_rpm / 5000.f, 0.f, 0.5f));

			//If there is no sound already playing on given emitter, play sound
			if (!this->snd->is_playing(sources::eng_interior))
			{
				//Play interior engine sound in a loop
				this->snd->play_loop(sources::eng_interior, sounds::eng_interior);
			}

			//Set gain for interior propeller emitter
			this->snd->set_gain(sources::prop_interior, clamp(propeller_rpm / 7000.f, 0.f, 0.5f));

			if (!this->snd->is_playing(sources::prop_interior))
			{
				//Play interior propeller sound in a loop
				this->snd->play_loop(sources::prop_interior, sounds::rumble);
			}

			//Set gain for interior rumble emitter
			this->snd->set_gain(sources::rumble_interior, clamp(propeller_rpm / 6000.f, 0.f, 1.5f));

			if (!this->snd->is_playing(sources::rumble_interior))
			{
				//Play rumble sound in a loop on interior emitter
				this->snd->play_loop(sources::rumble_interior, sounds::rumble);
			}
		}
		else
		{
			//Turn off the interior emitters, when camera is outside the plane
			this->snd->stop(sources::eng_interior);
			this->snd->stop(sources::prop_interior);
			this->snd->stop(sources::rumble_interior);

			//Set pitch for exterior engine emitter
			this->snd->set_pitch(sources::eng_exterior, clamp(1 + propeller_rpm / 1000.f, 1.f, 3.f));
			//Set gain for exterior engine emitter
			this->snd->set_gain(sources::eng_exterior, clamp(propeller_rpm / 450.f, 0.05f, 2.f));

			if (!this->snd->is_playing(sources::eng_exterior))
			{
				//Play exterior engine sound in a loop
				this->snd->play_loop(sources::eng_exterior, sounds::eng_exterior);
			}

			//Set gain for exterior propeller emitter
			this->snd->set_gain(sources::prop_exterior, clamp(propeller_rpm / 900.f, 0.f, 2.f));

			if (!this->snd->is_playing(sources::prop_exterior))
			{
				//Play exterior propeller sound in a loop
				this->snd->play_loop(sources::prop_exterior, sounds::eng_exterior);
			}

			//Set gain for exterior rumble emitter
			this->snd->set_gain(sources::rumble_exterior, clamp(propeller_rpm / 1200.f, 0.f, 2.f));

			if (!this->snd->is_playing(sources::rumble_exterior))
			{
				//Play rumble sound in a loop on exterior emitter
				this->snd->play_loop(sources::rumble_exterior, sounds::rumble);
			}
		}
	}
	else
	{
		//Stop all sounds playing on given emitters, when engine is turned off and propeller stops rotating
		this->snd->stop(sources::eng_exterior);
		this->snd->stop(sources::eng_interior);
		this->snd->stop(sources::prop_exterior);
		this->snd->stop(sources::prop_interior);
		this->snd->stop(sources::rumble_exterior);
		this->snd->stop(sources::rumble_interior);
	}

	//(Temporary) parking brake, when in idle state 
	if (!this->started && propeller_rpm < 5)
	{
		this->jsbsim->operator()("fcs/center-brake-cmd-norm", 1);
		this->jsbsim->operator()("fcs/left-brake-cmd-norm", 1);
		this->jsbsim->operator()("fcs/right-brake-cmd-norm", 1);
	}
	else if (this->braking < 0.1)
	{
		this->jsbsim->operator()("fcs/center-brake-cmd-norm", 0);
		this->jsbsim->operator()("fcs/left-brake-cmd-norm", 0);
		this->jsbsim->operator()("fcs/right-brake-cmd-norm", 0);
	}
}
