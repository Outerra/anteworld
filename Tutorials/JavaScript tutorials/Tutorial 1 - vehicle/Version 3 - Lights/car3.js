//*****Version 3 - Lights*****

//Declare global light variables
let BrakeMask, RevMask, TurnLeftMask, TurnRightMask;

let SteerWheel, SpeedGauge, AccelPedal, BrakePedal, DriverDoor, FLwheel, FRwheel, RLwheel, RRwheel;
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

	SteerWheel = this.get_joint_id('steering_wheel');		
	SpeedGauge = this.get_joint_id('dial_speed');						
	AccelPedal = this.get_joint_id('pedal_accelerator');
	BrakePedal = this.get_joint_id('pedal_brake');			
	DriverDoor = this.get_joint_id('door_l0');				
	
	this.register_event("vehicle/engine/reverse", ReverseAction); 
	this.register_event("vehicle/engine/on", EngineAction);
	this.register_axis("vehicle/controls/open", {minval:0, maxval: 1,center:0, vel:0.6}, function(v) {
		let doorAx = {z:-1};
		let doorAngle = v * 1.5;
		this.Geom.rotate_joint_orig(DriverDoor, doorAngle, doorAx);
	}); 
	
	//Add additional action handlers
	//Handle this action, when passing lights button is pressed ('L')
	this.register_axis("vehicle/lights/passing", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) {	
		//light_mask() function is used to turn lights on/off
		//1.param - light mask value - which bits/lights will be affected 
		//2.param - condition, when true, given bits/lights will be affected
		//Every time you press passing light button, parameter "v" switches it's value (in this case between minval and maxval, it can also switch between positions, if they are defined)	
		//if v===1, lights will turn on (light mask will affect the front and tail lights) 
		this.light_mask(0xf, v===1); 
		//if v===0, lights will turn off
		this.light_mask(0x0, v===0);
		//Note: this affects first 4 defined lights (because we didn't give an offset as 3. parameter). In this case it will affect 2 front lights and 2 tail lights, because every light is represented as 1 bit and as 1.parameter we used hexadecimal 0x0..0xf which works with 4 bits (0000....1111), we can also use 0x00..0xff which will work with first 8 bits
	});
		
	//Another way to use light mask, is to give light offset (in this case mainLightOffset) as 3. parameter, from this offset the light mask will affect next bits/lights
	//This action is handled, when you press Ctrl + L 
	this.register_axis("vehicle/lights/main", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) {	
		this.light_mask(0x3, v===1, mainLightOffset);
		this.light_mask(0x0, v===0, mainLightOffset);
	});

	//Handle this action, when turn signals buttons are pressed ('Shift' + 'A' or 'Shift' + 'D')
	//Turn signals can have -1/0/1 values, when 'Shift' + 'A' is pressed, the "v" value switches between 0 and -1, but when 'Shift' + 'D' is pressed, the value moves between 0 and 1.
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

	//Handle this action, when emergency lights buttons are pressed ('Shift' + 'W')
	//this.Emer ^= 1 toggles between 0 and 1
	this.register_event("vehicle/lights/emergency", function(v) { this.Emer ^= 1; });
	
	//Define light parameters and assign them to variable "lightProp"
	//Light parameters you can use:	
	//size (float)	 	- diameter of the light source (reflector)
	//angle (float)		- field of light [degrees] (ignored for point lights)
	//edge (float) 		- soft edge coefficient (0..1), portion of the light area along the edges where light fades to make it soft (ignored for point lights)
	//color (float3) 	- RGB color of the light emitter
	//intensity (float)	- light intensity
	//range (float) 	- effective light range
	//fadeout (float) 	- time to fade after turning off
	let lightProp = {size:0.19, angle:120, edge:0.2, fadeout:0.05, range:70 };
	//If either intensity or range are specified alone, the other one is computed automatically. If neither one is specified, the intensity is taken from the color value, otherwise the colors are normalized. If both intensity and range are specified, the light can have shorter range to avoid leaking outside of cockpits etc.
	//Note: intensity of light is affected not only by "intensity" param, but also by angle and range
	
	
	//Use add_spot_light() function to add lights
	//1.parameter - the model-space offset relative to bone or model pivot
	//2.parameter - light direction
	//3.parameter - light properties
	//Add front lights (offset relative to model pivot is given for the lights and direction is set to forward by {y:1})
	this.add_spot_light({x:-0.6,y:2.2,z:0.68}, {y:1}, lightProp); 	//left front light
	this.add_spot_light({x:0.6,y:2.2,z:0.68}, {y:1}, lightProp);	//right front light

	//Change the light properties in "lightProp" and use them for another lights
	lightProp = { size: 0.07, angle: 160, edge: 0.8, fadeout: 0.05, range: 150, color: { x: 1.0 } };
	
	//Add tail lights
	//You can also add 4. parameter (string name of the bone, to which you want to bind the light), this will make the lights offset and direction to be relative to the defined bone instead of model pivot (which is default)
	//The direction is now opposite to front lights (tail lights glow backwards) even though direction is still {y:1}, because the light is now relative to tail light bone, which has opposite direction
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightProp, "tail_light_l0"); 	//left tail light
	this.add_spot_light({x:0,y:0,z:0}, {y:1}, lightProp, "tail_light_r0"); 	//right tail light 	
	
	//Add brake lights
	lightProp = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100, color: { x: 1.0 } };
	
	//Here is another example to lights direction, brake lights are relative to model pivot but direction is {y:-1}, which is the opposite direction, therefore these lights will glow backwards
	//Add brake lights and store the offset of the first light in "brakeLightOffset"
	let brakeLightOffset =  
	this.add_spot_light({x:-0.38,y:-2.11,z:0.67}, {y:-1}, lightProp); 	//left brake light (0b01)
	this.add_spot_light({x:0.38,y:-2.11,z:0.67}, {y:-1}, lightProp); 	//right brake light (0b10)
	
	//Now we have to specify bit mask (BrakeMask), for that we have to use bit logic
	//Our 2 brake lights are defined as follows:
	//	01 - left brake light 
	//	10 - right brake light 
	//	11 - both brake lights  
	//We want the bit mask (BrakeMask) to affect both lights, therefore the given value will be "0b11" (in this case, the value is written in binary system, but it can also be written in decimal or hexadecimal system).
	//Also we want, that the mask starts affecting lights from the first brake light, therefore we have to "left shift" the bit mask by brake light offset
	BrakeMask = 0b11<<brakeLightOffset;

	//Add reverse lights
	lightProp = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100 };
	let revLightOffset =
	this.add_spot_light({x:-0.50,y:-2.11,z:0.72}, {y:-1}, lightProp);	//left reverse light (0b01)
	this.add_spot_light({x:0.50,y:-2.11,z:0.72}, {y:-1}, lightProp);	//right reverse light (0b10)
	
	// 01 (binary) -> 1 (decimal) - left reverse light 
	// 10 (binary) -> 2 (decimal) - right reverse light 
	// 11 (binary) -> 3 (decimal) - both reverse lights 
	//We want both lights to shine, when we hit the reverse button (in this case, the value is written in decimal system)
	RevMask = 3<<revLightOffset;

	//Add turn signal lights
	//In this case I used add_point_light() function, because we don't need this light to shine in given direction
	//add_point_light() also takes parameters position, and light properties, same as add_spot_light(), but without direction 
	lightProp = {size:0.1, edge:0.8, fadeout:0, color:{x:0.4,y:0.1,z:0},range:0.004,  intensity:1 };
	let turnLightOffset =
	this.add_point_light({x:-0.71,y:2.25,z:0.63}, lightProp); 	//left front turn light (0b0001)
	this.add_point_light({x:-0.64,y:-2.11,z:0.72}, lightProp); 	//left rear turn light (0b0010)
	this.add_point_light({x:0.71,y:2.25,z:0.63}, lightProp); 	//right front turn light (0b0100)
	this.add_point_light({x:0.64,y:-2.11,z:0.72}, lightProp); 	//right rear turn light (0b1000)
	
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
	//We want lights on the side of the car to glow, when we hit the corresponding left or right turn signal button (in this case, the value is written in hexadecimal system )
	//When the left turn signal button was pressed, we want turn lights on the left side to glow 
	TurnLeftMask = 0x3<<turnLightOffset;
	//When the right turn signal button was pressed, we want turn lights on the right side to glow 
	//Add number of previous left turn lights to the offset (or you can make another offset for right turn lights and use that....) 
	TurnRightMask = 0x3<<(turnLightOffset + 2);
	
	//add main lights
	//Here you don't have to identify lights for bit mask, because they were added as 4.parameter in add_spot_light() function while creating action handler
	lightProp = { size: 0.05, angle: 110, edge: 0.08, fadeout: 0.05, range: 110 };
	let mainLightOffset = 
	this.add_spot_light({x:-0.45,y:2.2,z:0.68}, {y:1}, lightProp); 	//left main light
	this.add_spot_light({x:0.45,y:2.2,z: 0.68}, {y:1}, lightProp);	//right main light

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
	this.Time = 0;
	this.Lturn = this.Rturn = this.Emer = 0;
	
	this.Geom = this.get_geomob(0);
	this.Started = 0;
	this.EngDir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});

}

