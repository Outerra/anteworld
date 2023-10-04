//*****Version 1 - Moving*****

//Define global variables 
var FLwheel, FRwheel, RLwheel, RRwheel, Started;
var EngineForce = 25000.0;
var BrakeForce = 5000.0;
var MaxKmh = 200;
var ForceLoss = EngineForce / (0.2*MaxKmh + 1);

function init_chassis()
{ 
	//Define physical wheel parameters
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

	//Bind to model wheel joint/bone (first parameter) and add physical parameters (second parameter) 
	//Function add_wheel() returns integer, which will be represented as ID of the wheel, that is later used in code( some functions take this ID as parameter(steer, wheel_force, wheel_brake etc.))
	FLwheel = this.add_wheel('wheel_l0', wheelparam); //front left wheel (will have ID 0)
	FRwheel = this.add_wheel('wheel_r0', wheelparam); //front right wheel (will have ID 1)
	RLwheel = this.add_wheel('wheel_l1', wheelparam); //rear left wheel (will have ID 2)
	RRwheel = this.add_wheel('wheel_r1', wheelparam); //rear right wheel (will have ID 3)

	//Declare action handlers (inputs)
	//In this case call ReverseAction function, when reverse button is presssed ('R')
	this.register_event("car/engine/reverse", ReverseAction);
	
	//In this case call EngineAction function, when engine ON/OFF button is presssed ('E')
	this.register_event("car/engine/on", EngineAction);

	//Return parameters (you should define some return parameters, because if not, they will be set to default parameters)
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
	//Initialize variables for your instance
	Started = 0;
	this.engdir = 1;
	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
}



//Called, when reverse button is pressed ('R')
//Function can take "v" parameter (button state) through action handler (not used in this case)
function ReverseAction(v)
{
	//Check, what is the engine direction and switch it (ternary operator)
	this.engdir = this.engdir>=0 ? -1 : 1;
	//Write fading message on the screen
	this.fade(this.engdir>0 ? "forward" : "reverse");
}

//Invoked when engine has started or stopped (when you turn on/off the engine by pressing 'E'))
function EngineAction()
{
	
	Started = Started == 0 ? 1 : 0;
	//Write fading message on the screen
	this.fade(Started == 1  ? "Engine ON" : "Engine OFF");
}


//Invoked each frame to handle the internal state of the object
function update_frame(dt, engine, brake, steering, parking)
{
	//Define local variable
	var kmh = this.speed()*3.6;

	//Calculate force, only when car has started
	if (Started == 1)
	{
		//Calculate force, which will be applied on wheels to move the car
		var redux = this.engdir>=0 ? 0.2 : 0.6;
		engine = EngineForce*Math.abs(engine);
		var force = (this.engdir>=0) == (kmh>=0)
			? engine/(redux*Math.abs(kmh) + 1)
			: engine;
		force -= ForceLoss;
		force = Math.max(0.0, Math.min(force, engine));
		
		//Calculate force and direction, which will be used to add force to wheels
		engine = force * this.engdir;
	}
	
	//Apply force on wheels(when you press "W")
	//wheel_force() function is used to move the car (apply a propelling force on wheel), where the first parameter is the wheel, you want to affect (takes the wheel ID, in this case, the car has front-wheel drive) and second parameter is the force, you want to exert on the wheel hub in fwd/bkw direction (in Newtons)
	this.wheel_force(FLwheel, engine);
	this.wheel_force(FRwheel, engine);
	//In wheel_force() function, you can also use -1 as first parameter (instead wheel ID), this will affect all wheels
	//Example: this.wheel_force(-1, engine)


	//Define steering sensitivity
	steering *= 0.3;
	
	//Steer wheels by setting angle
	//First parameter is the wheel you want to affect (it takes the wheel ID number as argument) and second parameter is the angle in radians to steer the wheel
	this.steer(FLwheel, steering);	//front left wheel
	this.steer(FRwheel, steering);	//front right wheel
	//In this.steer() function, you can also use -2 as first parameter (instead of wheel ID), this will affect first two wheels
	//Example: this.steer(-2, steering)
	
	//Calculate the brake value, which will affect the wheels when you are braking
	//Originally "brake" has value between 0..1, you have to multiply it by "BrakeForce" to have enough force to brake
	brake *= BrakeForce;
	
	//Add rolling friction
	brake += 200;
	
	//wheel_brake() function is used to apply braking force on wheels, where the first parameter is the wheel (in this case we want to affect all wheels, therefore -1) and second parameter is braking force, you want to exert on the wheel hub (in Newtons)
	this.wheel_brake(-1, brake);
}

