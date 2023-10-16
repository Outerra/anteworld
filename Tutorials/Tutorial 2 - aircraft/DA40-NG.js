//Declare global variables
let Propeller, WheelFront, WheelRight, WheelLeft, ElevatorRight, ElevatorLeft, Rudder, AileronRight, AileronLeft, PropBlur, BladeOneMesh, BladeTwoMesh, BladeThreeMesh;

//Declare sound variables
let SndRumble, SndEngInn, SndEngOut, SndPropInn, SndPropOut;

//Declare sound emitter variables 
let SrcEmitExt, SrcEmitInt, SrcEmitRumble;

//Used for joint rotation
const PI = 3.14159265358979323846;

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
	//Get joins/bones
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
	 
	
	//Get mesh, for that use function get_mesh_id() 
	// 1.parameter - mesh name
	PropBlur = this.get_mesh_id('propel_blur');
	BladeOneMesh = this.get_mesh_id('main_blade_01#0@0');
	BladeTwoMesh = this.get_mesh_id('main_blade_01.001#0@0');
	BladeThreeMesh = this.get_mesh_id('main_blade_01.002#0@0');
	
	//Add action handlers
	this.register_axis("air/lights/landing_lights", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) { this.light_mask( 0x3,v > 0); });
   	this.register_axis("air/lights/nav_lights", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) { this.light_mask( 0x3,v > 0, navLightOffset); });

	//Add lights
	//Landing lights
	let lightParams = {color:{x:1,y:1,z:1}, angle:100, size:0.1, edge:0.25, intensity:5, fadeout:0.05};
	this.add_spot_light({x:4.5, y:1.08, z:0.98}, {x:-0.1, y:1, z: 0.3}, lightParams);
	this.add_spot_light({x:-4.5, y:1.08, z:0.98}, {x:0.1, y:1, z: 0.3}, lightParams);

	//Navigation lights
	lightParams = {color:{x:0,y:1,z:0}, size:0.035, edge:1, range:0.0001, intensity:20, fadeout:0.1};
	let navLightOffset =
	//Right green navigation light
	this.add_point_light({x:5.08, y:0.18, z:1.33}, lightParams);
	//Left red navigation light
	lightParams.color = {x:1,y:0,z:0};
	this.add_point_light({x:-5.08, y:0.18, z:1.33}, lightParams);
	

	//Load sounds
  	//Engine rumble - in this case it's used for the outside sounds and also the inside
   	SndRumble = this.load_sound("sounds/engine/engn1.ogg");
	
	//Interior sounds - can be heard from inside the plane
   	//Engine
	SndEngOut = this.load_sound("sounds/engine/engn1_out.ogg");

	//Exterior sounds - can be heard from outside the plane 
   	//Engine
	SndEngInn = this.load_sound("sounds/engine/engn1_inn.ogg"); 
	
	//Add sound emitters
	//For engine rumbling sound
   	 SrcEmitRumble = this.add_sound_emitter_id(Propeller, 0, 5.0);
	//For interior sounds
	SrcEmitInt = this.add_sound_emitter_id(Propeller, -1, 1.2);
	//For exterior sounds 
	SrcEmitExt = this.add_sound_emitter_id(Propeller, 1, 2.0);

	return {
		mass: 1310,
		com: {x: 0.0, y: 0.0, z: 0.2},
	};


}

