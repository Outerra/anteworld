#include "DA40-NG.h"

// Definition of bone static members
int bones::propeller;
int bones::wheel_front;
int bones::wheel_right;
int bones::wheel_left;
int bones::elevator_right;
int bones::elevator_left;
int bones::rudder;
int bones::aileron_right;
int bones::aileron_left;
int bones::rudder_pedal_right;
int bones::rudder_pedal_left;
int bones::brake_pedal_left;
int bones::brake_pedal_right;
int bones::throttle_handle;

// Definition of mesh static members
int meshes::prop_blur;
int meshes::blade_one;
int meshes::blade_two;
int meshes::blade_three;

// Definition of sound static members
int sounds::rumble;
int sounds::eng_exterior;
int sounds::eng_interior;

// Definition of sound source static members
int sources::exterior;
int sources::interior;
int sources::rumble;

// Register the DA40 class as a client
namespace
{
	IFC_REGISTER_CLIENT(DA40);
};

//For now, it is also needed to register our mod, for that you have to create class which is derived from ot::script_module.
class register_mod : public ot::script_module
{
};

namespace dummy
{
	IFC_REGISTER_CLIENT(register_mod);
};

// Utility function to clamp a value within a specified range
float DA40::clamp(float val, float minval, float maxval)
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
void DA40::engine(int flags, uint code, uint channel, int handler_id)
{
	//safety checks for jsbsim pointer to avoid crashes
	if (!this->jsbsim) 
	{
		return;
	}

	// Toggle the engine state
	this->started ^= 1;

	// Start or stop the engine based on the state
	if (this->started == 1)
	{
		this->jsbsim->operator()("propulsion/starter_cmd", 1);
		this->jsbsim->operator()("propulsion/magneto_cmd", 1);
	}
	else
	{
		this->jsbsim->operator()("propulsion/starter_cmd", 0);
		this->jsbsim->operator()("propulsion/magneto_cmd", 0);
	}
}

// Function to handle landing lights events
void DA40::landing_lights(float val, uint code, uint channel, int handler_id)
{
	// Toggle landing lights based on the input value
	light_mask(0x3, val > 0);
};

// Function to handle navigation lights events
void DA40::navigation_lights(float val, uint code, uint channel, int handler_id)
{
	// Toggle navigation lights based on the input value
	light_mask(0x3, val > 0, nav_light_offset);
};

