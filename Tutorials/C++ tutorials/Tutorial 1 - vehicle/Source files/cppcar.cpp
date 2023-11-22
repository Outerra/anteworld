//Include header file
#include "cppcar.h"

//Static member must be defined outside class body (it doesn't need to be equal to anything for now, because we want to define it in init_chassis()).
uint TutorialCar::RevMask;
uint TutorialCar::BrakeMask;
uint TutorialCar::TurnLeftMask;
uint TutorialCar::TurnRightMask;
uint TutorialCar::MainLightOffset;
int TutorialCar::FLwheel;
int TutorialCar::FRwheel;
int TutorialCar::RLwheel;
int TutorialCar::RRwheel;
int TutorialCar::SteerWheel;
int TutorialCar::SpeedGauge;
int TutorialCar::AccelPedal;
int TutorialCar::BrakePedal;
int TutorialCar::DriverDoor;
int TutorialCar::SndStarter;
int TutorialCar::SndEngON;
int TutorialCar::SndEngOFF;
int TutorialCar::SrcOnOff;
int TutorialCar::SrcEngOn;

//namespace used, so that the registration will be limited in scope and only be visible within the translation unit, where the namespace is defined
namespace 
{
	//It is neccesary to register a derived client interface class, for that use macro IFC_REGISTER_CLIENT
	// param - derived client interface class
	IFC_REGISTER_CLIENT(TutorialCar);
}

//For now, it is needed to register also class, which is derived from ot::script_module, this will be changed later....
class register_vehicle : public ot::script_module
{
};

namespace dummy
{
	IFC_REGISTER_CLIENT(register_vehicle);
}

