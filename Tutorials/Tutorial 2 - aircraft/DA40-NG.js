var Propeller, WheelFront, WheelRight, WheelLeft, ElevatorRight, ElevatorLeft, Rudder, AileronRight, AileronLeft, PropBlur;

//Used for joint rotation
var PI = 3.14159265358979323846;

//Declare sound variables in enum, which will be represented as numbers (this is required, because some functions take integer parameters (play_sound, play_loop, is_looping etc.)).
var Snds = {
    SndRumble : 0,	
    SndEngInn : 1,		
	SndEngOut : 2,
	SndPropInn : 3,
	SndPropOut : 4,
}

//Declare sound emitter variable in enum, which will be represented as numbers (this is required, because some functions take integer parameters (play_sound, enqueue_loop, set_pitch etc.))
var SndEmit = {
	SrcEmitExt : 0,
	SrcEmitInt : 1,
	SrcEmitRumble : 2,
}

//function used to clamp pitch and gain values.
function Clamp(val, min, max) 
{
	if(val < min) return min; 
	else if(val > max) return max; 
	else return val;
}

//Invoked only when the model is first time loaded, or upon reload
function init_chassis()
{
	//Get joins/bones (takes bone name as parameter)
	//You can use this.get_joint_id() function to get bone directly, instead of getting geomob (this.get_geomob(0)) to use get_joint() function, how it was used in previous vehicle tutorial 
	Propeller = this.get_joint_id('Bone_propel');
	WheelFront = this.get_joint_id('Bone_front_wheel');
	WheelRight = this.get_joint_id('Bone_right_wheel');
	WheelLeft = this.get_joint_id('Bone_left_wheel');
	ElevatorRight = this.get_joint_id('Bone_elevator_right'); 
	ElevatorLeft = this.get_joint_id('Bone_elevator_left');
	Rudder = this.get_joint_id('Bone_rudder');
	AileronRight = this.get_joint_id('Bone_flap_right'); 
	AileronLeft = this.get_joint_id('Bone_flap_left');
	RudderPedalLeft = this.get_joint_id('Bone_rudder_pedal_left');
	RudderPedalRight = this.get_joint_id('Bone_rudder_pedal_right');
	BrakePedalLeft = this.get_joint_id('Bone_brake_pedal_left');
	BrakePedalRight = this.get_joint_id('Bone_brake_pedal_right');
	ThrottleHandle = this.get_joint_id('Bone_throttle_lever');
	
	//Get mesh, for that use function get_mesh_id() (takes mesh name as parameter)
	PropBlur = this.get_mesh_id('propel_blur');
	
	//Load sounds
  	//Engine rumble - in this case it's used for the outside aounds and also the inside
    Snds.SndRumble = this.load_sound("sounds/engine/engn1.ogg");
	
	//Interior sounds - can be heard from inside the plane
    //Engine
	Snds.SndEngOut = this.load_sound("sounds/engine/engn1_out.ogg");
	//Propeller
	Snds.SndPropOut = this.load_sound("sounds/engine/prop1_out.ogg");

	//Exterior sounds - can be heard from outside the plane 
    //Engine
	Snds.SndEngInn = this.load_sound("sounds/engine/engn1_inn.ogg"); 
	//Propeller
	Snds.SndPropInn = this.load_sound("sounds/engine/prop1_inn.ogg");
	
	//Add sound emitters
	//For engine rumbling sound
    SndEmit.SrcEmitRumble = this.add_sound_emitter_id(Propeller, 0, 5.0);
	//For interior sounds
	SndEmit.SrcEmitInt = this.add_sound_emitter_id(Propeller, 0, 1.2);
	//For exterior sounds 
	SndEmit.SrcEmitExt = this.add_sound_emitter_id(Propeller, 0, 3.0);
}

