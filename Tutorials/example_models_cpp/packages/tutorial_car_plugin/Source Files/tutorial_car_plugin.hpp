#pragma once
//Welcome to the first tutorial on C++ programming in Outerra.
//We recommend reviewing the Outerra/anteworld tutorial on Github ( https://github.com/Outerra/anteworld/wiki/Tutorial-1-%E2%80%90-car-(Cpp) ) before proceeding, as it provides detailed information.

//Warning: Mods working with models (in this case, we are working with car model), need to have the files located under "packages" folder (example: Outerra World Sandbox\mods\example_models_cpp\packages\tutorial_car_cpp)

//Include necessary files 
//vehicle_script.h is needed, so we can implement essential functionalities for our vehicle
#include <ot/vehicle_script.h>

//Initialize global variables and mark them as const, as they won't be changed later (will be used only in statements and calculations).
const uint MAX_KMH = 200;
const float SPEED_GAUGE_MIN = 10.0f;
const float RAD_PER_KMH = 0.018325957f;
const float ENGINE_FORCE = 25000.0f;
const float BRAKE_FORCE = 5000.0f;
const float FORCE_LOSS = ENGINE_FORCE / (0.2f * MAX_KMH + 1.f);

//Declare static variables, which will represent different parts and features of our aircraft.
//It is preferable to group related variables within structs, as it enhances modularity and creates structured and understandable code.
struct wheels
{
	//Use static keyword, so that the variable can be shared across all instances
	static int FL_wheel;
	static int FR_wheel;
	static int RL_wheel;
	static int RR_wheel;
};

struct bones
{
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

struct sounds_entity
{
	static int snd_starter;
	static int snd_eng_running;
	static int snd_eng_stop;

	static int src_eng_start_stop;
	static int src_eng_running;
};

//Create class which inherits from vehicle_script, so that it can implement specific functionalities 
class tutorial_car_plugin : public ot::vehicle_script
{
	//Override virtual functions from vehicle_script
	//These are basic functions needed for creating an vehicle
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
	void hand_brake(int flags, uint code, uint channel, int handler_id);
	void emergency_lights(int flags, uint code, uint channel, int handler_id);
	void power(float val, uint code, uint channel, int handler_id);
	void passing_lights(float val, uint code, uint channel, int handler_id);
	void turn_lights(float val, uint code, uint channel, int handler_id);
	void main_lights(float val, uint code, uint channel, int handler_id);
	void open_door(float val, uint code, uint channel, int handler_id);

	//Declare variables
	bool started;
	bool emer;
	bool hand_brake_val;
	int eng_dir;
	int left_turn;
	int right_turn;
	int current_camera_mode;
	int previous_cam_mode;
	float braking_power;
	float power_input;
	double time;

	//Geom and Sounds are smart pointers, pointing to geomob and sound groups, they are used for managing reference-counted objects 
	iref<ot::geomob> geom = nullptr;
	iref<ot::sndgrp> sounds = nullptr;
};