//Function initialize() is used to define per-instance parameters (similarly how vehicles use init_vehicle())
function initialize()
{
	//Get geomob interface
	this.Geom = this.get_geomob(0);
	
	//Get JSBSim interface, to be able to work with JSBSim properties
	this.JSBSim = this.jsb();
	
	//Get sound interface
	this.Snd = this.sound();
	
	//Set first person camera position 
	this.set_fps_camera_pos({x:0, y:1, z:1.4});
	
	//Set initial value for JSBSim properties (optional)
	//Turn off engines (0 - turn off, 1 - turn on)
	this.JSBSim['propulsion/starter_cmd'] = 0;
	//Set initial engine rpm to 0
	this.JSBSim['propulsion/engine[0]/engine-rpm'] = 0;
	//Set initial engine power to 0
	this.JSBSim['propulsion/engine[0]/power-hp'] = 0;
	//Set initial wheel speed to 0
	this.JSBSim['gear/unit[0]/wheel-speed-fps'] = 0;
	//Set initial elevator rotation to 0 (in radians)
	this.JSBSim['fcs/elevator-pos-rad'] = 0;
	//Set initial rudder rotation to 0 (in radians)
	this.JSBSim['fcs/rudder-pos-rad'] = 0;
	//Set initial right aileron rotation to 0 (in radians)
	this.JSBSim['fcs/right-aileron-pos-rad'] = 0;
	//Set initial left aileron rotation to 0 (in radians)
	this.JSBSim['fcs/left-aileron-pos-rad'] = 0;
	//Set initial throttle value to 0 (normalized - value is between 0 and 1)
	this.JSBSim['fcs/throttle-cmd-norm[0]'] = 0;
	//Set initial right brake value to 0 (normalized - value is between 0 and 1)
	this.JSBSim['fcs/right-brake-cmd-norm'] = 0;
	//Set initial left brake value to 0 (normalized - value is between 0 and 1)
	this.JSBSim['fcs/left-brake-cmd-norm'] = 0;
}