//Function initialize() is used to define per-instance parameters (similarly how vehicles use init_vehicle())
function initialize()
{
	//Get geomob interface, to be able to use functions, such as rotate_joint() and set_mesh_visible_id()
	this.geom = this.get_geomob(0);
	
	//Get JSBSim interface, to be able to work with JSBSim properties
	this.jsbsim = this.jsb();
	
	//Get sound interface, to be able to work with sounds
	this.snd = this.sound();
	
	//Set first person camera position 
	this.set_fps_camera_pos({x:0, y:1, z:1.4});
	
	//Set initial value for JSBSim properties (optional)
	//Turn off engines (0 - turn off, 1 - turn on)
	this.jsbsim['propulsion/starter_cmd'] = 0;
	//Set initial engine rpm to 0
	this.jsbsim['propulsion/engine[0]/engine-rpm'] = 0;
	//Set initial engine power to 0
	this.jsbsim['propulsion/engine[0]/power-hp'] = 0;
	//Set initial wheel speed to 0
	this.jsbsim['gear/unit[0]/wheel-speed-fps'] = 0;
	//Set initial elevator rotation to 0 (in radians)
	this.jsbsim['fcs/elevator-pos-rad'] = 0;
	//Set initial rudder rotation to 0 (in radians)
	this.jsbsim['fcs/rudder-pos-rad'] = 0;
	//Set initial right aileron rotation to 0 (in radians)
	this.jsbsim['fcs/right-aileron-pos-rad'] = 0;
	//Set initial left aileron rotation to 0 (in radians)
	this.jsbsim['fcs/left-aileron-pos-rad'] = 0;
	//Set initial throttle value to 0 (normalized - value is between 0 and 1)
	this.jsbsim['fcs/throttle-cmd-norm[0]'] = 0;
	//Set initial right brake value to 0 (normalized - value is between 0 and 1)
	this.jsbsim['fcs/right-brake-cmd-norm'] = 0;
	//Set initial left brake value to 0 (normalized - value is between 0 and 1)
	this.jsbsim['fcs/left-brake-cmd-norm'] = 0;
}

