//*****Version 3 - Lights*****

//Declare additional global light variables
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
	
	//Add additional action handlers
	//Handle this action, when passing lights button is pressed ('L')
	this.register_switch("car/lights/passing", function(v) {	
		//Every time you press passing light button, parameter "v" switches it's value (it can be 1 or -1)	
		//if v>0, lights will turn on (light mask will affect the front and tail lights) 
		this.light_mask(0xf, v>0);
		//if v<0, lights will turn off
		this.light_mask(0x0, v<0);
		//Warning! This affects first 4 defined lights (because we didn't give an offset as third parameter). In this case it will affect 2 front lights and 2 tail lights, because every light is defined as 1 bit and as first parameter we used hexadecimal 0x0..0xf which works with 4 bits (for example 0x0 = 0000, 0xf = 1111), we can also use 0x00..0xff which will work with first 8 bits
	});
		
	//Another way to use light mask, is to give the lights variable (which contains the lights) as third parameter in light_mask
	//This may be the better solution, because you don't have to use bit logic 
	//This action is handled, when you press Ctrl + L 
	this.register_switch("car/lights/main", function(v) {	
		this.light_mask(0xf, v>0, mainlights);
		this.light_mask(0x0, v<0, mainlights);
	});

	//Handle this action, when turn signals buttons are pressed ('Shift' + 'A' or 'Shift' + 'D')
	//Turn signals can have -1/0/1 values, when 'Shift' + 'A' is pressed, the "v" value switches between 0 and -1, but when 'Shift' + 'D' is pressed, the value moves between 0 and 1. 
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

	//Handle this action, when emergency lights buttons are pressed ('Shift' + 'W')
	//this.emer ^= 1 toggles between 0 and 1
	this.register_event("car/lights/emergency", function(v) { this.emer ^= 1; });
	
	//Define light parameters and assign them to variable "lightprop"
	//Light parameters you can use:	
	//size (float)	 	- diameter of the light source (reflector)
	//angle (float)		- field of light [degrees] (ignored for point lights)
	//edge (float) 		- soft edge coefficient (0..1), portion of the light area along the edges where light fades to make it soft (ignored for point lights)
	//color (float3) 	- RGB color of the light emitter
	//intensity (float)	- light intensity
	//range (float) 	- effective light range
	//fadeout (float) 	- time to fade after turning off
	var lightprop = {size:0.1, angle:54, edge:0.08, intensity:0.04, range:30};
	//If either intensity or range are specified alone, the other one is computed automatically. If neither one is specified, the intensity is taken from the color value, otherwise the colors are normalized. If both intensity and range are specified, the light can have shorter range to avoid leaking outside of cockpits etc.
	
	//Use add_spot_light() function to add lights, where the first parameter is the model-space offset relative to bone or model pivot (must be in {} brackets), second parameter is light direction (must be in {} brackets), third parameter is light values
	//Add front lights (offset relative to model pivot is given for the lights and direction is set to forward by {y:1})
	this.add_spot_light({x:-0.6,y:2.2,z:0.68}, {y:1}, lightprop); 	//left front light
	this.add_spot_light({x:0.6,y:2.2,z:0.68}, {y:1}, lightprop);	//right front light

	//You can change the previous light values in "lightprop" and use them for another lights
	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.05, range:0.3, color:{x:1,y:1e-4}};
	
	//Add tail lights
	//You can also add fourth parameter (string name of the bone, to which you want to bind the light), this will make the lights offset and direction to be relative to the defined bone instead of model pivot (which is default)
	//The direction is now opposite to front lights (tail lights glow backwards) even though direction is {y:1}, because the light is now relative to tail light bone, which has opposite direction
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightprop, "tail_light_l0"); 	//left tail light
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightprop, "tail_light_r0"); 	//right tail light 	
	
	//Add brake lights
	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.1, range:0.5, color:{x:1,y:1e-3}, fadeout:0};
	
	//Here is another example to lights direction, brake lights are relative to model pivot but direction is {y:-1}, which is the opposite direction, therefore these lights will glow backwards
	//Add brake lights and assign them to "brakelights"
	var brakelights =  
	this.add_spot_light({x:-0.38,y:-2.11,z:0.67}, {y:-1}, lightprop); 	//left brake light (0b01)
	this.add_spot_light({x:0.38,y:-2.11,z:0.67}, {y:-1}, lightprop); 	//right brake light (0b10)
	
	//Now we have to identify lights for bit mask (BrakeMask), for that we have to use bit logic
	//Our 2 lights in variable "brakelights" are defined as follows:
	//	01 - left brake light 
	//	10 - right brake light 
	//	11 - both brake lights  
	//We want both lights to glow, when we hit the brake button(we want the bit mask (BrakeMask) to affect both lights assigned to "brakelights"), therefore the given value will be "0b11" (in this case, the value is written in binary system, but it can also be written in decimal or hexadecimal system).
	BrakeMask = 0b11<<brakelights;

	//Add reverse lights
	lightprop = {size:0.12, angle:84, edge:0.8, intensity:0.2, range:5, fadeout:0};
	var revlights =
	this.add_spot_light({x:-0.50,y:-2.11,z:0.72}, {y:-1}, lightprop);	//left reverse light (0b01)
	this.add_spot_light({x:0.50,y:-2.11,z:0.72}, {y:-1}, lightprop);	//right reverse light (0b10)
	
	// 01 (binary) -> 1 (decimal) - left reverse light 
	// 10 (binary) -> 2 (decimal) - right reverse light 
	// 11 (binary) -> 3 (decimal) - both reverse lights 
	//We want both lights to glow, when we hit the reverse button (in this case, the value is written in decimal system)
	RevMask = 3<<revlights;

	//Add turn signal lights with new values
	lightprop = {size:0.12, angle:54, edge:0.8, intensity:0.02, color:{x:1,y:0.8}, fadeout:0};
	var turnlights =
	this.add_spot_light({x:-0.71,y:2.25,z:0.63}, {y:1}, lightprop); 	//left front turn light (0b0001)
	this.add_spot_light({x:-0.64,y:-2.11,z:0.72}, {y:-1}, lightprop); 	//left rear turn light (0b0010)
	this.add_spot_light({x:0.71,y:2.25,z:0.63}, {y:1}, lightprop); 		//right front turn light (0b0100)
	this.add_spot_light({x:0.64,y:-2.11,z:0.72}, {y:-1}, lightprop); 	//right rear turn light (0b1000)
	
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
	//We want lights on the side of the car to glow, when we hit the corresponding turn signal button (left or right) (in this case, the value is written in hexadecimal system )
	//When the left turn signal button was pressed, we want turn lights on the left side to glow 
	TurnLeftMask = 0x3<<turnlights;
	//When the right turn signal button was pressed, we want turn lights on the right side to glow 
	TurnRightMask = 0xc<<turnlights;
	
	//add main lights
	lightprop = {size:0.1, angle:54, edge:0.08, intensity:0.04, range:30};
	var mainlights = 
	this.add_spot_light({x:-0.45,y:2.2,z:0.68}, {y:1}, lightprop); 	//left main light
	this.add_spot_light({x:0.45,y:2.2,z: 0.68}, {y:1}, lightprop);	//right main light

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
	//Initialize additional variables (needed for blink)
	this.time = 0;
	this.lturn = this.rturn = this.emer = 0;
	
	this.geom = this.get_geomob(0);
	Started = 0;
	this.engdir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});

}

