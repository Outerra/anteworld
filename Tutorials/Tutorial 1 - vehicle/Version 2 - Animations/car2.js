//*****Version 2 - Animations*****
// This tutorial is for adding simple animations using geomob.

//Declare additional global variables
let SteerWheel, SpeedGauge, AccelPedal, BrakePedal, DriverDoor;
const SpeedGaugeMin = 10.0;
const RadPerKmh = 0.018325957;

let FLwheel, FRwheel, RLwheel, RRwheel;
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
	
	//get_geomob() is used to access instance geometry interface (with this you can get joints/bones IDs)
	//parameter - ID of geometry object (default 0)
	let body = this.get_geomob(0);

	//Get joints/bones IDs from geomob interface
	SteerWheel = body.get_joint('steering_wheel');		//Steering wheel
	SpeedGauge = body.get_joint('dial_speed');			//Speed gauge 
	AccelPedal = body.get_joint('pedal_accelerator');	//Accelerator pedal
	BrakePedal = body.get_joint('pedal_brake');			//Brake pedal
	DriverDoor = body.get_joint('door_l0');				//Driver's door
	
	this.register_event("vehicle/engine/reverse", ReverseAction); 
	this.register_event("vehicle/engine/on", EngineAction);
	
	//Another way to use action handlers, is to directly write the functionality, instead of calling another function
	//Declare additional action handler to open/close driver's door (when 'O' is pressed)
	//In this case, use register_axis(), here you can change the opening range, speed and other parameters (more in "Version 0 - Info" or Outerra Wiki) 
	this.register_axis("vehicle/controls/open", {minval:0, maxval: 1, center:0, vel:0.6}, function(v) {
		//Define around which axis and in which direction the door will move
		let doorAx = {z:-1};
		//Multiplied with 1.5 to wide open door
		let doorAngle = v * 1.5;
		//rotate_joint_orig() is used to rotate joint by given angle and back to default position
		//1.parameter - bone/joint ID
		//2.parameter - vec rotation angle in radians
		//3.parameter - rotation axis vector (must be normalized) - axis around which the bone rotates (in this case around Z axis) and the direction of rotation (-1...1)
		this.Geom.rotate_joint_orig(DriverDoor, doorAngle, doorAx);
		//Note: action handlers use geomob (Geom) functionality for current instance, which was initialized in init_vehicle()
	}); 

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
	//Get instance geometry interface, which will be used for current instance (to rotate bone, move bone, etc. )
	this.Geom = this.get_geomob(0);
	
	this.Started = 0;
	this.EngDir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
}

function ReverseAction(v)
{
	this.EngDir = this.EngDir>=0 ? -1 : 1;
	this.fade(this.EngDir>0 ? "Forward" : "Reverse");
}

function EngineAction()
{
	this.Started = this.Started === 0 ? 1 : 0;
	this.fade(this.Started === 1  ? "Engine ON" : "Engine OFF");
}

function update_frame(dt, engine, brake, steering, parking)
{
	//Define additional local variables
	let brakeAx = {x:1};
	//Brake pedal rotation angle will depend on the brake value
	let brakeAngle = brake*0.4;	
	//You can also use more than one axis
	let accelAx = {y:(-engine*0.02), z:(-engine*0.02)}
	
	//Rotate brake pedal
	this.Geom.rotate_joint_orig(BrakePedal, brakeAngle, brakeAx);
	
	//move_joint_orig() is used to move joint
	//1.parameter - joint you want to move
	//2.parameter - movement axis and direction
	this.Geom.move_joint_orig(AccelPedal, accelAx)
	
	let kmh = Math.abs(this.speed()*3.6);
	
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
	
	//Rotate speed gauge
	if(kmh > SpeedGaugeMin)
	{
        this.Geom.rotate_joint_orig(SpeedGauge, (kmh - SpeedGaugeMin) * RadPerKmh, {x:0,y:1,z:0});    
    }
	
	steering *= 0.3;
	this.steer(FLwheel, steering);	//front left wheel
	this.steer(FRwheel, steering);	//front right wheel

	//Rotate steering wheel 
	//1.prameter - bone/joint you want to rotate
	//2.parameter - rotation angle 
	//3.parameter (must be in {} brackets) - axis, around which you want to rotate (in this case you rotate around Z axis) and the direction of rotation (-1 or 1))
	this.Geom.rotate_joint_orig(SteerWheel, 10.5*steering, {z:1});

	brake *= BrakeForce; 
	brake += 200;
	this.wheel_brake(-1, brake);
	
	//Function used for animating wheels
	this.animate_wheels();
	//This method simplifies the animation of wheels for basic cases, without needing to animate the model via the geomob
}
