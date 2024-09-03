#pragma once

#include "tutorial_car_plugin.hpp"

//Static member must be defined outside class body (for debugging,it's better to define them to -1).
int wheels::FL_wheel = -1;
int wheels::FR_wheel = -1;
int wheels::RL_wheel = -1;
int wheels::RR_wheel = -1;
int bones::steer_wheel = -1;
int bones::speed_gauge = -1;
int bones::accel_pedal = -1;
int bones::brake_pedal = -1;
int bones::driver_door = -1;
uint lights_entity::rev_mask = -1;
uint lights_entity::brake_mask = -1;
uint lights_entity::turn_left_mask = -1;
uint lights_entity::turn_right_mask = -1;
uint lights_entity::main_light_offset = -1;
int sounds_entity::snd_starter = -1;
int sounds_entity::snd_eng_running = -1;
int sounds_entity::snd_eng_stop = -1;
int sounds_entity::src_eng_start_stop = -1;
int sounds_entity::src_eng_running = -1;

//namespace used, so that the registration will be limited in scope and only be visible within the translation unit, where the namespace is defined
namespace
{
	//It is neccesary to register our vehicle class as client, for that use macro IFC_REGISTER_CLIENT
	// parameter - derived client interface class
	IFC_REGISTER_CLIENT(tutorial_car_plugin);
}

//Used to start/stop the vehicle and play sounds associated with it
void tutorial_car_plugin::engine(int flags, uint code, uint channel, int handler_id)
{
	//Check, if the sounds pointer is not nullptr before attempting to use it, to avoid crashing due to a null pointer
	if (!this->sounds)
	{
		return;
	}

	//Check the actual "started" value and toggle it (using ternary operator)
	this->started = this->started == false ? true : false;
	//Write a fading message on screen using "fade" funciton
	fade(this->started == true ? "Engine start" : "Engine stop");

	//Based on the camera mode, set the sound gain value
	float sound_gain = this->current_camera_mode == 0 ? 0.5f : 1.f;

	//Use set_gain to set gain value on given emitter
	// 1.parameter - emitter
	// 2.parameter - gain value(this will affect all sounds emitted from this emitter)
	this->sounds->set_gain(sounds_entity::src_eng_start_stop, sound_gain);

	//Based on the camera mode, set the reference distance value
	float ref_distance = this->current_camera_mode == 0 ? 0.25f : 1.f;

	//Use set_ref_distance to set reference distance on given emitter (how far should the sounds be heard)
	// 1.parameter - emitter
	// 2.parameter - reference distance value(this will affect all sounds emitted from this emitter)
	this->sounds->set_ref_distance(sounds_entity::src_eng_start_stop, ref_distance);


	//Play sounds, depending on the "started" state
	if (this->started)
	{
		//function "play_sound" is used to play sound once, discarding older sounds
		// 1.parameter - emitter (source ID)
		// 2.parameter - sound (sound ID))
		this->sounds->play_sound(sounds_entity::src_eng_start_stop, sounds_entity::snd_starter);
	}
	else
	{
		//function "stop" discards all sounds playing on given emitter
		this->sounds->stop(sounds_entity::src_eng_running);
		this->sounds->play_sound(sounds_entity::src_eng_start_stop, sounds_entity::snd_eng_stop);

		//To not apply force on wheels, when the engine has stopped 
		wheel_force(wheels::FL_wheel, 0);
		wheel_force(wheels::FR_wheel, 0);
	}
}

//Function used to change direction and turn on/off reverse lights
void tutorial_car_plugin::reverse(int flags, uint code, uint channel, int handler_id)
{
	//change direction
	this->eng_dir = this->eng_dir >= 0 ? -1 : 1;
	fade(this->eng_dir > 0 ? "Forward" : "Reverse");
	//"light_mask" function is used to turn lights on/off
	// 1.parameter - light mask value - which bits/lights will be affected
	// 2.parameter - condition, when true, given lights will be affected ("turned on")
	// 3.parameter - offset, from which the light mask will affect bits/lights
	//if this->eng_dir < 0, rev_mask lights will turn on (bit mask will affect the specified lights) 
	light_mask(lights_entity::rev_mask, this->eng_dir < 0);
}