function ReverseAction(v)
{
	this.EngDir = this.EngDir>=0 ? -1 : 1;
	this.fade(this.EngDir>0 ? "Forward" : "Reverse");
	
	//Apply reverse light mask (RevMask) on reverse lights, when engine direction has value -1 (activate reverse lights)
	this.light_mask(RevMask, this.EngDir<0);
}

function EngineAction()
{
	this.Started = this.Started === 0 ? 1 : 0;
	this.fade(this.Started === 1  ? "Engine ON" : "Engine OFF");
}

function update_frame(dt, engine, brake, steering, parking)
{
	let brakeAx = {x:1};
	let brakeAngle = brake*0.4;	
	let accelAx = {y:(-engine*0.02), z:(-engine*0.02)}
	this.Geom.rotate_joint_orig(BrakePedal, brakeAngle, brakeAx);
	this.Geom.move_joint_orig(AccelPedal, accelAx)
	
	let kmh = this.speed()*3.6;
	
	if (this.Started === 1)
	{
	let redux = this.EngDir>=0 ? 0.2 : 0.6;
	engine = EngineForce*Math.abs(engine);
	let force = (kmh>=0) === (this.EngDir>=0)
		? engine/(redux*Math.abs(kmh) + 1)
		: engine;
	force -= ForceLoss;
	force = Math.max(0.0, Math.min(force, engine));
	engine = force * this.EngDir;
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

	//Apply brake light mask (BrakeMask) on brake lights, when brake value is bigger than 0
	//Note: add this code before adding rolling friction to brakes.
	this.light_mask(BrakeMask, brake>0);

	brake *= BrakeForce; 
	//Rolling friction
	brake += 200;
	this.wheel_brake(-1, brake);
	this.animate_wheels();

	if(this.Lturn || this.Rturn || this.Emer)
	{
		//Calculate blinking time for turn signal lights
		this.Time += dt;
		let blt = this.Time*0.85;
		//for turn lights blinking effect
		let blink = (blt - Math.floor(blt)) > 0.47 ? 1 : 0;
		
		//Apply light mask for turn lights, depending of which action was handled (left turn lights, right turn lights or all turn lights (emergency)), which will then turn on and off, depending on the "blink" value
		this.light_mask(TurnLeftMask, (blink&(this.Lturn|this.Emer)) );
		this.light_mask(TurnRightMask, (blink&(this.Rturn|this.Emer)) );
	}
	else
	{
		//To turn off the active turn lights
		this.light_mask(TurnLeftMask, false);
		this.light_mask(TurnRightMask, false);
	}
}

