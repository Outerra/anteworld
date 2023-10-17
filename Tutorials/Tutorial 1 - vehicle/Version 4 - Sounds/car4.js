//*****Version 4 - Sounds*****
 
//Declare sounds and sound emitter variables
let SndStarter, SndEngON, SndEngOFF, SrcOnOff, SrcEngOn;

let BrakeMask, RevMask, TurnLeftMask, TurnRightMask,
SteerWheel, SpeedGauge, AccelPedal, BrakePedal, DriverDoor, FLwheel, FRwheel, RLwheel, RRwheel;

const SpeedGaugeMin = 10.0;
const RadPerKmh = 0.018325957;
const EngineForce = 25000.0;
const BrakeForce = 5000.0;
const MaxKmh = 200;
const ForceLoss = EngineForce / (0.2*MaxKmh + 1);

function init_chassis()
{ 
	let wheelParam = {
		radius: 0.31515,
		width: 0.2,
		suspension_max: 0.1,
		suspension_min: -0.12,
		suspension_stiffness: 30.0,
		damping_compression: 0.4,
		damping_relaxation: 0.12,
		grip: 1,
	};
	
	FLwheel = this.add_wheel('wheel_l0', wheelParam); 
	FRwheel = this.add_wheel('wheel_r0', wheelParam); 
	RLwheel = this.add_wheel('wheel_l1', wheelParam); 
	RRwheel = this.add_wheel('wheel_r1', wheelParam); 

	let body = this.get_geomob(0);
	SteerWheel = body.get_joint('steering_wheel');		
	SpeedGauge = body.get_joint('dial_speed');							
	AccelPedal = body.get_joint('pedal_accelerator');
	BrakePedal = body.get_joint('pedal_brake');				
	DriverDoor = body.get_joint('door_l0');	
	
	this.register_event("vehicle/engine/reverse", ReverseAction); 
	this.register_event("vehicle/engine/on", EngineAction);
	this.register_axis("vehicle/controls/open", {minval:0, center:0, vel:0.6}, function(v) {
		let doorAx = {z:-1};
		let doorAngle = v * 1.5;
		this.Geom.rotate_joint_orig(DriverDoor, doorAngle, doorAx);
	}); 
	this.register_axis("vehicle/lights/passing", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) {
		this.light_mask(0xf, v===1);
		this.light_mask(0x0, v===0);
	});
	this.register_axis("vehicle/lights/main", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) {	
		this.light_mask(0x3, v===1, mainLightOffset);
		this.light_mask(0x0, v===0, mainLightOffset);
	});
	this.register_axis("vehicle/lights/turn", {minval: -1, maxval: 1, vel: 10, center: 0 }, function(v) {
		if(v===0)
		{
			this.Lturn=this.Rturn=0;
		}
		else if(v<0)
		{
			this.Lturn = 1; this.Rturn = 0;
		}
		else
		{
			this.Rturn = 1; this.Lturn = 0;
		}
	});
	this.register_event("vehicle/lights/emergency", function(v) { this.Emer ^= 1; });
	
	let lightProp = {size:0.05, angle:120, edge:0.2, fadeout:0.05, range:70 };
	this.add_spot_light({x:-0.5,y:2.2,z:0.68}, {y:1}, lightProp); 	//left front light
	this.add_spot_light({x:0.5,y:2.2,z: 0.68}, {y:1}, lightProp);	//right front light

	lightProp = { size: 0.07, angle: 160, edge: 0.8, fadeout: 0.05, range: 150, color: { x: 1.0 } };
	this.add_spot_light({x:0.05,y:0,z:0.02}, {y:1}, lightProp, "tail_light_l0"); 	//left tail light
	this.add_spot_light({x:0.05,y:0,z:0.02}, {y:1}, lightProp, "tail_light_r0"); 	//right tail light  

	lightProp = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100, color: { x: 1.0 } };
	let brakeLightOffset =  
	this.add_spot_light({x:-0.38,y:-2.11,z: 0.67}, {y:-1}, lightProp); 	//left brake light
	this.add_spot_light({x:0.38,y:-2.11,z: 0.67}, {y:-1}, lightProp); 	//right brake light
	BrakeMask = 0b11<<brakeLightOffset;

	lightProp = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100 };
	let revLightOffset =
	this.add_spot_light({x:-0.50,y:-2.11,z: 0.72}, {y:-1}, lightProp);	//left reverse light
	this.add_spot_light({x:0.50,y:-2.11,z: 0.72}, {y:-1}, lightProp);	//right reverse light
	RevMask = 3<<revLightOffset;

	lightProp = {size:0.1, edge:0.8, fadeout:0, color:{x:0.4,y:0.1,z:0},range:0.004,  intensity:1 };
	let turnLightOffset =
	this.add_point_light({x:-0.71,y:2.25,z:0.63}, lightProp); 	//left front turn light 
	this.add_point_light({x:-0.64,y:-2.11,z: 0.72}, lightProp); 	//left rear turn light 
	this.add_point_light({x:0.71,y:2.25,z: 0.63}, lightProp); 	//right front turn light 
	this.add_point_light({x:0.64,y:-2.11,z: 0.72}, lightProp); 	//right rear turn light 
	TurnLeftMask = 0x3<<turnLightOffset;
	TurnRightMask = 0x3<<(turnLightOffset + 2);

	lightProp = { size: 0.05, angle: 110, edge: 0.08, fadeout: 0.05, range: 110 };
	let mainLightOffset = 
	this.add_spot_light({x:-0.45,y:2.2,z:0.68}, {y:1}, lightProp); 	//left main light
	this.add_spot_light({x:0.45,y:2.2,z: 0.68}, {y:1}, lightProp);	//right main light


	//Load sound samples (located in "sounds" folder) using load_sound() function
	//1.param - string filename (audio file name, possibly with path)
	//Function load_sound() returns sound ID, which is later used in code
	SndStarter = this.load_sound("sounds/starter.ogg");		//will have ID 0
	SndEngON = this.load_sound("sounds/2714_on.ogg");	//will have ID 1
	SndEngOFF = this.load_sound("sounds/2714_off.ogg");	//will have ID 2

	//Create sound emitters that will be used by the vehicle
	//1.param - joint/bone, from which we want the sound to emit
	//Function add_sound_emitter() returns emitter ID, which is later used in code
	SrcOnOff = this.add_sound_emitter("exhaust_0_end");	//will have ID 0
	SrcEngOn = this.add_sound_emitter("exhaust_0_end");	//will have ID 1

	return {
		mass: 1120.0,
		com: {x: 0.0, y: 0.0, z: 0.3},
		steering_params:{
			radius: 0.17
		}
	};
}
 