//Function used to toggle hand brake value
void tutorial_car_plugin::hand_brake(int flags, uint code, uint channel, int handler_id)
{
	//Bitwise operator XOR (^) used, to switch the value each time, this function is called.
	this->hand_brake_val = this->hand_brake_val == true ? false : true;
}

//Function used to turn on/off emergency lights
void tutorial_car_plugin::emergency_lights(int flags, uint code, uint channel, int handler_id)
{
	//Bitwise operator XOR (^) used, to switch the value each time, this function is called.
	this->emer ^= 1;
}

//Function used to store power input value
void tutorial_car_plugin::power(float val, uint code, uint channel, int handler_id)
{
	this->power_input = val;
}

//Function used to turn on/off passing lights
void tutorial_car_plugin::passing_lights(float val, uint code, uint channel, int handler_id)
{
	//if v===1, lights will turn on (light mask will affect the front and tail lights), else it will turn off
	light_mask(0xf, val == 1);
	//Note: this affects first 4 defined lights (because we didn't give an offset as 3. parameter).
	//In this case it will affect 2 front lights and 2 tail lights, because every light is represented as 1 bit
	//and as 1.parameter we used hexadecimal 0x0..0xf which works with 4 bits (0000....1111), we can also use 0x00..0xff which will work with first 8 bits
}

//Function used to turn on/off main lights
void tutorial_car_plugin::main_lights(float val, uint code, uint channel, int handler_id)
{
	//Another way to use light mask, is to give light offset (in this case main_light_offset) as 3. parameter, from this offset the light mask will affect next bits/lights
	//This way, there is no need for a mask, like with brake & turn lights...
	light_mask(0x3, val == 1, lights_entity::main_light_offset);
}

//Function used to toggle turn lights
void tutorial_car_plugin::turn_lights(float val, uint code, uint channel, int handler_id)
{
	if (val == 0)
	{
		//Turn off turn lights
		this->left_turn = this->right_turn = 0;
	}
	else if (val < 0)
	{
		//Turn on left turn light
		this->left_turn = 1;
		this->right_turn = 0;
	}
	else
	{
		//Turn on right turn light
		this->left_turn = 0;
		this->right_turn = 1;
	}
}

//Function used to open/close driver door
void tutorial_car_plugin::open_door(float val, uint code, uint channel, int handler_id)
{
	//Check, if the Geom pointer is not nullptr before attempting to use it, to avoid crashing due to a null pointer
	if (!this->geom)
	{
		return;
	}
	//function "rotate_joint_orig" is used to rotate joint by given angle and back to default position
	// 1.parameter - bone/joint ID
	// 2.parameter - vec rotation angle in radians
	// 3.parameter - rotation axis vector (must be normalized) - axis around which the bone rotates (in this case around Z axis) and the direction of rotation (-1...1)
	this->geom->rotate_joint_orig(bones::driver_door, (val * 1.5f), { 0.f, 0.f, -1.f });
}

