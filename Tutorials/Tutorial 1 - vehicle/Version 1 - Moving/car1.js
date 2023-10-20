//*****Version 1 - Moving*****

//Define global variables (global variables will be mainly used to store values, that don't change at runtime by given instance, like const used for calculations, ID of wheels, sounds, emitters, bones etc.)
let FLwheel, FRwheel, RLwheel, RRwheel;
const EngineForce = 25000.0;
const BrakeForce = 5000.0;
const MaxKmh = 200;
const ForceLoss = EngineForce / (0.2*MaxKmh + 1);

function init_chassis()
{ 
	//Define physical wheel parameters
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

	//Bind model wheel joint/bone and add wheel parameters
	//Function add_wheel() returns ID of the wheel, that is later used in code
	//1.parameter - wheel joint/bone to which you want to bind 
	//2.parameter - wheel physical parameters 
	//Note: You can find model bones in "Scene editor"->"Entity properties"->"Skeleton".
	FLwheel = this.add_wheel('wheel_l0', wheelParam); //front left wheel (will have ID 0)
	FRwheel = this.add_wheel('wheel_r0', wheelParam); //front right wheel (will have ID 1)
	RLwheel = this.add_wheel('wheel_l1', wheelParam); //rear left wheel (will have ID 2)
	RRwheel = this.add_wheel('wheel_r1', wheelParam); //rear right wheel (will have ID 3)

	//Add action handlers
	//In this case call ReverseAction function, when reverse button is presssed ('R') (these inputs are binded in vehicle.cfg file, more info in "Version 0 - Info" or Outerra wiki)
	this.register_event("vehicle/engine/reverse", ReverseAction);
	
	//In this case call EngineAction function, when engine ON/OFF button is presssed ('E')
	this.register_event("vehicle/engine/on", EngineAction);

	//Return parameters (if not defined, they will be set to default parameters)
	return {
		mass: 1120.0,
		com: {x: 0.0, y: -0.2, z: 0.3},
		steering_params:{
			radius: 0.17
		}
	};
}

//define per-instance parameters in init_vehicle()
function init_vehicle()
{
	//Initialize variables for your instance (use "this" keyword for changes to affect only instance of sample car, you are currently in) 
	//this.Started creates "Started" variable for all instances of sample car, but future changes will affect only the current instance of an object, in which the code is being executed
	this.Started = 0;
	this.EngDir = 1;
	//"this" is also used to refer to already defined members from vehicle interface, such as set_fps_camera_pos() function (other members can be found on Outerra wiki) 
	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
}



//Called, when reverse button is pressed ('R')
//Function can take "v" parameter through action handler (not used in this case)
function ReverseAction(v)
{
	//Check, what is the engine direction and switch it (ternary operator)
	this.EngDir = this.EngDir>=0 ? -1 : 1;
	//Write fading message on the screen
	this.fade(this.EngDir>0 ? "Forward" : "Reverse");
}

//Invoked when engine has started or stopped (when you turn on/off the engine by pressing 'E'))
function EngineAction()
{
	this.Started = this.Started === 0 ? 1 : 0;
	this.fade(this.Started === 1  ? "Engine ON" : "Engine OFF");
}


//Invoked each frame to handle the internal state of the object
function update_frame(dt, engine, brake, steering, parking)
{
	//Define local variable and use speed() function to get current speed in m/s (to get km/h, multiply the value by 3.6) 
	let kmh = this.speed()*3.6;

	//Calculate force, but only if the car has started
	if (this.Started === 1)
	{
		//Calculate force, which will be applied on wheels to move the car
		let redux = this.EngDir>=0 ? 0.2 : 0.6;
		//You can use Math library
		engine = EngineForce*Math.abs(engine);
		//Calculate force value, which depends on if the car should move in forward or backward direction
		let force = (kmh>=0) === (this.EngDir>=0)
			? engine/(redux*Math.abs(kmh) + 1)
			: engine;
		//Add wind resistance
		force -= ForceLoss;
		//Make sure, that force can not be negative
		force = Math.max(0.0, Math.min(force, engine));
		
		//Calculate force and direction, which will be used to add force to wheels
		engine = force * this.EngDir;
	}
	
	//Use wheel_force() function to apply propelling force on wheel
	//wheel_force() function is used to move the car (apply a propelling force on wheel)
	//1.parameter - wheel, you want to affect (takes the wheel ID, in this case, the car has front-wheel drive)
	//2.parameter - force, you want to exert on the wheel hub in forward/backward  direction (in Newtons)
	this.wheel_force(FLwheel, engine);
	this.wheel_force(FRwheel, engine);
	//You can also use -1 as 1.parameter (instead wheel ID), this will affect all wheels
	//Example: this.wheel_force(-1, engine)


	//Define steering sensitivity
	steering *= 0.3;
	
	//Steer wheels by setting angle
	//1.parameter - wheel you want to affect (it takes the wheel ID number as argument)
	//2.parameter - angle in radians to steer the wheel
	this.steer(FLwheel, steering);	//front left wheel
	this.steer(FRwheel, steering);	//front right wheel
	//You can also use -2 as 1.parameter (instead of wheel ID), this will affect first two wheels
	//Example: this.steer(-2, steering)
	
	//Calculate the brake value, which will affect the wheels when you are braking
	//Originally "brake" has value between 0..1, you have to multiply it by "BrakeForce" to have enough force to brake
	brake *= BrakeForce;
	
	//Add rolling friction
	brake += 200;
	
	//wheel_brake() function is used to apply braking force on given wheels
	//1.parameter - wheel ID (in this case we want to affect all wheels, therefore -1)
	//2.parameter - braking force, you want to exert on the wheel hub (in Newtons)
	this.wheel_brake(-1, brake);
}