//Invoked each frame to handle the internal state of the object
function update_frame(dt)
{	
	//Get engine rpm from JSBSim
	var eng_rpm = this.jsbsim['propulsion/engine[0]/engine-rpm'];
	//Get wheel speed from JSBSim
	var wheel_speed = this.jsbsim['gear/unit[0]/wheel-speed-fps'];
	//Get elevator rotation in radians
	var elev_pos_rad = this.jsbsim['fcs/elevator-pos-rad'];
	
	//Rotate joints (1.param - joint you want to rotate, 2.param - rotation angle, 3.param - around which axis, and in what direction should the joint/bone rotate)
	//rotate_joint - does not return to original position
	//rotate_joint_orig - returns to original position
	this.geom.rotate_joint(Propeller, dt * (2 * PI) * (eng_rpm/600), {y:1});
	this.geom.rotate_joint(WheelFront, dt * PI * (wheel_speed / 5), {x:-1});
	this.geom.rotate_joint(WheelRight, dt * PI * (wheel_speed / 5), {x:-1});
	this.geom.rotate_joint(WheelLeft, dt * PI * (wheel_speed / 5), {x:-1});
	this.geom.rotate_joint_orig(ElevatorRight, elev_pos_rad, {x:1});
	this.geom.rotate_joint_orig(ElevatorLeft, elev_pos_rad, {x:1});
	
	//Of course, you can write JSBSim property as parameter without storing them in variable
	this.geom.rotate_joint_orig(Rudder, this.jsbsim['fcs/rudder-pos-rad'], {z:-1});
	this.geom.rotate_joint_orig(AileronRight, this.jsbsim['fcs/right-aileron-pos-rad'], {x:-1});
	this.geom.rotate_joint_orig(AileronLeft, this.jsbsim['fcs/left-aileron-pos-rad'], {x:1});
	this.geom.rotate_joint_orig(BrakePedalLeft, this.jsbsim['fcs/left-brake-cmd-norm'] * 0.7, {x:-1});
	this.geom.rotate_joint_orig(BrakePedalRight, this.jsbsim['fcs/right-brake-cmd-norm'] * 0.7, {x:-1});
	//Rudder has 2 pedals for turning, therefore depending on the rudder position value from JSBSim, we will move left or right pedal
	if(this.jsbsim['fcs/rudder-pos-rad'] > 0)
	{
		this.geom.move_joint_orig(RudderPedalLeft, {y:(this.jsbsim['fcs/rudder-pos-rad']/5)} );
	}
	else if(this.jsbsim['fcs/rudder-pos-rad'] < 0)
	{
		this.geom.move_joint_orig(RudderPedalRight, {y:-(this.jsbsim['fcs/rudder-pos-rad']/5)} );
	}	
	
	//Move joint
	this.geom.move_joint_orig(ThrottleHandle, {y:(this.jsbsim['fcs/throttle-cmd-norm[0]'] / 7)});
	
	//In this case the pitch/roll handle rotates without the need of scripting, because it's binded with inputs in the model
	
	//Use set_mesh_visible_id() function (1.param - mesh, 2.param - condition), to set propeller blur mesh visible for nice motion effect
	this.geom.set_mesh_visible_id(PropBlur, eng_rpm > 2100.0);
	
	if(eng_rpm > 0)
	{
		//Camera mode 0 is the first person view inside the plane
		//Interior
		if (this.get_camera_mode() == 0 ) 
		{	
			//Turn off the exterior sounds, when camera is inside the plane
			this.snd.set_gain (Snds.SndEngOut, 0);
			this.snd.set_gain (Snds.SndPropOut, 0);
			
			if(!this.snd.is_playing(SndEmit.SrcEmitInt)) 
			{
				//Play interior engine sound in a loop
				this.snd.play_loop(SndEmit.SrcEmitInt, Snds.SndEngInn);
				//Play interior propeller sound in a loop 
				this.snd.play_loop(SndEmit.SrcEmitInt, Snds.SndPropInn);
				//Play rumble sound in a loop
				this.snd.play_loop(SndEmit.SrcEmitRumble, Snds.Rumble);
			}
			
			//Pitch and gain are set to custom values, feel free to modify them to your liking
			//Set pitch for interior emitter and clamp it between 0 and 5
			this.snd.set_pitch(SndEmit.SrcEmitInt, Clamp(eng_rpm/1400.0, 0.0, 5.0));
			//Set gain for interior engine sound and clamp it between 0 and 1
			this.snd.set_gain(Snds.SndEngInn, Clamp(eng_rpm/1500.0, 0.0, 1.0));
			
			//Set pitch for rumble emitter and clamp it between 0 and 10
			this.snd.set_pitch(SndEmit.SrcEmitRumble, Clamp((eng_rpm)/ 1500.0, 0.0, 10.0));
			//Set gain for rumbling sound and clamp it between 0 and 1
			this.snd.set_gain (Snds.Rumble, Clamp(eng_rpm / 2000.0, 0.0, 1.0));
        }
		//Exterior
		else 
		{
			//Turn off the interior sounds, when camera is outside the plane
			this.snd.set_gain (Snds.SndEngInn, 0);
			this.snd.set_gain (Snds.SndPropInn, 0);
			
			if(!this.snd.is_playing(SndEmit.SrcEmitExt)) 
			{
				//Play exterior engine sound in a loop
				this.snd.play_loop(SndEmit.SrcEmitExt, Snds.SndEngOut);
				//Play exterior propeller sound in a loop
				this.snd.play_loop(SndEmit.SrcEmitExt, Snds.SndPropOut);
				//Play rumble sound in a loop
				this.snd.play_loop(SndEmit.SrcEmitRumble, Snds.Rumble);
			}
			
			//Set pitch for exterior emitter and clamp it between 0 and 5
			this.snd.set_pitch(SndEmit.SrcEmitExt, Clamp(eng_rpm/1300.0, 0.0, 5.0));
			//Set gain for exterior engine sound and clamp it between 0 and 1
			this.snd.set_gain (Snds.SndEngOut, Clamp(eng_rpm/1500.0, 0.0, 1.0));

			//Set pitch for rumble emitter and clamp it between 0 and 10
			this.snd.set_pitch(SndEmit.SrcEmitRumble, Clamp((eng_rpm)/ 1300.0, 0.0, 10.0));
			//Set gain for rumbling sound and clamp it between 0 and 1
			this.snd.set_gain (Snds.Rumble, Clamp(eng_rpm / 1500.0, 0.0, 1.0));
		}
	}
	else 
	{
		//Stop all sounds, when engine is turned off and propeller stops rotating
		this.snd.stop(SndEmit.SrcExhaustExt);
		this.snd.stop(SndEmit.SrcExhaustInt);
		this.snd.stop(SndEmit.SrcEmitRumble);
		this.snd.set_gain (Snds.SndEngOut, 0.0);
		this.snd.set_gain (Snds.SndPropOut, 0.0);
		this.snd.set_gain (Snds.Rumble, 0.0);
		this.snd.set_gain (Snds.SndEngInn, 0.0);
		this.snd.set_gain (Snds.SndPropInn, 0.0);
    }	
}