//Initialize chassis (shared across all instances of same type)
// param params - custom parameters from objdef
ot::chassis_params TutorialCar::init_chassis(const coid::charstr& params)
{
	//Define wheel setup
	ot::wheel wheelParam = {			
		//Wheel parameters, that can be used:
	   .radius1 = 0.31515f,				//outer tire radius [m]
	   .width = 0.2f,					//tire width [m]
	   .suspension_max = 0.1f,			//max.movement up from default position [m]
	   .suspension_min = -0.12f,		//max.movement down from default position [m]
	   .suspension_stiffness = 30.0f,	//suspension stiffness coefficient
	   .damping_compression = 0.4f,		//damping coefficient for suspension compression
	   .damping_relaxation = 0.12f,		//damping coefficient for suspension relaxation
	   .grip = 1.f,						//relative tire grip compared to an avg.tire, +-1
	   .slip_lateral_coef = 1.5f,		//lateral slip muliplier, relative to the computed longitudinal tire slip
	   .differential = true,			//true if the wheel is paired with another through a differential
	};

	//Add wheel with vertical suspension (along z axis) using add_wheel() function
	// 1.param - wheel joint/bone pivot
	// 2.param - wheel structure
	// returns ID of the wheel
	FLwheel = add_wheel("wheel_l0", wheelParam);
	FRwheel = add_wheel("wheel_r0", wheelParam);
	RLwheel = add_wheel("wheel_l1", wheelParam);
	RRwheel = add_wheel("wheel_r1", wheelParam);

	//Get joint/bone ID for given bone name, using get_joint_id() function
	// param - bone name
	// returns joint/bone id or -1 if doesn't exist
	SteerWheel = get_joint_id("steering_wheel");
	SpeedGauge = get_joint_id("dial_speed");
	AccelPedal = get_joint_id("pedal_accelerator");
	BrakePedal = get_joint_id("pedal_brake");
	DriverDoor = get_joint_id("door_l0");

	//Register action handlers, which are called, when binded input/object changed it's state.

	//Register event handlers, invoked on key or button press (for example a fire action)
	// 1.param name - hierarchic action name (config_file/group/action)
	// 2.param handler - handler for changed value (our callback function)
	// 3.param handler_id - optional extra data for the handler
	// 4.param group - activation group where the action is assigned (can be enabled/disabled together)
	// returns slot id or -1 on fail
	register_event_handler("vehicle/engine/on", &TutorialCar::Engine);
	register_event_handler("vehicle/engine/reverse", &TutorialCar::Reverse);
	register_event_handler("vehicle/lights/emergency", &TutorialCar::EmergencyLights);

	//Register value handlers, they are receiving a tracked value which is by default in -1..1 range
	// 1.param name - hierarchic action name (config_file/group/action)
	// 2.param handler - handler for changed value (our callback function)
	// 3.param ramp - value limiter parameters
	// 4.param handler_id - optional extra data for the handler
	// 5.param def_val - optional default action value
	// 6.param group - activation group where the action is assigned (can be enabled/disabled together)
	// returns slot id or -1 on fail
	register_axis_handler("vehicle/lights/passing", &TutorialCar::PassingLights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/lights/main", &TutorialCar::MainLights, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/lights/turn", &TutorialCar::TurnLights, { .minval = -1.f, .maxval = 1.f, .cenvel = 0.f, .vel = 10.f });
	register_axis_handler("vehicle/controls/open", &TutorialCar::OpenDoor, { .minval = 0.f, .maxval = 1.f, .cenvel = 0.f, .vel = 0.6f });
	
	//Ramp parameters, that can be used: 
	// float minval - minimum value to clamp to, should be >= -1 for compatibility with joystick
	// float maxval - maximum value to clamp to, should be <= +1 for compatibility with joystick
	// float cenvel - centering speed per second (speed after releasing the button), 0 for freeze on button release
	// float vel - max rate of value change (saturated) per second
	// float acc - max rate of initial velocity change per second
	// uint8 positions - number of positions between min and max values including the endpoints, 0 = no stepping mode
	// uint8 extra_channels - optional number of extra channels (multiple engines etc)

	//Define light parameters
	//Light parameters, that can be defined: 
	// float size - diameter of the light source (reflector or bulb)
	// float angle - light field angle [deg], ignored on point lights
	// float edge - soft edge coefficient, 0..1 portion of the light area along the edges where light fades to make it soft
	// float intensity - light intensity (can be left 0 and use the range instead)
	// float4 color - RGB color and near-infrared component of the light emitter
	// float range - desired range of light
	// float fadeout - time to fade after turning off
	LightParams = { .size = 0.05f, .angle = 120.f, .edge = 0.8f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .range = 70.f, .fadeout = 0.05f };
	//If either intensity or range are specified alone, the other one is computed automatically. If neither one is specified, the intensity is taken from the color value, otherwise the colors are normalized. If both intensity and range are specified, the light can have shorter range to avoid leaking outside of cockpits etc.
	//Note: intensity of light is affected not only by "intensity" param, but also by angle and range


	//Define circular spotlight source, using add_spot_light
	// 1.param offset - model-space offset relative to the bone or model pivot
	// 2.param dir - light direction
	// 3.param lp - light parameters
	// 4.param joint - joint name to attach the light to
	// returns light emitter id

	//Passing front lights
	uint passLightOffset =
	add_spot_light({ -0.5f, 2.2f, 0.68f }, { 0.0f, 1.f, 0.0f }, LightParams);
	add_spot_light({ 0.5f, 2.2f, 0.68f }, { 0.0f, 1.f, 0.0f }, LightParams);

	//Tail lights
	LightParams = { .size = 0.07f, .angle = 160.f, .edge = 0.8f, .color = { 1.0f, 0.0f, 0.0f, 0.0f } , .range = 150.f, .fadeout = 0.05f };
	uint tailLightOffset =
	add_spot_light({ 0.05f, 0.f, 0.02f }, { 0.0f, 1.f, 0.0f }, LightParams, "tail_light_l0");
	add_spot_light({ 0.05f, 0.f, 0.02f }, { 0.0f, 1.f, 0.0f }, LightParams, "tail_light_r0");

	//Brake lights
	LightParams = { .size = 0.04f, .angle = 120.f, .edge = 0.8f, .color = { 1.0f, 0.0f, 0.0f, 0.0f}, .range = 100.f, .fadeout = 0.05f };
	uint brakeLightOffset =
	add_spot_light({ -0.38f, -2.11f, 0.67f }, { 0.0f, -1.f, 0.0f }, LightParams);
	add_spot_light({ 0.38f, -2.11f, 0.67f }, { 0.0f, -1.f, 0.0f }, LightParams);

	//You can specify bit mask (BrakeMask) using bit logic
	//Our 2 brake lights are defined as follows:
	//	01 - left brake light 
	//	10 - right brake light 
	//	11 - both brake lights  
	//We want the bit mask (BrakeMask) to affect both lights, therefore the given value will be "0b11" (in this case, the value is written in binary system, but it can also be written in decimal or hexadecimal system).
	//Also we want, that the mask starts affecting lights from the first brake light, therefore we have to "left shift" the bit mask by brake light offset
	BrakeMask = 0b11 << brakeLightOffset;

	//Reverse lights
	LightParams = { .size = 0.04f, .angle = 120.f, .edge = 0.8f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .range = 100.f, .fadeout = 0.05f };
	uint revLightOffset =
	add_spot_light({ -0.50f, -2.11f, 0.72f }, { 0.0f, -1.f, 0.0f }, LightParams);
	add_spot_light({ 0.50f, -2.11f, 0.72f }, { 0.0f, -1.f, 0.0f }, LightParams);
	//We want both lights to shine, when we hit the reverse button (in this case, the value is written in decimal system)
	// 01 (binary) -> 1 (decimal) - left reverse light 
	// 10 (binary) -> 2 (decimal) - right reverse light 
	// 11 (binary) -> 3 (decimal) - both reverse lights 
	RevMask = 3 << revLightOffset;

	//Main lights
	LightParams = { .size = 0.05f, .angle = 110.f, .edge = 0.8f, .color = { 1.0f, 1.0f, 1.0f, 0.0f }, .range = 110.f, .fadeout = 0.05f };
	MainLightOffset =
	add_spot_light({ -0.45f, 2.2f, 0.68f }, { 0.0f, 1.f, 0.0f }, LightParams);
	add_spot_light({ 0.45f, 2.2f, 0.68f }, { 0.0f, 1.f, 0.0f }, LightParams);
	//In this case, main lights will work with offset, instead of mask

	//Define point light source, using add_point_light 
	// 1.param offset - model-space offset relative to the bone or model pivot
	// 2.param lp - light parameters
	// 3.param joint - joint name to attach the light to
	// returns light emitter id

	//Turn lights
	LightParams = { .size = 0.1f, .edge = 0.8f, .intensity = 1.f, .color = { 0.4f, 0.1f, 0.0f, 0.0f}, .range = 0.004f, .fadeout = 0.f };
	uint turnLightOffset =
	add_point_light({ -0.71f, 2.25f, 0.63f }, LightParams);
	add_point_light({ -0.64f, -2.11f, 0.72f }, LightParams);
	add_point_light({ 0.71f, 2.25f, 0.63f }, LightParams);
	add_point_light({ 0.64f, -2.11f, 0.72f }, LightParams);
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
	TurnLeftMask = 0x3 << turnLightOffset;
	//To create right turn light mask, add the number of previous left turn lights to the offset (or you can make another offset for right turn lights and use that....)
	TurnRightMask = 0x3 << (turnLightOffset + 2);

	//Load sound samples using load_sound() function
	// param - string filename (audio file name, possibly with path)
	// returns sound ID
	SndStarter = load_sound("sounds/starter.ogg");
	SndEngON = load_sound("sounds/2714_on.ogg");
	SndEngOFF = load_sound("sounds/2714_off.ogg");

	//Create sound emitters that will be used by the vehicle
	// param - joint/bone, from which we want the sound to emit
	// returns emitter ID, which is later used in code
	SrcOnOff = add_sound_emitter("exhaust_0_end");
	SrcEngOn = add_sound_emitter("exhaust_0_end");

	//Return parameters
	ot::chassis_params chassisParam = {
		//Chassis parameters that can be used: 
		.mass = 1120.0f,					//vehicle mass [kg]
		.com_offset = {0.0f, 0.0f, 0.3f},	//center of mass offset
		.clearance = 0.f,					//clearance from ground, default wheel radius
		.bumper_clearance = 0.f,			//clearance in front and back (train bumpers)
		.steering = {
			//Steering parameters that can be used: 
			.bone_ovr = "steering_wheel",	//< steering wheel bone name override, default "steering_wheel"
			.radius = 0.17f,				//< steering wheel radius, if 0 disabled
			.grip_angle = 0.15f,			//< grip angular offset in degrees
			.steering_thr = 50.f,			//< speed [km/h] at which the steering speed is reduced by 60%
			.centering_thr = 20.f,			//< speed [km/h] when the centering acts at 60% already
		}
	};

	return chassisParam;
}

