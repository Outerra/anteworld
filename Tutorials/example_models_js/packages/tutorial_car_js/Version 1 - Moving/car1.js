//*****Version 1 - Moving*****

//Define global variables (global variables will be mainly used to store values, that don't change at runtime by given instance, like const values used for calculations, ID of wheels, sounds, emitters, bones etc.)
const ENGINE_FORCE = 25000.0;
const BRAKE_FORCE = 5000.0;
const MAX_KMH = 200;
const FORCE_LOSS = ENGINE_FORCE / (0.2*MAX_KMH + 1);

//It is better to group related variables within objects, as it enhances modularity and creates structured and understandable code.
let wheels = {
    FLwheel : -1, 
    FRwheel : -1, 
    RLwheel : -1, 
    RRwheel : -1, 
};
//Note: for debug purpose, it's better to define these variables to -1

//Called, when reverse button is pressed ('R')
//Function can take "v" parameter through action handler (not used in this case)
function reverse_action(v)
{
	//Check, what is the engine direction and switch it (ternary operator)
	this.eng_dir = this.eng_dir >= 0 ? -1 : 1;
	//Write fading message on the screen
	this.fade(this.eng_dir > 0 ? "Forward" : "Reverse");
}

//Invoked when engine has started or stopped (when you turn on/off the engine by pressing 'E'))
function engine_action()
{
	this.started = this.started === 0 ? 1 : 0;
	this.fade(this.started === 1  ? "Engine start" : "Engine stop");
}

function init_chassis()
{ 
	//Define physical wheel parameters
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

	//Bind model wheel joint/bone and add wheel parameters
	//Function add_wheel returns ID of the wheel
	//1.parameter - wheel joint/bone to which you want to bind 
	//2.parameter - wheel physical parameters 
	//Note: You can find model bones in "Scene editor"->"Entity properties"->"Skeleton".
	wheels.FLwheel = this.add_wheel('wheel_l0', wheel_params); //front left wheel (will have ID 0)
	wheels.FRwheel = this.add_wheel('wheel_r0', wheel_params); //front right wheel (will have ID 1)
	wheels.RLwheel = this.add_wheel('wheel_l1', wheel_params); //rear left wheel (will have ID 2)
	wheels.RRwheel = this.add_wheel('wheel_r1', wheel_params); //rear right wheel (will have ID 3)

	//Add action handlers
	//In this case call reverse_action function, when reverse button is presssed ('R') (these inputs are binded in vehicle.cfg file, more info in "Version 0 - Info" or Outerra wiki)
	this.register_event("vehicle/engine/reverse", reverse_action);
	
	//In this case call engine_action function, when engine ON/OFF button is presssed ('E')
	this.register_event("vehicle/engine/on", engine_action);

	//Return parameters (if not defined, they will be set to default parameters)
	return {
		mass : 1120.0,
		com: {x: 0.0, y: -0.2, z: 0.3},
		steering_params:{
			steering_ecf: 50,
		},
	};
}

//define per-instance parameters in init_vehicle function
function init_vehicle()
{
	//Initialize variables for your instance (use "this" keyword for changes to affect only instance of sample car, you are currently in) 
	//this.started creates "started" variable for all instances of sample car, but future changes will affect only the current instance of an object, in which the code is being executed
	this.started = 0;
	this.eng_dir = 1;
	//"this" is also used to refer to already defined members from vehicle interface, such as set_fps_camera_pos function (other members can be found on Outerra wiki) 
	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
}

//Invoked each frame to handle the internal state of the object
function update_frame(dt, engine, brake, steering, parking)
{
	//Define local variable and use speed function to get current speed in m/s (to get km/h, multiply the value by 3.6) 
	let kmh = this.speed()*3.6;

	//Calculate force, but only if the car has started
	if (this.started === 1)
	{
		//Calculate force, which will be applied on wheels to move the car
		let redux = this.eng_dir>=0 ? 0.2 : 0.6;
		//You can use Math library
		engine = ENGINE_FORCE * Math.abs(engine);
        //Determine the force value based on whether the car should move forward or backward
		let force = (kmh >= 0) === (this.eng_dir >= 0)
			? engine / (redux * Math.abs(kmh) + 1)
			: engine;
		//Add wind resistance
		force -= FORCE_LOSS;
		//Make sure, that force can not be negative
		force = Math.max(0.0, Math.min(force, engine));
		
		//Calculate force and direction, which will be used to add force to wheels
		engine = force * this.eng_dir;
	}
	
	//Use wheel_force function to apply propelling force on wheel
	//wheel_force is used to apply a propelling force on wheel and move the car.
	//1.parameter - wheel, you want to affect (takes the wheel ID, in this case, the car has front-wheel drive)
	//2.parameter - force, you want to exert on the wheel hub in forward/backward  direction (in Newtons)
	this.wheel_force(wheels.FLwheel, engine);
	this.wheel_force(wheels.FRwheel, engine);
	//You can also use -1 as 1.parameter (instead wheel ID), this will affect all wheels
	//Example: this.wheel_force(-1, engine)


	//Define steering sensitivity
	steering *= 0.3;
	
	//Steer wheels by setting angle
	//1.parameter - wheel you want to affect (it takes the wheel ID number as argument)
	//2.parameter - angle in radians to steer the wheel
	this.steer(wheels.FLwheel, steering);	//front left wheel
	this.steer(wheels.FRwheel, steering);	//front right wheel
	//You can also use -2 as 1.parameter (instead of wheel ID), this will affect first two wheels
	//Example: this.steer(-2, steering)
	
	//Calculate the brake value, which will affect the wheels when you are braking
	//Originally "brake" has value between 0..1, you have to multiply it by "BRAKE_FORCE" to have enough force
	brake *= BRAKE_FORCE;
	
	//rolling friction
	brake += 200;
	
	//wheel_brake function is used to apply braking force on given wheels
	//1.parameter - wheel ID (in this case we want to affect all wheels, therefore -1)
	//2.parameter - braking force, you want to exert on the wheel hub (in Newtons)
	this.wheel_brake(-1, brake);
}
