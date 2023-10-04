//*****Version 4 - Sounds*****

//Declare sounds and sound emmiter variables
var SndStarter, SndEngineON, SndEngineOFF, SndEmitExhaust;

var BrakeMask,RevMask,TurnLeftMask,TurnRightMask;
var SteerWheel, SpeedGauge, AccelPedal, BrakePedal, DriverDoor, FLwheel, FRwheel, RLwheel, RRwheel, Started;
var SpeedGaugeMin = 10.0;
var RadPerKmh = 0.018325957;
var EngineForce = 25000.0;
var BrakeForce = 5000.0;
var MaxKmh = 200;
var ForceLoss = EngineForce / (0.2*MaxKmh + 1);

function init_chassis()
{ 
	var wheelparam = {
		radius: 0.31515,
		width: 0.2,
		suspension_max: 0.1,
		suspension_min: -0.12,
		suspension_stiffness: 30.0,
		damping_compression: 0.4,
		damping_relaxation: 0.12,
		grip: 1,
	};
	
	FLwheel = this.add_wheel('wheel_l0', wheelparam); 
	FRwheel = this.add_wheel('wheel_r0', wheelparam); 
	RLwheel = this.add_wheel('wheel_l1', wheelparam); 
	RRwheel = this.add_wheel('wheel_r1', wheelparam); 

	var body = this.get_geomob(0);
	SteerWheel = body.get_joint('steering_wheel');		
	SpeedGauge = body.get_joint('dial_speed');							
	AccelPedal = body.get_joint('pedal_accelerator');
	BrakePedal = body.get_joint('pedal_brake');				
	DriverDoor = body.get_joint('door_l0');	
	
	this.register_event("car/engine/reverse", ReverseAction); 
	this.register_event("car/engine/on", EngineAction);
	this.register_axis("car/controls/open", {minval:0, center:0, vel:0.6}, function(v) {
		var doorax = {z:-1};
		var doorangle = v * 1.5;
		this.geom.rotate_joint_orig(DriverDoor, doorangle, doorax);
	}); 
	this.register_switch("car/lights/passing", function(v) {
		this.light_mask(0xf, v>0);
		this.light_mask(0x0, v<0);
	});
	this.register_switch("car/lights/main", function(v) {	
		this.light_mask(0xf, v>0, mainlights);
		this.light_mask(0x0, v<0, mainlights);
	});
	this.register_switch("car/lights/turn", function(v) {
		if(v==0)
		{
			this.lturn=this.rturn=0;
		}
		else if(v<0)
		{
			this.lturn = 1; this.rturn = 0;
		}
		else
		{
			this.rturn = 1; this.lturn = 0;
		}
	});
	this.register_event("car/lights/emergency", function(v) { this.emer ^= 1; });
	
	var lightprop = {size:0.1, angle:54, edge:0.08, intensity:0.04, range:30};
	this.add_spot_light({x:-0.5,y:2.2,z:0.68}, {y:1}, lightprop); 	//left front light
	this.add_spot_light({x:0.5,y:2.2,z: 0.68}, {y:1}, lightprop);	//right front light

	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.05, range:0.3, color:{x:1,y:1e-4}};
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightprop, "tail_light_l0"); 	//left tail light
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightprop, "tail_light_r0"); 	//right tail light  

	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.1, range:0.5, color:{x:1,y:1e-3}, fadeout:0};
	var brakelights =  
	this.add_spot_light({x:-0.38,y:-2.11,z: 0.67}, {y:-1}, lightprop); 	//left brake light
	this.add_spot_light({x:0.38,y:-2.11,z: 0.67}, {y:-1}, lightprop); 	//right brake light
	BrakeMask = 0b11<<brakelights;

	lightprop = {size:0.12, angle:84, edge:0.8, intensity:0.2, range:5, fadeout:0};
	var revlights =
	this.add_spot_light({x:-0.50,y:-2.11,z: 0.72}, {y:-1}, lightprop);	//left reverse light
	this.add_spot_light({x:0.50,y:-2.11,z: 0.72}, {y:-1}, lightprop);	//right reverse light
	RevMask = 3<<revlights;

	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.02, color:{x:1,y:0.8}, fadeout:0};
	var turnlights =
	this.add_spot_light({x:-0.71,y:2.25,z:0.63}, {y:1}, lightprop); 	//left front turn light 
	this.add_spot_light({x:-0.64,y:-2.11,z: 0.72}, {y:-1}, lightprop); 	//left rear turn light 
	this.add_spot_light({x:0.71,y:2.25,z: 0.63}, {y:1}, lightprop); 	//right front turn light 
	this.add_spot_light({x:0.64,y:-2.11,z: 0.72}, {y:-1}, lightprop); 	//right rear turn light 
	TurnLeftMask = 0x3<<turnlights;
	TurnRightMask = 0xc<<turnlights;

	lightprop = {size:0.1, angle:54, edge:0.08, intensity:0.04, range:30};
	var mainlights = 
	this.add_spot_light({x:-0.45,y:2.2,z:0.68}, {y:1}, lightprop); 	//left main light
	this.add_spot_light({x:0.45,y:2.2,z: 0.68}, {y:1}, lightprop);	//right main light


	//Load sound samples (located in "sounds" folder) using load_sound() function (takes string filename (audio file name, possibly with path) as parameter)
	//Function load_sound() returns integer ID, which is later used in code( some functions take this ID as parameter(steer, wheel_force, wheel_brake etc.))
	SndStarter = this.load_sound("sounds/starter.ogg");		//will have ID 0
	SndEngineON = this.load_sound("sounds/2714_on.ogg");	//will have ID 1
	SndEngineOFF = this.load_sound("sounds/2714_off.ogg");	//will have ID 2

	//Create sound emitter that will be used by the vehicle(takes joint/bone (from which we want the sound to emit) as parameter)
	//Function add_sound_emitter() returns integer ID, which is later used in code
	SndEmitExhaust = this.add_sound_emitter("exhaust_0_end");	//will have ID 0

	return {
		mass: 1120.0,
		com: {x: 0.0, y: -0.2, z: 0.3},
		steering_params:{
			radius: 0.17
		}
	};
}