function init_vehicle()
{	
	this.Time = 0;
	this.Lturn = this.Rturn = this.Emer = 0;
	this.Geom = this.get_geomob(0);
	this.Started = 0;
	this.EngDir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
	
	//use sound() function to get sound interface
	this.Snd = this.sound();
}

function ReverseAction(v)
{
	this.EngDir = this.EngDir>=0 ? -1 : 1;
	this.fade(this.EngDir>0 ? "Forward" : "Reverse");
	this.light_mask(RevMask, this.EngDir<0);
}

function EngineAction()
{
	this.Started = this.Started === 0 ? 1 : 0;
	this.fade(this.Started === 1  ? "Engine ON" : "Engine OFF");
	if(this.Started) 
	{
		//play_sound() is used to play sound once, discarding older sounds
		//1.parameter - emitter (source ID) 
		//2.parameter - sound (sound ID))
		//On emitter with ID 0 play sound with ID 0 once
		this.Snd.play_sound(SrcOnOff, SndStarter);
	}
	else 
	{
		//function stop() discards all sounds playing on given emitter
		this.Snd.stop(SrcEngOn);
		//On emitter with ID 0 play sound with ID 2 once  
		this.Snd.play_sound(SrcOnOff, SndEngOFF);
	}
}

function update_frame(dt, engine, brake, steering, parking)
{
	BrakeAx = {x:1};
	BrakeAngle = brake*0.4;	
	AccelAx = {y:(-engine*0.02), z:(-engine*0.02)}
	this.Geom.rotate_joint_orig(BrakePedal, BrakeAngle, BrakeAx);
	this.Geom.move_joint_orig(AccelPedal, AccelAx)
	
	let kmh = this.speed()*3.6;
	
	if(this.Started === 1)
	{
	let redux = this.EngDir>=0 ? 0.2 : 0.6;
	engine = EngineForce*Math.abs(engine);
	let force = (kmh>=0) === (this.EngDir>=0) 
		? engine/(redux*Math.abs(kmh) + 1)
		: engine;
	force -= ForceLoss;
	force = Math.max(0.0, Math.min(force, engine));
	engine = force * this.EngDir;
	
	//Move only when there is no sound playing on given emitter (to not be able to move when car is starting, but after the starter sound ends)
	//Note: add this code before wheel_force() functions
		if(this.Snd.is_playing(SrcOnOff))
		{
			engine = 0;
		}
		//If car has started and there isn't active loop on given emitter, play loop
		else if(this.Started && !this.Snd.is_looping(SrcEngOn))
		{
		//play_loop() is used to play sound in loop, breaking previous sounds
		//1.parameter - emitter (source ID)
		//2.parameter - sound (sound ID))
		this.Snd.play_loop(SrcEngOn, SndEngON);
		
		//another loop function, that can be used is enqueue_loop() 
		//1.parameter - emitter (source ID) 
		//2.parameter - sound (sound ID)
		//can take 3.parameter - bool value (default: true), if true - previous loops will be removed, otherwise the new sound is added to the loop chain
		//example: this.Snd.enqueue_loop(SrcEngOn, SndEngON);
		}
	}
	
	this.wheel_force(FLwheel, engine);
	this.wheel_force(FRwheel, engine);
	
	if(kmh > SpeedGaugeMin)
	{
        this.Geom.rotate_joint_orig(SpeedGauge, (kmh - SpeedGaugeMin) * RadPerKmh, {x:0,y:1,z:0});    
    }
	
	steering *= 0.3;
	this.steer(FLwheel, steering);	//front left wheel
	this.steer(FRwheel, steering);	//front right wheel
	this.Geom.rotate_joint_orig(SteerWheel, 10.5*steering, {z:1});

	this.light_mask(BrakeMask, brake>0);
 
	brake *= BrakeForce; 
	brake += 200;
	this.wheel_brake(-1, brake);
	this.animate_wheels();

	if(this.Lturn || this.Rturn || this.Emer)
	{
		this.Time += dt;
		let blt = this.Time*0.85;
		let blink = (blt - Math.floor(blt)) > 0.47 ? 1 : 0;
		
		this.light_mask(TurnLeftMask, blink&(this.Lturn|this.Emer));
		this.light_mask(TurnRightMask, blink&(this.Rturn|this.Emer));
	}
	else
	{
		this.light_mask(TurnLeftMask, false);
		this.light_mask(TurnRightMask, false);
	}
	
	//If engine has started, calculate and set volume pitch and gain for emitter
	if(this.Started===1) 
	{
		//max_rpm() function returns rpm of the fastest revolving wheel
		let rpm = this.max_rpm();
		let pitch = Math.abs(kmh/40) + Math.abs(rpm/200.0);
		let pitchRpm = rpm > 0 ? Math.floor(pitch) : 0;
		pitch += (0.5 * pitchRpm) - pitchRpm;
		//Use set_pitch() to set pitch value on given emitter
		//1.parameter - emitter 
		//2.parameter - pitch value (this will affect all sounds emitted from this emitter)
		this.Snd.set_pitch(SrcEngOn, (0.5 * pitch) + 1.0);
		//Use set_gain() to set gain value on given emitter
		//1.parameter - emitter 
		//2.parameter - gain value (this will affect all sounds emitted from this emitter)
		this.Snd.set_gain(SrcEngOn, (0.25 * pitch) + 0.5);
	}
}