function ReverseAction(v)
{
	this.engdir = this.engdir>=0 ? -1 : 1;
	this.fade(this.engdir>0 ? "forward" : "reverse");
	
	//Apply reverse light mask (RevMask) on reverse lights, when engine direction has value -1 (activate reverse lights)
	this.light_mask(RevMask, this.engdir<0);
}

function EngineAction()
{
	Started = Started == 0 ? 1 : 0;
	this.fade(Started == 1  ? "Engine ON" : "Engine OFF");
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

	//Apply brake light mask (BrakeMask) on brake lights, when brake value is bigger than 0 (activate brake lights)
	//Add this before calculating brake force with rolling friction,
	this.light_mask(BrakeMask, brake>0);

	brake *= BrakeForce; 
	//Rolling friction
	brake += 200;
	this.wheel_brake(-1, brake);
	this.animate_wheels();

	if(this.lturn || this.rturn || this.emer)
	{
		//Calculate blinking time for turn signal lights
		this.time += dt;
		var blt = this.time*0.85;
		var blink = (blt - Math.floor(blt)) > 0.47 ? 1 : 0;
		
		//Apply light mask for turn lights, depending of which action was handled (left turn lights, right turn lights or all turn lights (emergency)), which will then turn on and off, depending on the "blink" value
		this.light_mask(TurnLeftMask, blink&(this.lturn|this.emer));
		this.light_mask(TurnRightMask, blink&(this.rturn|this.emer));
	}
	else
	{
		this.light_mask(TurnLeftMask, false);
		this.light_mask(TurnRightMask, false);
	}
}
