//*****Version 0 - Info*****

//Welcome to the first tutorial on JavaScript programming in Outerra.
//We recommend reviewing the Outerra/anteworld tutorial on Github ( https://github.com/Outerra/anteworld/wiki/Tutorial-1-%E2%80%90-car-(javascript) ) before proceeding, as it provides detailed information.

//You should use at least init_chassis, init_vehicle and update_frame functions.

//Function init_chassis is invoked, when the model is first time loaded. 
//Can take extra parameters (string "param") from .objdef file.
//It is used to define the vehicle construction, binding bones, adding wheels, lights, sounds, sound emitters, action handlers, etc. (They can be defined only in init_chassis).
function init_chassis(param)
{ 
	//Basic wheel parameters you can use:
	//	radius - outer tire radius [m] (default: 1)
	//	width - tire width [m] (default: 0.2)
	//	suspension_max - max.movement up from default position [m] (default: 0.01)
	//	suspension_min - max.movement down from default position [m] (default: -0.01)
	//	suspension_stiffness - suspension stiffness coefficient (default: 5)
	//	damping_compression - damping coefficient for suspension compression (default: 0.06)
	//	damping_relaxation - damping coefficient for suspension relaxation (default: 0.04)
	//	slip_lateral_coef - proportion of the slip used for sideways direction (default: 1.5)
	//	grip - relative tire grip compared to an avg.tire, +-1 (default: 0)
	//	differential - true if the wheel is paired with another through a differential (default: true)
	
	let wheel_params = {
		radius: 0.31515,
		width: 0.2,
		suspension_max: 0.1,
		suspension_min: -0.04,
		suspension_stiffness: 50.0,
		damping_compression: 0.4,
		damping_relaxation: 0.12,
		grip: 1,
	};

	//Action handlers (events, which are called, when binded input/object changed it's state).
	//Extended controls allow vehicles and aircraft to bind to any input actions they want to handle, while the user can configure multiple custom keyboard and joystick bindings to the actions.
	//For interactive/VR mode it is also possible to bind to an object/knob without using input.
	
	//Action handlers can be of two types: event and value 
	
	//Event handlers  are invoked on key or button press (for example a fire action)
	//Examples:	this.register_event("air/controls/eject", function(v){...});
	//			this.register_button("air/controls/eject", function(v){...});
	//Note: register_button is helper function for creating buttons, that should return to default position after releasing (without the need to use register_axis and add "options" parameters).
	
	//Value handlers are receiving a tracked value which is by default in -1..1 range. If user has the action bound to a joystick axis, the handler will receive the axis position. If it's bound to keyboard or joystick buttons, the value is tracked internally depending on the binding modes and the handler configuration.
	//Example:	this.register_axis("air/heli/collective", options, function(v,dt){...});
	
	//Action handler parameters
	//Action handlers can have different parameters, depending on which handler you use
	
	//register_event can have 4 parameters:	
	//	action - path to input binding (example: vehicle/lights/cabin - "vehicle" is the name of .cfg file, in which we have the binding defined, "lights" is name of the group, to which it belongs and "cabin" is the name of the bound action (this action is bound to shift+L))
	//	handler - function, which is invoked whenever given event occurs or the tracked value changes as the result of the button state. 
    //              This function can have 3 parameters:	"v" - the action value itself(integer for events or floating point for values)
	//				"dt" - elapsed time in seconds since the last frame 
	//				"ch" - is a channel id, which is useful for actions that can adress multiple channels (id 0 should be interpreted as "all channels", so that it remains compatible with single channel bindings
	//	group (optional) - event group id, in case you want to enable/disable groups of actions on seat change
	//	channels (optional) - number of extra channels supported by the handler
	//register_event(action, handler, group, channels)
	
	//register_axis can have 5 parameters:	
	//	action - *explained above*
	//	options (required, but can be empty {} ) - optional properties, which determine how the script wants the action value to behave when controlled by keyboard or buttons
	//	handler - *explained*
	//	defval (optional) - *explained above*
	//	group (optional) - *explained above*
	//register_axis(action, options, handler, defval, group)
	
	//Options properties in register_axis, you can use:	
	//	vel - max rate of value change (saturated) per second during key press (defines how fast the axis value changes when key is pressed)
	//	acc - max rate of initial velocity change per second during key press (controls the key press response swiftnes)
	//	center - centering speed per second, 0 for no centering (defines how fast the axis value returns to default, when key is released)
	//	minval - minimum value to clamp to, it's often desired to have a 0..1 range instead of the default -1..1 range (should be >= -1 for compatibility with joystick)
	//	maxval - maximum value to clamp to (should be <= +1 for compatibility with joystick)
	//	positions - number of positions between minval and maxval, for example light switch on train has 4 positions, his minval is 0 and maxval is 3, when coresponding input is pressed, the switch will jump between 0,1,2 and 3.  
	//	channels - number of extra channels supported by the handler (max 7)
	//The vel and center values can be negative, in that case the actual speed is multiplied by -e^(-kmh/steering_threshold). This can be used for steering, with centering going slower at slower vehicle speeds or to slow down steering speed at higher vehicle speeds.
	
	//In this case call reverse_action function, when reverse button is presssed ('R')
	this.register_event("vehicle/engine/reverse", reverse_action);
		
	//Warning: Action handlers can be handled by engine or by user (but not both), some handlers are automatically handled by engine (for example default parameters used by update_frame function, such as: dt, engine, brake, steering and parking)
	//When you declare an action handler for an event that the engine already handles internally, you take over control of handling that event, and the engine will no longer manage it internally.
	//Example: here we have declared action handler, that checks, if user pressed brake button ('S'), originally it would be automatically handled by engine, and in update_frame function you would get the braking value from "brake" parameter, which can be then used, but now the "brake" value in update_frame will be always 0.
	this.register_event("vehicle/controls/brake", brake_action);
	
	//Return parameters
	//You should assign some return parameters, because if not, they will be set to default parameters
	//Return parameters you can define:
	//	mass - vehicle mass in kg (default: 1000kg)
	//	com - center of mass offset [float3,m] - displacement in model space, for example com:{z:0.6} shifts the center 0.6m up from the model pivot (default: {x:0,y:0,z:0})
	//  clearance - clearance from ground, default wheel radius
    //  bumper_clearance - clearance in front and back (train bumpers)
    //	steering_params (structure): steering_ecf - speed [km/h] at which the steering angle is reduced by 60% (default: 50)
	//								 centering_ecf - speed [km/h] when the centering acts at 60% already (default: 20)
	return {
		mass: 1120.0,
		com: {x: 0.0, y: -0.2, z: 0.3},
		steering_params:{
			steering_ecf: 50,
		},
	//Effective steering angle is reduced with vehicle speed by exp(-kmh/steering_threshold). Wheel auto-centering speed depends on the vehicle speed by 1 - exp(-kmh/centering_threshold), e.g with higher speed the wheel centers faster
	};
}