// Initialize chassis parameters for the DA40
ot::chassis_params DA40::init_chassis(const coid::charstr& params)
{
	// Define bone members
	bones::propeller = get_joint_id("propel");
	bones::wheel_front = get_joint_id("front_wheel");
	bones::wheel_right = get_joint_id("right_wheel");
	bones::wheel_left = get_joint_id("left_wheel");
	bones::elevator_right = get_joint_id("elevator_right");
	bones::elevator_left = get_joint_id("elevator_left");
	bones::rudder = get_joint_id("rudder");
	bones::aileron_right = get_joint_id("flap_right");
	bones::aileron_left = get_joint_id("flap_left");
	bones::rudder_pedal_right = get_joint_id("rudder_pedal_right");
	bones::rudder_pedal_left = get_joint_id("rudder_pedal_left");
	bones::brake_pedal_left = get_joint_id("brake_pedal_left");
	bones::brake_pedal_right = get_joint_id("brake_pedal_right");
	bones::throttle_handle = get_joint_id("throttle_lever");

	// Define mesh members 
	meshes::prop_blur = get_mesh_id("propel_blur");
	meshes::blade_one = get_mesh_id("main_blade_01");
	meshes::blade_two = get_mesh_id("main_blade_02");
	meshes::blade_three = get_mesh_id("main_blade_03");

	// Define sound members 
	sounds::rumble = load_sound("sounds/engine/engn1.ogg");
	sounds::eng_exterior = load_sound("sounds/engine/engn1_out.ogg");
	sounds::eng_interior = load_sound("sounds/engine/engn1_inn.ogg");
	
	// Define sound source members 
	sources::exterior = add_sound_emitter_id(bones::propeller, 1, 5.0f);
	sources::interior = add_sound_emitter_id(bones::propeller, -1, 1.2f);
	sources::rumble = add_sound_emitter_id(bones::propeller, 0, 2.0f);

	// Set up landing lights
	light_params = { .size = 0.1f, .angle = 100.f, .edge = 0.25f, .intensity = 5.f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .fadeout = 0.05f,  };
	add_spot_light({ 4.5f, 1.08f, 0.98f }, {-0.1f, 1.f, 0.3f}, light_params);
	add_spot_light({ -4.5f, 1.08f, 0.98f }, {0.1f, 1.f, 0.3f}, light_params);

	// Set up navigation lights
	light_params = { .size = 0.035f, .angle = 100.f, .edge = 1.f, .intensity = 20.f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .range = 0.0001f, .fadeout = 0.1f, };
	nav_light_offset = add_point_light({ 5.08f, 0.18f, 1.33f }, light_params);

	light_params.color = { 0.f, 1.f, 0.f, 0.f };
	add_point_light({-5.05f, 0.18f, 1.33f}, light_params);

	// Register handlers
	register_axis_handler("air/lights/landing_lights", &DA40::landing_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("air/lights/nav_lights", &DA40::navigation_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_event_handler("air/engines/on", &DA40::engine);

	// Return chassis parameters
	return {
		.mass = 1310,
		.com_offset = {0.0f, 0.0f, 0.2f},
	};
}

// Initialize the DA40 aircraft
void DA40::initialize(bool reload)
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

	//safety check for jsbsim pointer to avoid crashes
	if (!this->jsbsim)
	{
		return;
	}

	// Set initial JSBSim properties
	this->jsbsim->operator()("propulsion/starter_cmd", 0);
	this->jsbsim->operator()("fcs/elevator-pos-rad", 0);
	this->jsbsim->operator()("fcs/rudder-pos-rad", 0);
	this->jsbsim->operator()("fcs/right-aileron-pos-rad", 0);
	this->jsbsim->operator()("fcs/left-aileron-pos-rad", 0);
	this->jsbsim->operator()("fcs/right-brake-cmd-norm", 0);
	this->jsbsim->operator()("fcs/left-brake-cmd-norm", 0);
	this->jsbsim->operator()("fcs/throttle-cmd-norm[0]", 0);
	this->jsbsim->operator()("fcs/mixture-cmd-norm[0]", 0);
}

//Update model instance each frame
void DA40::update_frame(float dt)
{
	//safety check for pointers to avoid crashes
	if (!this->jsbsim || !this->geom || !this->snd)
	{
		return;
	}

	//Store values based on JSBSim data
	float eng_rpm = static_cast<float>(this->jsbsim->operator()("propulsion/engine[0]/engine-rpm"));
	float wheel_speed = static_cast<float>(this->jsbsim->operator()("gear/unit[0]/wheel-speed-fps"));
	float elev_pos_rad = static_cast<float>(this->jsbsim->operator()("fcs/elevator-pos-rad"));

	// Update the orientation of different components
	this->geom->rotate_joint(bones::propeller, dt * (2 * static_cast<float>(PI)) * eng_rpm, { 0.f, 1.f, 0.f });
	this->geom->rotate_joint(bones::wheel_front, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::wheel_right, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::wheel_left, dt * static_cast<float>(PI) * (wheel_speed / 5), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::elevator_right, elev_pos_rad, { 1.f, 0.f, 0.f });
	this->geom->rotate_joint(bones::elevator_left, elev_pos_rad, { 1.f, 0.f, 0.f });

	this->geom->rotate_joint_orig(bones::rudder, static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")), { 0.f, 0.f, -1.f });
	this->geom->rotate_joint_orig(bones::aileron_right, static_cast<float>(this->jsbsim->operator()("fcs/right-aileron-pos-rad")), { -1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::aileron_left, static_cast<float>(this->jsbsim->operator()("fcs/left-aileron-pos-rad")), { 1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::brake_pedal_left, static_cast<float>(this->jsbsim->operator()("fcs/left-brake-cmd-norm")) * 0.7f, { -1.f, 0.f, 0.f });
	this->geom->rotate_joint_orig(bones::brake_pedal_right, static_cast<float>(this->jsbsim->operator()("fcs/right-brake-cmd-norm")) * 0.7f, { -1.f, 0.f, 0.f });
   
	if (static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) > 0)
	{
		this->geom->move_joint_orig(bones::rudder_pedal_left, { 0, static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) / 5, 0 });
	}
	else if (static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) < 0)
	{
		this->geom->move_joint_orig(bones::rudder_pedal_right, { 0, - static_cast<float>(this->jsbsim->operator()("fcs/rudder-pos-rad")) / 5,0 });
	}

	this->geom->move_joint_orig(bones::throttle_handle, { 0, static_cast<float>(this->jsbsim->operator()("fcs/throttle-cmd-norm[0]")) / 7, 0 });

	// Update the visibility of propeller meshes, based on the engine rpm
	this->geom->set_mesh_visible_id(meshes::prop_blur, eng_rpm > 200.f);
	this->geom->set_mesh_visible_id(meshes::blade_one, eng_rpm < 300.f);
	this->geom->set_mesh_visible_id(meshes::blade_two, eng_rpm < 300.f);
	this->geom->set_mesh_visible_id(meshes::blade_three, eng_rpm < 300.f);

	// When engine runs, control sounds, based on the camera position & sound effects, based on engine RPM
	if (eng_rpm > 0)
	{
		if (get_camera_mode() == 0)
		{
			this->snd->stop(sources::exterior);

			this->snd->set_pitch(sources::interior, clamp(eng_rpm / 4000.f, 0.f, 0.6f));
			this->snd->set_gain(sources::interior, clamp(eng_rpm / 4000.f, 0.f, 0.6f));

			if (this->snd->is_playing(sources::interior))
			{
				this->snd->play_loop(sources::interior, sounds::eng_interior);
			}

			this->snd->set_pitch(sources::rumble, clamp(eng_rpm / 1200, 0.f, 2.f));
			this->snd->set_gain(sources::rumble, clamp(eng_rpm / 2000.f, 0.f, 1.5f));

			if (this->snd->is_playing(sources::rumble))
			{
				this->snd->play_loop(sources::rumble, sounds::rumble);
			}
		}
		else
		{
			this->snd->stop(sources::interior);

			this->snd->set_pitch(sources::exterior, clamp(eng_rpm / 4000.f, 0.f, 0.6f));
			this->snd->set_gain(sources::exterior, clamp(eng_rpm / 4000.f, 0.f, 0.6f));

			if (this->snd->is_playing(sources::exterior))
			{
				this->snd->play_loop(sources::exterior, sounds::eng_exterior);
			}

			this->snd->set_pitch(sources::rumble, clamp(eng_rpm / 1200, 0.f, 2.f));
			this->snd->set_gain(sources::rumble, clamp(eng_rpm / 1300.f, 0.f, 2.f));

			if (this->snd->is_playing(sources::rumble))
			{
				this->snd->play_loop(sources::rumble, sounds::rumble);
			}
		}
	} // Stop sounds when the engine is not running
	else
	{
		this->snd->stop(sources::exterior);
		this->snd->stop(sources::interior);
		this->snd->stop(sources::rumble);
	}
}

