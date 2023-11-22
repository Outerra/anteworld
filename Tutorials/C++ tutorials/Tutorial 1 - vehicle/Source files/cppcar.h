#pragma once
//Include necessary files 
//vehicle_script.h is needed for our car simulation, to provide important tools and features
#include<ot/vehicle_script.h>
//For now, script_module is needed to registering client, might be changed later.... 
#include<ot/script_module.h>

//These variables can be global and initialized, as they are only used in statements and calculations, they will not be changed later, therefore they are marked as const.
const uint MaxKmh = 200;
const float SpeedGaugeMin = 10.0f;
const float RadPerKmh = 0.018325957f;
const float EngineForce = 25000.0f;
const float BrakeForce = 5000.0f;
const float ForceLoss = EngineForce / (0.2f * MaxKmh + 1.f);

//Create class which inherits from vehicle_script, so that it can implement specific functionalities 
class TutorialCar : public ot::vehicle_script
{
	//Override virtual functions from vehicle_script
	//These are basic functions needed for creating an vehicle plugin
	ot::chassis_params init_chassis(const coid::charstr& params) override;
	void init_vehicle(bool reload) override;
	void update_frame(float dt, float engine, float brake, float steering, float parking) override;

	//Declare additional functions
	//these will be used as callback functions (and given as parameter in register action handlers), 
	//therefore they need to align with the parameters specified in the callback function predefinition, which are:
	//for event handlers - int flags, uint code, uint channel, int handler_id
	//for axis handlers - float val, uint code, uint channel, int handler_id 
	void Engine(int flags, uint code, uint channel, int handler_id);
	void Reverse(int flags, uint code, uint channel, int handler_id);
	void EmergencyLights(int flags, uint code, uint channel, int handler_id);
	void PassingLights(float val, uint code, uint channel, int handler_id);
	void TurnLights(float val, uint code, uint channel, int handler_id);
	void MainLights(float val, uint code, uint channel, int handler_id);
	void OpenDoor(float val, uint code, uint channel, int handler_id);

	//Declare variables
	bool Started;
	bool Emer;
	int EngDir;
	int Lturn;
	int Rturn;
	double Time;

	//Use static keyword, so that the variable can be shared across all instances (familiar to creating global variable)
	//without it, the variables would be defined only for the 1. instance/vehicle, other instances would have variables set to 0, because they are defined in init_chassis(), which is called only the first time.  
	//Use only for variables, that will be shared across all instances/vehicles and will not be changed later (should be defined in init_chassis) 
	static uint RevMask;
	static uint BrakeMask;
	static uint TurnLeftMask;
	static uint TurnRightMask;
	static uint MainLightOffset;
	static int FLwheel;
	static int FRwheel;
	static int RLwheel;
	static int RRwheel;
	static int SteerWheel;
	static int SpeedGauge;
	static int AccelPedal;
	static int BrakePedal;
	static int DriverDoor;
	static int SndStarter;
	static int SndEngON;
	static int SndEngOFF;
	static int SrcOnOff;
	static int SrcEngOn;

	//Spot and point light parameters
	ot::light_params LightParams = ot::light_params(); // Initializes all fields to 0

	//Geom and Sounds are smart pointers, pointing to geomob and sound groups, they are used for managing reference-counted objects 
	iref<ot::geomob> Geom = nullptr;
	iref<ot::sndgrp> Sounds = nullptr;
};