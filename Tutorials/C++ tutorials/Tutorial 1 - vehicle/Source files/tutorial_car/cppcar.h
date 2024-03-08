#pragma once
//Include necessary files 
//vehicle_script.h is needed for our car simulation, to provide important tools and features
#include<ot/vehicle_script.h>
//For now, script_module is needed to registering client
#include<ot/script_module.h>

//These variables can be global and initialized, as they are only used in statements and calculations, they will not be changed later, therefore they are marked as const.
const uint MAX_KMH = 200;
const float SPEED_GAUGE_MIN = 10.0f;
const float RAD_PER_KMH = 0.018325957f;
const float ENGINE_FORCE = 25000.0f;
const float BRAKE_FORCE = 5000.0f;
const float FORCE_LOSS = ENGINE_FORCE / (0.2f * MAX_KMH + 1.f);

//Declare structs
struct bones
{
	/*Use static keyword, so that the variable can be shared across all instances (familiar to creating global variable)
	without it, the variables would be defined only for the 1. instance/vehicle, other instances would have variables set to 0, because they are defined in init_chassis(), which is called only the first time.  
	Use only for variables, that will be shared across all instances/vehicles and will not be changed later (should be defined in init_chassis) */
	static int FL_wheel;
	static int FR_wheel;
	static int RL_wheel;
	static int RR_wheel;
	static int steer_wheel;
	static int speed_gauge;
	static int accel_pedal;
	static int brake_pedal;
	static int driver_door;
};

struct lights_entity
{
	static uint rev_mask;
	static uint brake_mask;
	static uint turn_left_mask;
	static uint turn_right_mask;
	static uint main_light_offset;
};

struct sound_entity {
	static int snd_starter;
	static int snd_eng_running;
	static int snd_eng_OFF;

	static int src_ON_OFF;
	static int src_eng_running;
};

//Create class which inherits from vehicle_script, so that it can implement specific functionalities 
class tutorial_car : public ot::vehicle_script
{
	//Override virtual functions from vehicle_script
	//These are basic functions needed for creating an vehicle mod
	ot::chassis_params init_chassis(const coid::charstr& params) override;
	void init_vehicle(bool reload) override;
	void update_frame(float dt, float engine, float brake, float steering, float parking) override;

	//Declare additional functions
	//these will be used as callback functions (and given as parameter in register action handlers), 
	//therefore they need to align with the parameters specified in the callback function predefinition, which are:
	//for event handlers - int flags, uint code, uint channel, int handler_id
	//for axis handlers - float val, uint code, uint channel, int handler_id 
	void engine(int flags, uint code, uint channel, int handler_id);
	void reverse(int flags, uint code, uint channel, int handler_id);
	void emergency_lights(int flags, uint code, uint channel, int handler_id);
	void passing_lights(float val, uint code, uint channel, int handler_id);
	void turn_lights(float val, uint code, uint channel, int handler_id);
	void main_lights(float val, uint code, uint channel, int handler_id);
	void open_door(float val, uint code, uint channel, int handler_id);

	//Declare variables
	bool started;
	bool emer;
	int eng_dir;
	int left_turn;
	int right_turn;
	double time;

	//Spot and point light parameters
	ot::light_params light_params = ot::light_params(); // Initializes all fields to 0

	//Geom and Sounds are smart pointers, pointing to geomob and sound groups, they are used for managing reference-counted objects 
	iref<ot::geomob> geom = nullptr;
	iref<ot::sndgrp> sounds = nullptr;
};