function init_vehicle()
{	
	this.time = 0;
	this.lturn = this.rturn = this.emer = 0;
	this.geom = this.get_geomob(0);
	Started = 0;
	this.engdir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
	
	//this.sound() function lets us access the sounds, which we defined with load_sound() function
	this.snd = this.sound();
}

function ReverseAction(v)
{
	this.engdir = this.engdir>=0 ? -1 : 1;
	this.fade(this.engdir>0 ? "forward" : "reverse");
	this.light_mask(RevMask, this.engdir<0);
}

function EngineAction()
{
	Started = Started == 0 ? 1 : 0;
	this.fade(Started == 1  ? "Engine ON" : "Engine OFF");
	
	if(Started) 
	{
		//play_sound() is used to play sound once (first parameter is emitter (source ID) and second parameter is sound (sound ID))
		this.snd.play_sound(SndEmitExhaust, SndStarter);
		//In this case enqueue_loop() is used to play sound in loop after the previous sound ends (first parameter is emitter (source ID) and second parameter is sound (sound ID))
		this.snd.enqueue_loop(SndEmitExhaust, SndEngineON);
	}
	else 
	{
		//Play sound with ID 2 from emitter with ID 0 once 
		this.snd.play_sound(SndEmitExhaust, SndEngineOFF);
	}
}

function update_frame(dt, engine, brake, steering, parking)
{
	var brakeax = {x:1};
	var brakeangle = brake*0.4;	
	var accelax = {y:(-engine*0.02), z:(-engine*0.02)}
	this.geom.rotate_joint_orig(BrakePedal, brakeangle, brakeax);
	this.geom.move_joint_orig(AccelPedal, accelax)
	
	var kmh = this.speed()*3.6;
	
	if (Started == 1)
	{
	var redux = this.engdir>=0 ? 0.2 : 0.6;
	engine = EngineForce*Math.abs(engine);
	var force = (this.engdir>=0) == (kmh>=0)
		? engine/(redux*Math.abs(kmh) + 1)
		: engine;
	force -= ForceLoss;
	force = Math.max(0.0, Math.min(force, engine));
	engine = force * this.engdir;
	}
	
	//Start moving only when sound with ID 0 (starter sound) is not playing (without this, the car could move even while the engine is starting)
	//Add this code before wheel_force() functions
	if(!this.snd.is_looping(SndStarter))
	{
	engine = 0;
	}
	
	this.wheel_force(FLwheel, engine);
	this.wheel_force(FRwheel, engine);
	
	if(kmh > SpeedGaugeMin)
	{
        this.geom.rotate_joint_orig(SpeedGauge, (kmh - SpeedGaugeMin) * RadPerKmh, {x:0,y:1,z:0});    
    }
	
	steering *= 0.3;
	this.steer(FLwheel, steering);	//front left wheel
	this.steer(FRwheel, steering);	//front right wheel
	this.geom.rotate_joint_orig(SteerWheel, 10.5*steering, {z:1});

	this.light_mask(BrakeMask, brake>0);

	brake *= BrakeForce; 
	brake += 200;
	this.wheel_brake(-1, brake);
	this.animate_wheels();

	if(this.lturn || this.rturn || this.emer)
	{
		this.time += dt;
		var blt = this.time*0.85;
		var blink = (blt - Math.floor(blt)) > 0.47 ? 1 : 0;
		
		this.light_mask(TurnLeftMask, blink&(this.lturn|this.emer));
		this.light_mask(TurnRightMask, blink&(this.rturn|this.emer));
	}
	else
	{
		this.light_mask(TurnLeftMask, false);
		this.light_mask(TurnRightMask, false);
	}
	
	//If engine has started, calculate and set volume pitch for emitter
	if(Started==1) 
	{
		//max_rpm() function returns rpm of the fastest revolving wheel
		var rpm = this.max_rpm();
		var pitch = Math.abs(kmh/40) + Math.abs(rpm/200.0);
		var g = rpm>0 ? Math.floor(pitch) : 0;
		var f = pitch - g;
		f += 0.5 * g;
		//First parameter is emitter and second parameter is pitch value (this will affect all sounds emitted from this emitter)
		this.snd.set_pitch(SndEmitExhaust, (0.5 * f) + 1.0);
	}
}