//Invoked each frame to handle the internal state of the object
function update_frame(dt)
{	
	//Get engine rpm from JSBSim
	let eng_rpm = this.JSBSim['propulsion/engine[0]/engine-rpm'];
	//Get wheel speed from JSBSim
	let wheel_speed = this.JSBSim['gear/unit[0]/wheel-speed-fps'];
	//Get elevator rotation in radians
	let elev_pos_rad = this.JSBSim['fcs/elevator-pos-rad'];
	
	//rotate_joint - is incremental, bone rotates by given angle every time, this function is called (given angle is added to joint current orientation)
	//rotate_joint_orig - rotate to given angle (from default orientation)
	this.Geom.rotate_joint(Propeller, dt * (2 * PI) * (eng_rpm), {y:1});
	this.Geom.rotate_joint(WheelFront, dt * PI * (wheel_speed / 5), {x:-1});
	this.Geom.rotate_joint(WheelRight, dt * PI * (wheel_speed / 5), {x:-1});
	this.Geom.rotate_joint(WheelLeft, dt * PI * (wheel_speed / 5), {x:-1});
	this.Geom.rotate_joint_orig(ElevatorRight, elev_pos_rad, {x:1});
	this.Geom.rotate_joint_orig(ElevatorLeft, elev_pos_rad, {x:1});
	
	//Of course, you can write JSBSim property as parameter without storing them in variable
	this.Geom.rotate_joint_orig(Rudder, this.JSBSim['fcs/rudder-pos-rad'], {z:-1});
	this.Geom.rotate_joint_orig(AileronRight, this.JSBSim['fcs/right-aileron-pos-rad'], {x:-1});
	this.Geom.rotate_joint_orig(AileronLeft, this.JSBSim['fcs/left-aileron-pos-rad'], {x:1});
	this.Geom.rotate_joint_orig(BrakePedalLeft, this.JSBSim['fcs/left-brake-cmd-norm'] * 0.7, {x:-1});
	this.Geom.rotate_joint_orig(BrakePedalRight, this.JSBSim['fcs/right-brake-cmd-norm'] * 0.7, {x:-1});
	//Rudder has 2 pedals for turning, therefore depending on the rudder position value from JSBSim, we will move left or right pedal
	if(this.JSBSim['fcs/rudder-pos-rad'] > 0)
	{
		this.Geom.move_joint_orig(RudderPedalLeft, {y:(this.JSBSim['fcs/rudder-pos-rad']/5)} );
	}
	else if(this.JSBSim['fcs/rudder-pos-rad'] < 0)
	{
		this.Geom.move_joint_orig(RudderPedalRight, {y:-(this.JSBSim['fcs/rudder-pos-rad']/5)} );
	}	
	
	//Move joint
	this.Geom.move_joint_orig(ThrottleHandle, {y:(this.JSBSim['fcs/throttle-cmd-norm[0]'] / 7)});
	
	//Note: in this case the pitch/roll handle rotates without the need of scripting, because it's binded with inputs in the model
	
	//Use set_mesh_visible_id() function, to set propeller blur mesh visible, when condition is true 
	//1.param - mesh ID
	//2.param - condition
	this.Geom.set_mesh_visible_id(PropBlur, eng_rpm > 1400.0);
	
	//You can also use this function to hide meshes, for that, the condition has to be false (we want the propeller blades to be hidden, when eng_rpm >= 1600, so that only the propeller blur is visible)
	this.Geom.set_mesh_visible_id(BladeOneMesh, eng_rpm < 1600.0);
	this.Geom.set_mesh_visible_id(BladeTwoMesh, eng_rpm < 1600.0);
	this.Geom.set_mesh_visible_id(BladeThreeMesh, eng_rpm < 1600.0);
	
	if(eng_rpm > 0)
	{
		//Camera mode 0 is the first person view inside the plane
		//Interior
		if (this.get_camera_mode() === 0 ) 
		{	
			//Turn off the exterior emitter, when camera is inside the plane
			this.Snd.stop(SrcEmitExt);
			
			//If there is no sound already playing on given emitter, play sound
			if(!this.Snd.is_playing(SrcEmitInt)) 
			{
				//Play interior engine sound in a loop
				this.Snd.play_loop(SrcEmitInt, SndEngInn);
			}
			
			//Pitch and gain are set to custom values, feel free to modify them to your liking
			//Set pitch for interior emitter and clamp it between 0 and 0.6
			this.Snd.set_pitch(SrcEmitInt, Clamp(eng_rpm/4000.0, 0, 0.6));
			//Set gain for interior emitter and clamp it between 0 and 0.6
			this.Snd.set_gain(SrcEmitInt, Clamp(eng_rpm/4000.0, 0, 0.6));
						
			if(!this.Snd.is_playing(SrcEmitRumble)) 
			{
				//Play rumble sound in a loop
				this.Snd.play_loop(SrcEmitRumble, SndRumble);
			}
			
			
			//Set pitch for rumble emitter and clamp it between 0 and 2
			this.Snd.set_pitch(SrcEmitRumble, Clamp(eng_rpm/1200.0, 0, 2.0));
			//Set gain for rumble emitter and clamp it between 0 and 1.5
			this.Snd.set_gain (SrcEmitRumble, Clamp(eng_rpm/2000.0, 0.0, 1.5));
        	}
		//Exterior
		else 
		{
			//Turn off the interior emitter, when camera is outside the plane
			this.Snd.stop (SrcEmitInt);
			
			if(!this.Snd.is_playing(SrcEmitExt)) 
			{
				//Play exterior engine sound in a loop
				this.Snd.play_loop(SrcEmitExt, SndEngOut);
			}
			
			//Set pitch for interior emitter and clamp it between 0 and 0.6
			this.Snd.set_pitch(SrcEmitExt, Clamp(eng_rpm/4000.0, 0, 0.6));
			//Set gain for interior emitter and clamp it between 0 and 0.6
			this.Snd.set_gain (SrcEmitExt, Clamp(eng_rpm/4000.0, 0, 0.6));
			
			if(!this.Snd.is_playing(SrcEmitRumble)) 
			{
				//Play rumble sound in a loop
				this.Snd.play_loop(SrcEmitRumble, SndRumble);
			}

			//Set pitch for rumble emitter and clamp it between 0 and 2
			this.Snd.set_pitch(SrcEmitRumble, Clamp(eng_rpm/1200.0, 0, 2.0));
			//Set gain for rumble emitter and clamp it between 0 and 2
			this.Snd.set_gain (SrcEmitRumble, Clamp(eng_rpm/1300.0, 0.0, 2.0));
		}
	}
	else 
	{
		//Stop all sounds, when engine is turned off and propeller stops rotating
		this.Snd.stop(SrcEmitExt);
		this.Snd.stop(SrcEmitInt);
		this.Snd.stop(SrcEmitRumble);
	}	
}