// Initialize chassis parameters for our car (invoked once, to define the chassis for all instances of same type)
// parameter params - custom parameters from objdef
ot::chassis_params tutorial_car_plugin::init_chassis(const coid::charstr& params)
{
	//Define wheel setup
	ot::wheel wheel_params = {
		//Wheel parameters, that can be used:
	   .radius1 = 0.31515f,				//outer tire radius [m]
	   .width = 0.2f,					//tire width [m]
	   .suspension_max = 0.1f,			//max.movement up from default position [m]
	   .suspension_min = -0.04f,		//max.movement down from default position [m]
	   .suspension_stiffness = 50.f,	//suspension stiffness coefficient
	   .damping_compression = 0.4f,		//damping coefficient for suspension compression
	   .damping_relaxation = 0.12f,		//damping coefficient for suspension relaxation
	   .grip = 1.f,						//relative tire grip compared to an avg.tire, +-1
	   .slip_lateral_coef = 1.5f,		//lateral slip muliplier, relative to the computed longitudinal tire slip
	   .differential = true,			//true if the wheel is paired with another through a differential
	};

	//Add wheel with vertical suspension (along z axis) using "add_wheel" function
	// 1.parameter - wheel joint/bone pivot
	// 2.parameter - wheel structure
	// returns ID of the wheel
	wheels::FL_wheel = add_wheel("wheel_l0", wheel_params);
	wheels::FR_wheel = add_wheel("wheel_r0", wheel_params);
	wheels::RL_wheel = add_wheel("wheel_l1", wheel_params);
	wheels::RR_wheel = add_wheel("wheel_r1", wheel_params);

	//Get joint/bone ID for given bone name, using "get_joint_id" function
	// parameter - bone name
	// returns joint/bone id or -1 if doesn't exist
	bones::steer_wheel = get_joint_id("steering_wheel");
	bones::speed_gauge = get_joint_id("dial_speed");
	bones::accel_pedal = get_joint_id("pedal_accelerator");
	bones::brake_pedal = get_joint_id("pedal_brake");
	bones::driver_door = get_joint_id("door_l0");

	//Initialize light parameters
	//Light parameters, that can be defined:
	// float size - diameter of the light source (reflector or bulb)
	// float angle - light field angle [deg], ignored on point lights
	// float edge - soft edge coefficient, 0..1 portion of the light area along the edges where light fades to make it soft
	// float intensity - light intensity (can be left 0 and use the range instead)
	// float4 color - RGB color and near-infrared component of the light emitter
	// float range - desired range of light
	// float fadeout - time to fade after turning off
	 ot::light_params light_params = { .size = 0.05f, .angle = 120.f, .edge = 0.2f, .color = { 1.f, 1.f, 1.f, 0.f }, .range = 70.f, .fadeout = 0.05f };
	//If either intensity or range are specified alone, the other one is computed automatically.
	//If neither one is specified, the intensity is taken from the color value, otherwise the colors are normalized. If both intensity and range are specified, the light can have shorter range to avoid leaking outside of cockpits etc.
	//Note: intensity of light is affected not only by "intensity" parameter, but also by angle and range


	//Define circular spotlight source, using add_spot_light
	// 1.parameter offset - model-space offset relative to the bone or model pivot
	// 2.parameter dir - light direction
	// 3.parameter lp - light parameters
	// 4.parameter joint - joint name to attach the light to
	// returns light emitter id

	//Passing front lights
	uint pass_light_offset =
	add_spot_light({ -0.55f, 2.2f, 0.68f }, { 0.f, 1.f, 0.f }, light_params);
	add_spot_light({ 0.55f, 2.2f, 0.68f }, { 0.f, 1.f, 0.f }, light_params);

	//Tail lights
	light_params = { .size = 0.07f, .angle = 160.f, .edge = 0.8f, .color = { 1.f, 0.f, 0.f, 0.f } , .range = 150.f, .fadeout = 0.05f };
	uint tail_light_offset =
	add_spot_light({ -0.05f, -0.06f, 0.f }, { 0.f, 1.f, 0.f }, light_params, "tail_light_l0");
	add_spot_light({ 0.05f, -0.06f, 0.f }, { 0.f, 1.f, 0.f }, light_params, "tail_light_r0");

	//Brake lights
	light_params = { .size = 0.04f, .angle = 120.f, .edge = 0.8f, .color = { 1.f, 0.f, 0.f, 0.f}, .range = 100.f, .fadeout = 0.05f };
	uint brake_light_offset =
	add_spot_light({ -0.43f, -2.11f, 0.62f }, { 0.f, -1.f, 0.f }, light_params);
	add_spot_light({ 0.43f, -2.11f, 0.62f }, { 0.f, -1.f, 0.f }, light_params);

	//You can specify bit mask (brake_mask) using bit logic
	//Our 2 brake lights are defined as follows:
	// 01 - left brake light
	// 10 - right brake light
	// 11 - both brake lights
	//We want the bit mask (brake_mask) to affect both lights, therefore the given value will be "0b11" (in this case, the value is written in binary system, but it can also be written in decimal or hexadecimal system).
	//Also we want, that the mask starts affecting lights from the first brake light, therefore we have to "left shift" the bit mask by brake light offset
	lights_entity::brake_mask = 0b11 << brake_light_offset;

	//Reverse lights
	light_params = { .size = 0.04f, .angle = 120.f, .edge = 0.8f, .color = { 1.f, 1.f, 1.f, 0.f }, .range = 100.f, .fadeout = 0.05f };
	uint rev_light_offset =
	add_spot_light({ -0.5f, -2.11f, 0.715f }, { 0.f, -1.f, 0.f }, light_params);
	add_spot_light({ 0.5f, -2.11f, 0.715f }, { 0.f, -1.f, 0.f }, light_params);
	//We want both lights to shine, when we hit the reverse button (in this case, the value is written in decimal system)
	// 01 (binary) -> 1 (decimal) - left reverse light
	// 10 (binary) -> 2 (decimal) - right reverse light
	// 11 (binary) -> 3 (decimal) - both reverse lights
	lights_entity::rev_mask = 3 << rev_light_offset;

	//Main lights
	light_params = { .size = 0.05f, .angle = 110.f, .edge = 0.8f, .color = { 1.f, 1.f, 1.f, 0.f }, .range = 110.f, .fadeout = 0.05f };
	lights_entity::main_light_offset =
	add_spot_light({ -0.45f, 2.2f, 0.68f }, { 0.f, 1.f, 0.f }, light_params);
	add_spot_light({ 0.45f, 2.2f, 0.68f }, { 0.f, 1.f, 0.f }, light_params);
	//In this case, main lights will work with offset, instead of mask

	//Define point light source, using add_point_light
	// 1.parameter offset - model-space offset relative to the bone or model pivot
	// 2.parameter lp - light parameters
	// 3.parameter joint - joint name to attach the light to
	// returns light emitter id

	//Turn lights
	light_params = { .size = 0.1f, .edge = 0.8f, .intensity = 1.f, .color = { 0.4f, 0.1f, 0.f, 0.f}, .range = 0.004f, .fadeout = 0.f };
	uint turn_light_offset =
		add_point_light({ -0.71f, 2.23f, 0.62f }, light_params);
	add_point_light({ -0.66f, -2.11f, 0.715f }, light_params);
	add_point_light({ 0.71f, 2.23f, 0.62f }, light_params);
	add_point_light({ 0.66f, -2.11f, 0.715f }, light_params);
	//We want lights on the side of the car to glow, when we hit the corresponding left or right turn signal button (in this case, the value is written in hexadecimal system )
	// 0001 (binary) -> 0x1 (hexadecimal) - left front turn light
	// 0010 (binary) -> 0x2 (hexadecimal) - left rear turn light
	// 0011 (binary) -> 0x3 (hexadecimal) - left rear + left front (left side)
	// 0100 (binary) -> 0x4 (hexadecimal) - right front turn light
	// 0101 (binary) -> 0x5 (hexadecimal) - right front + left front (front side)
	// 0110 (binary) -> 0x6 (hexadecimal) - right front + left rear
	// 0111 (binary) -> 0x7 (hexadecimal) - right front + left rear + left front
	// 1000 (binary) -> 0x8 (hexadecimal) - right rear turn light
	// 1001 (binary) -> 0x9 (hexadecimal) - right rear + left front
	// 1010 (binary) -> 0xa (hexadecimal) - right rear + left rear (rear side)
	// 1011 (binary) -> 0xb (hexadecimal) - right rear + left rear + left front
	// 1100 (binary) -> 0xc (hexadecimal) - right rear + right front (right side)
	// 1101 (binary) -> 0xd (hexadecimal) - right rear + right front + left front
	// 1110 (binary) -> 0xe (hexadecimal) - right rear + right front + left rear
	// 1111 (binary) -> 0xf (hexadecimal) - all turn lights (emergency lights)
	lights_entity::turn_left_mask = 0x3 << turn_light_offset;
	//To create right turn light mask, add the number of previous left turn lights to the offset (or you can make another offset for right turn lights and use that....)
	lights_entity::turn_right_mask = 0x3 << (turn_light_offset + 2);

	//Load sound samples using "load_sound" function
	// parameter - string filename (audio file name, possibly with path)
	// returns sound ID
	sounds_entity::snd_starter = load_sound("Sounds/starter.ogg");
	sounds_entity::snd_eng_running = load_sound("Sounds/eng_running.ogg");
	sounds_entity::snd_eng_stop = load_sound("Sounds/eng_stop.ogg");

	//Create sound emitters that will be used by the vehicle
	//1. parameter - joint/bone, from which we want the sound to emit
	//2. parameter - sound type (-1 interior only, 0 universal, 1 exterior only)
	//3. parameter - reference distance (saturated volume distance)
	// returns emitter ID, which is later used in code
	sounds_entity::src_eng_start_stop = add_sound_emitter("exhaust_0_end", 0);
	sounds_entity::src_eng_running = add_sound_emitter("exhaust_0_end", 0);

	//Register action handlers, which are called, when binded input/object changes it's state.

	//Note: the hand brake and power are handled through script, to avoid bug, where parking brake cannot be engaged right after the power input was released 
	//(because by default, the centering is lower, and takes some time, until the value drops to 0)

	//Register event handlers, invoked on key or button press (for example a fire action)
	// 1.parameter name - path to input binding (config_file/group/action) where: 
	//						"config_file" is the name of .cfg file, in which we have the binding defined(these can be found in bin / defaults / iomap directory, where vehicle.cfg is for vehicles, air.cfg is for aircraft etc.)
	//						"group" is name of the group, to which it belongs
	//						"action" is the name of the bound action(this action is bound to shift + L))
	// 2.parameter handler - function, which is invoked whenever given event occurs or the tracked value changes as the result of the button state. 
	// 3.parameter handler_id - optional extra data for the handler
	// 4.parameter group - activation group where the action is assigned (can be enabled/disabled together)
	// returns slot id or -1 on fail
	register_event_handler("vehicle/engine/on", &tutorial_car_plugin::engine);
	register_event_handler("vehicle/engine/reverse", &tutorial_car_plugin::reverse);
	register_event_handler("vehicle/controls/hand_brake", &tutorial_car_plugin::hand_brake);
	register_event_handler("vehicle/lights/emergency", &tutorial_car_plugin::emergency_lights);
	
	//Register value handlers, they are receiving a tracked value which is by default in -1..1 range
	// 1.parameter name - hierarchic action name (config_file/group/action)
	// 2.parameter handler - handler for changed value (our callback function)
	// 3.parameter ramp - value limiter parameters
	// 4.parameter handler_id - optional extra data for the handler
	// 5.parameter def_val - optional default action value
	// 6.parameter group - activation group where the action is assigned (can be enabled/disabled together)
	// returns slot id or -1 on fail
	register_axis_handler("vehicle/controls/power", &tutorial_car_plugin::power, { .minval = 0.f, .maxval = 1.f, .cenvel = 100.f, .vel = 10.f });
	register_axis_handler("vehicle/lights/passing", &tutorial_car_plugin::passing_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/lights/main", &tutorial_car_plugin::main_lights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/lights/turn", &tutorial_car_plugin::turn_lights, { .minval = -1.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/controls/open", &tutorial_car_plugin::open_door, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 0.6f });

	//Warning: "register_axis" events are called with value 0 when the script loads or reloads. 
	//To prevent issues with functionality that toggles values, you can, for example, add a check to ignore these initial events by adding condition "if (v === 0) { return; }"

	//Ramp parameters, that can be used:
	// float minval - minimum value to clamp to, should be >= -1 for compatibility with joystick
	// float maxval - maximum value to clamp to, should be <= +1 for compatibility with joystick
	// float cenvel - centering speed per second (speed after releasing the button), 0 for freeze on button release
	// float vel - max rate of value change (saturated) per second
	// float acc - max rate of initial velocity change per second
	// uint8 positions - number of positions between min and max values including the endpoints, 0 = no stepping mode
	// uint8 extra_channels - optional number of extra channels (multiple engines etc)

	//Return parameters
	ot::chassis_params chassis_params = {
		//Chassis parameters that can be used:
		.mass = 1120.f,					//vehicle mass [kg]
		.com_offset = {0.f, 0.f, 0.3f},	//center of mass offset
		.clearance = 0.f,					//clearance from ground, default wheel radius
		.bumper_clearance = 0.f,			//clearance in front and back (train bumpers)
		.steering = {
			//Steering parameters that can be used:
			.steering_thr = 50.f,			//speed [km/h] at which the steering speed is reduced by 60%
			.centering_thr = 20.f,			//speed [km/h] when the centering acts at 60% already
		}
	};

	return chassis_params;
}

//Invoked for each new instance of the vehicle (including the first one),it is used to define per-instance parameters.
// parameter reload - true if object is being reloaded
void tutorial_car_plugin::init_vehicle(bool reload)
{
	//Initialize variables for your instance (use "this" keyword for changes to affect only current instance)
	this->started = false;
	this->emer = false;
	this->hand_brake_val = true;
	this->eng_dir = 1;
	this->left_turn = 0;
	this->right_turn = 0;
	this->current_camera_mode = 0;
	this->previous_cam_mode = 0;
	this->braking_power = 0.f;
	this->power_input = 0.f;
	this->time = 0.0;

	//Get geomob interface
	this->geom = get_geomob(0);
	//Get sound interface
	this->sounds = sound();

	//Set FPS camera position


	//Function "set_fps_camera_pos" set's the camera position, when FPS mode is active
	// 1. parameter - model-space position from the pivot (when the joint id as 2. parameter is not specified, otherwise it works as offset, relative to the joint position)
	// 2. parameter - bone/joint id (optional), to set fps camera position to joint position 
	// 3. parameter - joint rotation mode (if the offset should be set based on joint orientation or not, where 0 - Enabled, 1 - Disabled )
	set_fps_camera_pos({ -0.4f, 0.16f, 1.3f });

	//Example of using bone, to set the FPS camera position
	//this.get_fps_camera_pos({0.f, 0.f, 0.f}, this->get_joint_id("fps_cam_bone"), 1);

	//set initial sound values
	this->sounds->set_pitch(sounds_entity::src_eng_start_stop, 1);
	this->sounds->set_pitch(sounds_entity::src_eng_running, 1);
	this->sounds->set_gain(sounds_entity::src_eng_start_stop, 1);
	this->sounds->set_gain(sounds_entity::src_eng_running, 1);
	this->sounds->set_ref_distance(sounds_entity::src_eng_start_stop, 1);
	this->sounds->set_ref_distance(sounds_entity::src_eng_running, 1);
}

//Update model instance each frame
void tutorial_car_plugin::update_frame(float dt, float engine, float brake, float steering, float parking)
{
	//Check, if the Geom pointer is not nullptr before attempting to use it, to avoid crashing due to a null pointer
	if (!this->geom || !this->sounds)
	{
		return;
	}

	// Rotate brake pedal
	float brake_angle = brake * 0.4f;
	this->geom->rotate_joint_orig(bones::brake_pedal, brake_angle, { 1, 0, 0 });
	
	//move_joint_orig function is used to move joint to given position
	//1.parameter - joint you want to move
	//2.parameter - movement axis and direction
	this->geom->move_joint_orig(bones::accel_pedal, { 0, (-this->power_input * 0.02f), (-this->power_input * 0.02f) });

	//Get current ground speed, using "speed" function (returns current speed in m/s, multiply with 3.6 to get km/h or 2.23693629 to get mi/h)
	float kmh = glm::abs(speed() * 3.6f); //We want to get the absolute value

	//Get_camera_mode returns current camera mode (0 - FPS camera mode, 1 - TPS camera mode, 2 - TPS follow camera mod )
	this->current_camera_mode = this->get_camera_mode();

	//To not set reference distance every frame, check if the camera mode has changed
	if (this->previous_cam_mode != this->current_camera_mode)
	{
		float ref_distance;
		//Choose reference distance, based on current camera mode
		if (this->current_camera_mode == 0)
		{
			ref_distance = 0.25f;
		}
		else
		{
			ref_distance = 1.f;
		}

		//Set reference distance
		this->sounds->set_ref_distance(sounds_entity::src_eng_running, ref_distance);

		//set this.previous_cam_mode to current camera mode
		this->previous_cam_mode = this->current_camera_mode;
	}

	//Calculate force, but only if the car has started
	if (this->started == true)
	{
		//Calculate force value, depending on the direction
		float redux = this->eng_dir >= 0 ? 0.2f : 0.6f;
		//glm library can be used
		float eng_power = ENGINE_FORCE * this->power_input;
		float force = (kmh >= 0) == (this->eng_dir >= 0)
			? eng_power / (redux * kmh + 1)
			: eng_power;
		//simulate resistance
		force -= FORCE_LOSS;
		//Make sure, that force can not be negative
		force = glm::max(0.f, glm::min(force, eng_power));
		//Calculate force and direction, which will be used to add force to wheels
		force *= this->eng_dir;

		//Move only when there is no sound playing on given emitter (to not be able to move when car is starting, but after the starter sound ends)
		//"is_playing" function checks, if there is sound playing on given emitter
		if (this->sounds->is_playing(sounds_entity::src_eng_start_stop))
		{
			force = 0;
		}
		else
		{
			//Calculate and set volume pitch and gain for emitter
			//"max_rpm" function returns rpm of the fastest revolving wheel
			float rpm = this->max_rpm();
			float speed_modulation = kmh / 40 + glm::abs(rpm / 200);
			float pitch_gain_factor = rpm > 0 ? glm::floor(speed_modulation) : 0;
			float pitch_gain = speed_modulation + (0.5f * pitch_gain_factor) - pitch_gain_factor;

			//Use "set_pitch" function to set pitch value on given emitter
			// 1.parameter - emitter
			// 2.parameter - pitch value (this will affect all sounds emitted from this emitter)
			this->sounds->set_pitch(sounds_entity::src_eng_running, (0.5f * pitch_gain) + 1.f);

			//et gain value on given emitter
			this->sounds->set_gain(sounds_entity::src_eng_running, (0.25f * pitch_gain) + 0.5f);

			//"play_loop" function is used to play sound in loop, breaking other sounds
			// 1.parameter - emitter (source ID)
			// 2.parameter - sound (sound ID))
			if (!this->sounds->is_looping(sounds_entity::src_eng_running))
			{
				this->sounds->play_loop(sounds_entity::src_eng_running, sounds_entity::snd_eng_running);
			}
		}

		//Release the parking brake, when accelerating, while started
		if (this->hand_brake_val == true && force > 0)
		{
			this->hand_brake_val = false;
		}

		//Use wheel_force function to apply propelling force on wheel and move the car
		//1.parameter - wheel, you want to affect (takes the wheel ID, in this case, the car has front-wheel drive)
		//2.parameter - force, you want to exert on the wheel hub in forward/backward  direction (in Newtons)
		this->wheel_force(wheels::FL_wheel, force);
		this->wheel_force(wheels::FR_wheel, force);
	}

	//Define steering sensitivity
	steering *= 0.5f;
	//Steer wheels by setting angle, using "steer" function
	// 1.parameter - wheel ID
	// 2.parameter - angle in radians to steer the wheel
	steer(wheels::FL_wheel, steering);
	steer(wheels::FR_wheel, steering);

	//Apply brake light mask (brake_mask) on brake lights, when brake value is bigger than 0
	//Note: add this code before adding rolling friction to brakes.
	light_mask(lights_entity::brake_mask, brake > 0);

	//Set the braking value, which will be applied on wheels, based on the type of brake 
	if (this->hand_brake_val == true)
	{
		// Apply full braking force when the parking brake is engaged 
		this->braking_power = BRAKE_FORCE;
	}
	else if (brake != 0)
	{
		// Apply proportional braking force when the regular brake is engaged
		this->braking_power = brake * BRAKE_FORCE;
	}
	else
	{
		this->braking_power = 0;
	}

	//Add resistance
	this->braking_power += 200;
	//Use "wheel_brake" function to apply braking force on given wheels
	// 1.parameter - wheel ID
	// 2.parameter - braking force, you want to exert on the wheel hub (in Newtons)
	wheel_brake(-1, this->braking_power);

	if (this->left_turn == 1 || this->right_turn == 1 || this->emer == true)
	{
		//When turn/emergency lights are turned on, calculate blinking time (in this case between 0 and 1) for turn signal lights
		this->time += dt;
		this->time = glm::mod(this->time, 1.0); // Use glm::mod to perform the modulo operation
		//For turn lights blinking effect
		int blink = this->time > 0.47f ? 1 : 0;
		//Apply light mask for turn lights, depending on which action was handled (left turn lights, right turn lights or all turn lights (emergency))
		light_mask(lights_entity::turn_left_mask, blink && (this->left_turn || this->emer));
		light_mask(lights_entity::turn_right_mask, blink && (this->right_turn || this->emer));
	}
	else
	{
		//To turn off the active turn lights
		light_mask(lights_entity::turn_left_mask, false);
		light_mask(lights_entity::turn_right_mask, false);
		this->time = 0;
	}

	//Animate vehicle components
	if (kmh > SPEED_GAUGE_MIN)
	{
		//Rotate speedometer needle
		this->geom->rotate_joint_orig(bones::speed_gauge, (kmh - SPEED_GAUGE_MIN) * RAD_PER_KMH, { 0.f, 1.f, 0.f });
	}
	//Rotate steering wheel
	this->geom->rotate_joint_orig(bones::steer_wheel, 10.5f * steering, { 0.f, 0.f, 1.f });
	//For animating wheels is better to use "animate_wheels" function, which simplifies the animation
	animate_wheels();
}