//Invoked for each new instance of the vehicle (including the first one),it is used to define per-instance parameters.
// param reload - true if object is being reloaded, false if object is going to be destroyed
void TutorialCar::init_vehicle(bool reload)
{
	//Initialize variables for your instance (use "this" keyword for changes to affect only current instance) 
	this->Emer = false;
	this->Started = false;
	this->Time = 0.f;
	this->Lturn = 0;
	this->Rturn = 0;
	this->EngDir = 1;

	//Get geomob interface
	this->Geom = get_geomob(0);
	//Get sound interface
	this->Sounds = sound();

	//Set FPS camera position
	set_fps_camera_pos({ -0.4f, 0.0f, 1.2f });
}

//Used to turn on/off the vehicle and play according sounds
void TutorialCar::Engine(int flags, uint code, uint channel, int handler_id)
{
	//Ternary operator used, to switch the "Started" value
	this->Started = this->Started == false ? true : false;
	//Write a fading message on the screen, depending on the "Started" value
	fade(this->Started == true ? "Engine ON" : "Engine OFF");

	//Play sounds, depending on the "Started" state
	if (this->Started == true)
	{
		//play_sound() is used to play sound once, discarding older sounds
		// 1.param - emitter (source ID) 
		// 2.param - sound (sound ID))
		this->Sounds->play_sound(SrcOnOff, SndStarter);
	}
	else
	{
		//stop() discards all sounds playing on given emitter
		this->Sounds->stop(SrcEngOn);
		this->Sounds->play_sound(SrcOnOff, SndEngOFF);
	}
}
//Used to change direction and turn on/off reverse lights
void TutorialCar::Reverse(int flags, uint code, uint channel, int handler_id)
{
	this->EngDir = this->EngDir >= 0 ? -1 : 1;
	fade(this->EngDir > 0 ? "Forward" : "Reverse");
	//light_mask() function is used to turn lights on/off
	// 1.param - light mask value - which bits/lights will be affected 
	// 2.param - condition, when true, given lights will be affected ("turned on")
	// 3.param - offset, from which the light mask will affect bits/lights
	light_mask(RevMask, this->EngDir < 0);
}
//Used to turn on/off emergency lights
void TutorialCar::EmergencyLights(int flags, uint code, uint channel, int handler_id)
{
	//Bitwise operator XOR used, to switch the value each time, this function is called.
	this->Emer ^= 1;
}
//Used to turn on/off passing lights
void TutorialCar::PassingLights(float val, uint code, uint channel, int handler_id)
{
	//if v===1, lights will turn on (light mask will affect the front and tail lights) 
	light_mask(0xf, val == 1);
	//if v===0, lights will turn off (light mask will affect the front and tail lights) 
	light_mask(0x0, val == 0);
	//Note: this affects first 4 defined lights (because we didn't give an offset as 3. parameter). In this case it will affect 2 front lights and 2 tail lights, because every light is represented as 1 bit and as 1.parameter we used hexadecimal 0x0..0xf which works with 4 bits (0000....1111), we can also use 0x00..0xff which will work with first 8 bits
}
//Used to turn on/off main lights
void TutorialCar::MainLights(float val, uint code, uint channel, int handler_id)
{
	//Another way to use light mask, is to give light offset (in this case mainLightOffset) as 3. parameter, from this offset the light mask will affect next bits/lights
	//This way, there is no need for a mask, like with brake & turn lights...
	light_mask(0x3, val == 1, MainLightOffset);
	light_mask(0x0, val == 0, MainLightOffset);
}
//Used to toggle turn lights
void TutorialCar::TurnLights(float val, uint code, uint channel, int handler_id)
{
	if (val == 0)
	{
		//Turn off turn lights
		this->Lturn = 0;
		this->Rturn = 0;
	}
	else if (val < 0)
	{
		//Turn on left turn light
		this->Lturn = 1;
		this->Rturn = 0;
	}
	else
	{
		//Turn on right turn light
		this->Lturn = 0;
		this->Rturn = 1;
	}
}
//Used to open/close driver door
void TutorialCar::OpenDoor(float val, uint code, uint channel, int handler_id)
{
	//Check, if the Geom pointer is not null before attempting to use it, to avoid crashing due to a null pointer
	if (!this->Geom)
	{
		return;
	}
	//rotate_joint_orig() is used to rotate joint by given angle and back to default position
	// 1.param - bone/joint ID
	// 2.param - vec rotation angle in radians
	// 3.param - rotation axis vector (must be normalized) - axis around which the bone rotates (in this case around Z axis) and the direction of rotation (-1...1)
	this->Geom->rotate_joint_orig(DriverDoor, (val * 1.5f), { 0.f, 0.f, -1.f });
}