//"this." is used to refer to the current instance of an object, in which the code is being executed

//Invoked for each new instance of the vehicle (including the first one) and it can be used to define per-instance parameters.
function init_vehicle()
{	
	//Fps camera position should be defined in init_vehicle function
	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
	//Camera position can be also set at camera bone location, if model has any
	//Example: this.set_fps_camera_pos(cam_fps);
}

//Called, when reverse button is pressed ('R')
//Function gets parameter "v" through action handler
function reverse_action(v)
{
	
}

//Called, when brake button is pressed ('S')
function brake_action(v)
{
	
}

//Invoked each frame to handle the internal state of the object
//Function has following parameters: 	
//	dt - delta time from previous frame 
//	engine - gas pedal state (0..1)
//	brake - brake pedal state (0..1)
//	steering - steering state (-1..1)
//	parking - hand brake state (0..1)
//Warning: update_frame updates these parameters only in case, they are not handled by user (mentioned in init_chassis, while declaring action handlers) 
//When action handlers for these parameters are declared, the engine no longer manages them internally, causing function to receive value 0 for the parameter.
function update_frame(dt, engine, brake, steering, parking)
{
	//Note: for debugging purposes, you can use $log() to write info on console. 
	//For example, in update_frame function, you can write the brake value on console each frame
	//$log("Brake value: " + brake); 

}

/*
## simulation_step()
Function simulation_step() can be used instead of update_frame, this function is invoked 60 times per second 
```c
function simulation_step(dt)
{	
}
```
*/
