//*****Version 2 - Animations*****

var Wheels = {
    FLwheel : 0,	
    FRwheel : 1,	
	RLwheel : 2,	
    RRwheel : 3		 
}

//Declare additional global variables
var SteerWheel, SpeedGauge, AccelPedal, BrakePedal, DriverDoor;
var SpeedGaugeMin = 10.0;
var RadPerKmh = 0.018325957;

var EngineForce = 15000.0;
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
	
	Wheels.FLwheel = this.add_wheel('wheel_l0', wheelparam); 
	Wheels.FRwheel = this.add_wheel('wheel_r0', wheelparam); 
	Wheels.RLwheel = this.add_wheel('wheel_l1', wheelparam); 
	Wheels.RRwheel = this.add_wheel('wheel_r1', wheelparam);
	
	//get_geomob() is used to access instances geometry (with this you can get joints/bones), takes as parameter ID of geometry object (default 0)
	var body = this.get_geomob(0);

	//Get joints/bones (first parameter) from geomob and assign them to variables
	SteerWheel = body.get_joint('steering_wheel');		//Steering wheel
	SpeedGauge = body.get_joint('dial_speed');			//Speed gauge 
	AccelPedal = body.get_joint('pedal_accelerator');	//Accelerator pedal
	BrakePedal = body.get_joint('pedal_brake');			//Brake pedal
	DriverDoor = body.get_joint('door_l0');				//Driver's door
	
	this.register_event("car/engine/reverse", ReverseAction); 
	
	//Another way to use action handlers, is to directly write the definition, instead of calling another function
	//Declare additional action handler to open/close driver's door (when 'O' is pressed)
	this.register_axis("car/controls/open", {minval:0, center:0, vel:0.6}, function(v) {
		//Define around which axis and in which direction the door will move
		var doorax = {z:-1};
		//Multiplied with 1.5 to wide open door
		var doorangle = v * 1.5;
		//rotate_joint_orig() is used to rotate joint, first parameter is the joint you want to rotate, second parameter specifies the angle, to which it should rotate and third parameter is the rotation axis (in this case it rotates around Z axis) and the direction of rotation (-1 or 1)
		this.geom.rotate_joint_orig(DriverDoor, doorangle, doorax);
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
	//Initialize geomob variable
	this.geom = this.get_geomob(0);
	
	this.engdir = 1;
	this.started = 0;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
}

function engine(start)
{
	if(start) 
	{
		this.started=1;
	}
	else 
	{
		this.started=0;
	}
}

function ReverseAction(v)
{
	this.engdir = this.engdir>=0 ? -1 : 1;
	this.fade(this.engdir>0 ? "forward" : "reverse");
}

function update_frame(dt, engine, brake, steering, parking)
{
	//Define additional local variables
	var brakeax = {x:1};
	//Brake pedal rotation angle will depend on the brake value
	var brakeangle = brake*0.4;	
	//You can also use more than one axis
	var accelax = {y:(-engine*0.02), z:(-engine*0.02)}
	
	//Rotate brake pedal
	this.geom.rotate_joint_orig(BrakePedal, brakeangle, brakeax);
	
	//move_joint_orig() is used to move joint, first parameter is the joint you want to rotate and second parameter is the rotation axis and direction
	this.geom.move_joint_orig(AccelPedal, accelax)
	
	var kmh = Math.abs(this.speed()*3.6);
	var redux = this.engdir>=0 ? 0.2 : 0.6;
	engine = EngineForce*Math.abs(engine);
	var force = (this.engdir>=0) == (kmh>=0)
		? engine/(redux*Math.abs(kmh) + 1)
		: engine;
	force -= ForceLoss;
	force = Math.max(0.0, Math.min(force, engine));
	engine = force * this.engdir;
	
	if(this.started>0) 
	{
		this.wheel_force(Wheels.FLwheel, engine);
		this.wheel_force(Wheels.FRwheel, engine);
	}
	
	//Rotate speed gauge
	if(kmh > SpeedGaugeMin)
	{
        this.geom.rotate_joint_orig(SpeedGauge, (kmh - SpeedGaugeMin) * RadPerKmh, {x:0,y:1,z:0});    
    }
	
	steering *= 0.6;
	this.steer(Wheels.FLwheel, steering);	//front left wheel
	this.steer(Wheels.FRwheel, steering);	//front right wheel

	//Rotate steering wheel (first prameter is bone/joint you want to rotate, second parameter is the rotation angle and third parameter (must be in {} brackets) is the axis (around which you want to rotate, in this case you rotate around Z axis) and the direction of rotation (-1 or 1))
	this.geom.rotate_joint_orig(SteerWheel, 8*steering, {z:1});

	brake *= BrakeForce; 
	this.wheel_brake(-1, brake);
	
	//Function used for animating wheels
	this.animate_wheels();
	//This method simplifies the animation of wheels for basic cases, without needing to animate the model via the geomob
}