//Update model instance
void TutorialCar::update_frame(float dt, float engine, float brake, float steering, float parking)
{	
	//Get current ground speed, using speed() function (returns current speed in m/s, multiply with 3.6 to get km/h or 2.23693629 to get mi/h) 
	float kmh = speed() * 3.6f;
	//Calculate force, but only if the car has started
	if (this->Started == true)
	{
		//Calculate force value, depending on the direction
		float redux = this->EngDir >= 0 ? 0.2f : 0.6f;
		//glm library can be used
		engine = EngineForce * glm::abs(engine);
		float force = (kmh >= 0) == (this->EngDir >= 0)
			? engine / (redux * glm::abs(kmh) + 1)
			: engine;
		//Add wind resistance
		force -= ForceLoss;
		//Make sure, that force can not be negative
		force = glm::max(0.0f, glm::min(force, engine));
		//Calculate force and direction, which will be used to add force to wheels
		engine = force * this->EngDir;

		//Move only when there is no sound playing on given emitter (to not be able to move when car is starting, but after the starter sound ends)
		//Note: add this code before wheel_force() functions
		//is_playing() function checks, if there is sound playing on given emitter
		if (this->Sounds->is_playing(SrcOnOff))
		{
			engine = 0;
		}
		else if (this->Started == true && !this->Sounds->is_looping(SrcEngOn))
		{
			//play_loop() is used to play sound in loop, breaking other sounds
			// 1.param - emitter (source ID)
			// 2.param - sound (sound ID))
			this->Sounds->play_loop(SrcEngOn, SndEngON);
		}
	}
	//Use wheel_force() function to apply propelling force on wheels, to move the car
	// 1.param - wheel ID (in this case, the car has front-wheel drive)
	// 2.param - force, you want to exert on the wheel hub in forward/backward  direction (in Newtons)
	wheel_force(FLwheel, engine);
	wheel_force(FRwheel, engine);
	//As 1.parameter, you can also use -1 to affect all wheels, or -2 to affect first 2 wheels
	//Example: this.wheel_force(-1, engine)

	//Define steering sensitivity
	steering *= 0.5f;
	//Steer wheels by setting angle, using steer() function
	// 1.param - wheel ID
	// 2.param - angle in radians to steer the wheel
	steer(FLwheel, steering);
	steer(FRwheel, steering);

	//Apply brake light mask (BrakeMask) on brake lights, when brake value is bigger than 0
	//Note: add this code before adding rolling friction to brakes.
	light_mask(BrakeMask, brake > 0);

	//Calculate the brake value, which will affect the wheels when you are braking
	//Originally "brake" has value between 0..1, you have to multiply it by "BrakeForce" to have enough force to brake
	brake *= BrakeForce;
	//Rolling friction
	brake += 200;
	//Use wheel_brake() to apply braking force on given wheels
	// 1.param - wheel ID
	// 2.param - braking force, you want to exert on the wheel hub (in Newtons)
	wheel_brake(-1, brake);

	if (this->Lturn == 1 || this->Rturn == 1 || this->Emer == true)
	{
		//When turn/emergency lights are turned on, calculate blinking time for turn signal lights
		this->Time += dt;
		double blt = this->Time * 0.85;
		//For turn lights blinking effect
		int blink = (blt - glm::floor(blt)) > 0.47f ? 1 : 0;
		//Apply light mask for turn lights, depending on which action was handled (left turn lights, right turn lights or all turn lights (emergency))
		light_mask(TurnLeftMask, blink && (this->Lturn || this->Emer));
		light_mask(TurnRightMask, blink && (this->Rturn || this->Emer));
	}
	else
	{
		//To turn off the active turn lights
		light_mask(TurnLeftMask, false);
		light_mask(TurnRightMask, false);
	}

	//If engine has started, calculate and set volume pitch and gain for emitter
	if (this->Started == true)
	{
		//max_rpm() function returns rpm of the fastest revolving wheel
		float rpm = this->max_rpm();
		float pitch = glm::abs(kmh / 40) + glm::abs(rpm / 200);
		float pitchRpm = rpm > 0 ? glm::floor(pitch) : 0;
		pitch += (0.5f * pitchRpm) - pitchRpm;
		//Use set_pitch() to set pitch value on given emitter
		// 1.param - emitter 
		// 2.param - pitch value (this will affect all sounds emitted from this emitter)
		this->Sounds->set_pitch(SrcEngOn, (0.5f * pitch) + 1.f);
		//Use set_gain() to set gain value on given emitter
		// 1.param - emitter 
		// 2.param - gain value (this will affect all sounds emitted from this emitter)
		this->Sounds->set_gain(SrcEngOn, (0.25f * pitch) + 0.5f);
	}

	//Check, if the Geom pointer is not null before attempting to use it, to avoid crashing due to a null pointer
	if (!this->Geom)
	{
		return;
	}


	if (kmh > SpeedGaugeMin)
	{
		//Rotate speedometer needle
		this->Geom->rotate_joint_orig(SpeedGauge, (kmh - SpeedGaugeMin) * RadPerKmh, { 0.f, 1.f, 0.f });
	}
	//Rotate steering wheel
	this->Geom->rotate_joint_orig(SteerWheel, 10.5f * steering, { 0.f, 0.f, 1.f });
	//For animating wheels is better to use animate_wheels() function, which simplifies the animation
	animate_wheels();
}


