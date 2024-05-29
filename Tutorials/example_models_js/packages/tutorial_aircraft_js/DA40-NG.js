//Welcome to the second tutorial on JavaScript programming in Outerra.
//We recommend reviewing previous tutorial (tutorial_car) before proceeding, as well as the corresponding tutorial on the Outerra/anteworld GitHub page ( https://github.com/Outerra/anteworld/wiki/Tutorial-2-%E2%80%90-Aircraft-(javascript) ), as it contains detailed information.

//Define global variables
//Used for joint rotation
const PI = 3.14159265358979323846;

//Create objects and initialize it's properties with default values (-1 for debug purpose)
//Bones
let bones = {
    propeller : -1,
    wheel_front : -1,
    wheel_right : -1,
    wheel_left : -1,
    elevator_right : -1,
    elevator_left : -1,
    rudder : -1,
    aileron_right : -1,
    aileron_left : -1,
    rudder_pedal_left : -1,
    rudder_pedal_right : -1,
    brake_pedal_left : -1,
    brake_pedal_right : -1,
    throttle_handle : -1,
    flap_left : -1,
    flap_right : -1,  
};

//Meshes
let meshes = {
    prop_blur : -1,
	blade_one : -1,
	blade_two : -1,
	blade_three : -1,
};

//Sounds 
let sounds = {
    rumble : -1,
    eng_int : -1,
    eng_ext : -1,
    prop_int : -1,
    prop_ext : -1,
};

//Sound sources
let sources = {
    rumble_int : -1,
    rumble_ext : -1,
    eng_int : -1,
    eng_ext : -1,
    prop_int : -1,
    prop_ext : -1,
};

//Used to clamp values.
function clamp(val, min, max) 
{
	if(val < min) return min; 
	else if(val > max) return max; 
	else return val;
}

//Gets called when engine starts/stops
function engine()
{
    //toggle the value every time this function is called 
    this.started ^= 1;
    
    if(this.started === 1)
    {
        //For JSBSim aircraft to start, the starter and magneto needs to be activated
        //Turn on engine (0 - turn off, 1 - turn on)
        this.jsbsim['propulsion/starter_cmd'] = 1;
        //debug tip: when the magneto isn't activated, it results in decreased piston engine power, with the rpm not exceeding 1000
        this.jsbsim['propulsion/magneto_cmd'] = 1;
    }
    else
    {
        this.jsbsim['propulsion/starter_cmd'] = 0;
        this.jsbsim['propulsion/magneto_cmd'] = 0;
    }
}
//"this.jsbsim['property']" is used, to set/get property belonging to JSBSim interface. 
//For information on JSBSim, visit the JSBSim & Aeromatic page (https://github.com/Outerra/anteworld/wiki/JSBSim-&-Aeromatic).
//For a list of usable JSBSim properties, refer to the JSBSim properties page (https://github.com/Outerra/anteworld/wiki/JSBSim-properties).

//Invoked once, to define the chassis for all instances
function init_chassis()
{
	//Get joins/bones
	bones.propeller = this.get_joint_id('propel');
	bones.wheel_front = this.get_joint_id('front_wheel');
	bones.wheel_right = this.get_joint_id('right_wheel');
	bones.wheel_left = this.get_joint_id('left_wheel');
	bones.elevator_right = this.get_joint_id('elevator_right'); 
	bones.elevator_left = this.get_joint_id('elevator_left');
	bones.rudder = this.get_joint_id('rudder');
	bones.aileron_right = this.get_joint_id('aileron_right'); 
	bones.aileron_left = this.get_joint_id('aileron_left');
    bones.flap_left = this.get_joint_id('flap_left');
    bones.flap_right = this.get_joint_id('flap_right');
	bones.rudder_pedal_left = this.get_joint_id('rudder_pedal_left');
	bones.rudder_pedal_right = this.get_joint_id('rudder_pedal_right');
	bones.brake_pedal_left = this.get_joint_id('brake_pedal_left');
	bones.brake_pedal_right = this.get_joint_id('brake_pedal_right');
	bones.throttle_handle = this.get_joint_id('throttle_lever');
	
	//Get mesh, for that use function get_mesh_id
	// 1.parameter - mesh name
	meshes.prop_blur = this.get_mesh_id('propel_blur');
	meshes.blade_one = this.get_mesh_id('main_blade_01#0@0');
	meshes.blade_two = this.get_mesh_id('main_blade_02#0@0');
	meshes.blade_three = this.get_mesh_id('main_blade_03#0@0');

	//Add lights
	//Landing lights
	let light_params = {color:{x:1,y:1,z:1}, angle:100, size:0.1, edge:0.25, intensity:5, fadeout:0.05};
	this.add_spot_light({x:4.5, y:1.08, z:0.98}, {x:-0.1, y:1, z: 0.3}, light_params);
	this.add_spot_light({x:-4.5, y:1.08, z:0.98}, {x:0.1, y:1, z: 0.3}, light_params);

	//Navigation lights
	light_params = {color:{x:0,y:1,z:0}, size:0.035, edge:1, range:0.0001, intensity:20, fadeout:0.1};
	let nav_light_offset =
	//Right green navigation light
	this.add_point_light({x:5.08, y:0.18, z:1.33}, light_params);
	//Left red navigation light
	light_params.color = {x:1,y:0,z:0};
	this.add_point_light({x:-5.08, y:0.18, z:1.33}, light_params);
	

	//Load sounds
  	//engine rumble - in this case it's used for the outside sounds and also the inside
    sounds.rumble = this.load_sound("sounds/engine/engn1.ogg");
	
	//Interior sounds - will be heard from inside the plane
    //engine
	sounds.eng_ext = this.load_sound("sounds/engine/engn1_out.ogg");
    //propeller
	sounds.prop_ext = this.load_sound("sounds/engine/prop1_out.ogg");

	//Exterior sounds - will be heard from outside the plane 
    //engine
	sounds.eng_int = this.load_sound("sounds/engine/engn1_inn.ogg"); 
    //propeller
	sounds.prop_int = this.load_sound("sounds/engine/prop1_out.ogg");
	
	//Add sound emitters
    //Interior emitters
	//For interior rumbling sound
    sources.rumble_int = this.add_sound_emitter_id(bones.propeller, -1, 0.5);
    //For interior engine sounds
	sources.eng_int = this.add_sound_emitter_id(bones.propeller, -1, 0.5);
	//For interior propeller sounds
	sources.prop_int = this.add_sound_emitter_id(bones.propeller, -1, 0.5);
    
    //Exterior emitters
    //For exterior rumbling sound
    sources.rumble_ext = this.add_sound_emitter_id(bones.propeller, 1, 3);
	//For exterior engine sounds 
	sources.eng_ext = this.add_sound_emitter_id(bones.propeller, 1, 3);
	//For exterior propeller sounds 
	sources.prop_ext = this.add_sound_emitter_id(bones.propeller, 1, 3);
    
    //Note: it is possible, to use one emitter for interior and exterior, like in previous tutorial
    
    //Add action handlers
	this.register_axis("air/lights/landing_lights", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) { this.light_mask( 0x3,v > 0); });
    this.register_axis("air/lights/nav_lights", {minval: 0, maxval: 1, vel: 10, center: 0 }, function(v) { this.light_mask( 0x3,v > 0, nav_light_offset); });
   	
    /*Note: by default, JSBSim actions are handled internally, allowing gameplay without direct user intervention. However, sometimes it is needed to 
    customize or fine-tune aircraft behavior, therefore in this case, the following handlers are handled in script.*/
    this.register_event("air/engines/on", engine);
    this.register_axis("air/controls/aileron", { minval: -1, maxval: 1, center: 0.5, vel: 0.5, positions: 0 }, function(v){
       	this.jsbsim['fcs/aileron-cmd-norm'] = v;
    });
	this.register_axis("air/controls/elevator", { minval: -1, maxval: 1, center: 0.5, vel: 0.5, positions: 0 }, function(v){
        this.jsbsim['fcs/elevator-cmd-norm'] = -v;
    });
    this.register_axis("air/controls/brake", { minval: 0}, function(v) { 
        this.braking = v;
    	this.jsbsim['fcs/center-brake-cmd-norm'] = v;
        this.jsbsim['fcs/left-brake-cmd-norm'] = v;
        this.jsbsim['fcs/right-brake-cmd-norm'] = v;
    });
  
	return {
		mass: 1310,
		com: {x: 0.0, y: 0.0, z: 0.2},
	};
}

//Function initialize() is used to define per-instance parameters (similarly how vehicles use init_vehicle())
function initialize()
{
	//Get geomob interface
	this.geom = this.get_geomob(0);
	
	//Get JSBSim interface, to be able to work with JSBSim properties
	this.jsbsim = this.jsb();
	
	//Get sound interface
	this.snd = this.sound();
	
	//Set first person camera position 
	this.set_fps_camera_pos({x:0, y:1, z:1.4});
    
    this.started = 0;
    this.braking = 0;
	
	//Set initial value for JSBSim properties
	//Turn off engine (commands are normalized)
	this.jsbsim['propulsion/starter_cmd'] = 0;
    //Turn off magneto
	this.jsbsim['propulsion/magneto_cmd'] = 0;
	//Set initial throttle value to 0 
	this.jsbsim['fcs/throttle-cmd-norm[0]'] = 0;
    //Set initial throttle value to 0 
    this.jsbsim['fcs/mixture-cmd-norm'] = 0;
	//Set initial right brake value to 0 
	this.jsbsim['fcs/right-brake-cmd-norm'] = 0;
	//Set initial left brake value to 0 
	this.jsbsim['fcs/left-brake-cmd-norm'] = 0;
    
    //Set default pitch value for sound emitters
    this.snd.set_pitch(sources.rumble_ext, 1);
    this.snd.set_pitch(sources.rumble_int, 1);
    this.snd.set_pitch(sources.eng_ext, 1);
    this.snd.set_pitch(sources.eng_int, 1);
    this.snd.set_pitch(sources.prop_ext, 1);
    this.snd.set_pitch(sources.prop_int, 1);
    
    //Set default gain value for sound emitters
    this.snd.set_gain(sources.rumble_ext, 1);
    this.snd.set_gain(sources.rumble_int, 1);
    this.snd.set_gain(sources.eng_ext, 1);
    this.snd.set_gain(sources.eng_int, 1);
    this.snd.set_gain(sources.prop_ext, 1);
    this.snd.set_gain(sources.prop_int, 1);
}

//Invoked each frame to handle the internal state of the object
function update_frame(dt)
{	
	//Get engine rpm from JSBSim
	let propeller_rpm = this.jsbsim['propulsion/engine[0]/propeller-rpm'];
	//Get wheel speed from JSBSim
	let wheel_speed = this.jsbsim['gear/unit[0]/wheel-speed-fps'];
	//Get elevator rotation in radians
	let elev_pos_rad = this.jsbsim['fcs/elevator-pos-rad'];
	
	//rotate_joint - incremental, bone rotates by given angle every time, this function is called (given angle is added to joint current orientation)
	this.geom.rotate_joint(bones.propeller, dt * (2 * PI) * (propeller_rpm), {y:1});
	this.geom.rotate_joint(bones.wheel_front, dt * PI * (wheel_speed / 5), {x:-1});
	this.geom.rotate_joint(bones.wheel_right, dt * PI * (wheel_speed / 5), {x:-1});
	this.geom.rotate_joint(bones.wheel_left, dt * PI * (wheel_speed / 5), {x:-1});
    //rotate_joint_orig - rotate to given angle (from default orientation)
    this.geom.rotate_joint_orig(bones.elevator_right, elev_pos_rad, {x:1});
	this.geom.rotate_joint_orig(bones.elevator_left, elev_pos_rad, {x:1});
	this.geom.rotate_joint_orig(bones.rudder, this.jsbsim['fcs/rudder-pos-rad'], {z:-1});
	this.geom.rotate_joint_orig(bones.aileron_right, this.jsbsim['fcs/right-aileron-pos-rad'], {x:-1});
	this.geom.rotate_joint_orig(bones.aileron_left, this.jsbsim['fcs/left-aileron-pos-rad'], {x:1});
	this.geom.rotate_joint_orig(bones.flap_left, this.jsbsim['fcs/flap-pos-rad'], {x:1,y:0,z:0});
    this.geom.rotate_joint_orig(bones.flap_right, this.jsbsim['fcs/flap-pos-rad'], {x:1,y:0,z:0});
    this.geom.rotate_joint_orig(bones.brake_pedal_left, this.jsbsim['fcs/left-brake-cmd-norm'] * 0.7, {x:-1});
	this.geom.rotate_joint_orig(bones.brake_pedal_right, this.jsbsim['fcs/right-brake-cmd-norm'] * 0.7, {x:-1});

	//Rudder has 2 pedals for turning, therefore depending on the rudder position value from JSBSim, the left or right pedal will move
	if(this.jsbsim['fcs/rudder-pos-rad'] > 0)
	{
		this.geom.move_joint_orig(bones.rudder_pedal_left, {y:(this.jsbsim['fcs/rudder-pos-rad']/5)} );
	}
	else if(this.jsbsim['fcs/rudder-pos-rad'] < 0)
	{
		this.geom.move_joint_orig(bones.rudder_pedal_right, {y:-(this.jsbsim['fcs/rudder-pos-rad']/5)} );
	}	
	
	//Move throttle handle, depending on the throttle command value 
	this.geom.move_joint_orig(bones.throttle_handle, {y:(this.jsbsim['fcs/throttle-cmd-norm[0]'] / 7)});
	
	//Note: in this case the pitch/roll handle rotates without the need of scripting, because it's binded with inputs in the model
	
	//Use set_mesh_visible_id() function, to set propeller blur mesh visible, when condition is true and invisible otherwise
	//1.param - mesh ID
	//2.param - condition
	this.geom.set_mesh_visible_id(meshes.prop_blur, propeller_rpm > 200.0);
	this.geom.set_mesh_visible_id(meshes.blade_one, propeller_rpm < 300.0);
	this.geom.set_mesh_visible_id(meshes.blade_two, propeller_rpm < 300.0);
	this.geom.set_mesh_visible_id(meshes.blade_three, propeller_rpm < 300.0);
	
    //When engine runs, control sounds, based on the camera position & sound effects, based on engine RPM
	if(propeller_rpm > 0)
	{
		//Interior
		if (this.get_camera_mode() === 0 ) 
		{	
			//Turn off the exterior emitters, when player camera is inside the plane
			this.snd.stop(sources.eng_ext); 
            this.snd.stop(sources.prop_ext);
            this.snd.stop(sources.rumble_ext);
            
			//Calculate and set pitch for interior engine emitter (clamped between 1 and 2)
			this.snd.set_pitch(sources.eng_int, clamp(1 + propeller_rpm/4000, 1, 2));
			//Calculate and set gain for interior engine emitter (clamped between 0 and 0.5)
			this.snd.set_gain(sources.eng_int, clamp(propeller_rpm/5000, 0, 0.5));
			
            //If there is no sound already playing on given emitter, play loop
			if(!this.snd.is_playing(sources.eng_int)) 
			{
				//Play interior engine sound in a loop
				this.snd.play_loop(sources.eng_int, sounds.eng_int);
			}
            
			//Set gain for interior propeller emitter
			this.snd.set_gain(sources.prop_int, clamp(propeller_rpm/7000, 0, 0.5));
			
			if(!this.snd.is_playing(sources.prop_int)) 
			{
				//Play interior propeller sound in a loop
				this.snd.play_loop(sources.prop_int, sounds.prop_int);
			}
			
			//Set gain for interior rumble emitter
			this.snd.set_gain (sources.rumble_int, clamp(propeller_rpm/6000, 0, 0.5));
			
            if(!this.snd.is_playing(sources.rumble_int)) 
			{
				//Play rumble sound in a loop on interior emitter
				this.snd.play_loop(sources.rumble_int, sounds.rumble);
			}
        }
		//Exterior
		else 
		{
			//Turn off the interior emitters, when camera is outside the plane
			this.snd.stop (sources.eng_int);
			this.snd.stop (sources.prop_int);
			this.snd.stop (sources.rumble_int);
            
			//Set pitch for exterior engine emitter
			this.snd.set_pitch(sources.eng_ext, clamp(1 + propeller_rpm/1000, 1, 3));
			//Set gain for exterior engine emitter
			this.snd.set_gain (sources.eng_ext, clamp(propeller_rpm/450, 0.05, 2));
			
            if(!this.snd.is_playing(sources.eng_ext)) 
			{
				//Play exterior engine sound in a loop
				this.snd.play_loop(sources.eng_ext, sounds.eng_ext);
			}
            
            //Set gain for exterior propeller emitter
			this.snd.set_gain (sources.prop_ext, clamp(propeller_rpm/900, 0, 2));
            
            if(!this.snd.is_playing(sources.prop_ext)) 
			{
				//Play exterior propeller sound in a loop
				this.snd.play_loop(sources.prop_ext, sounds.prop_ext);
			}
			
			//Set gain for exterior rumble emitter
			this.snd.set_gain (sources.rumble_ext, clamp(propeller_rpm/1200, 0, 2));
			
			if(!this.snd.is_playing(sources.rumble_ext)) 
			{
				//Play rumble sound in a loop on exterior emitter
				this.snd.play_loop(sources.rumble_ext, sounds.rumble);
			}
		}
	}
	else 
	{
		//Stop all sounds playing on given emitters, when engine is turned off and propeller stops rotating
		this.snd.stop(sources.eng_ext);
		this.snd.stop(sources.eng_int);
        this.snd.stop(sources.prop_ext);
		this.snd.stop(sources.prop_int);
		this.snd.stop(sources.rumble_ext);
		this.snd.stop(sources.rumble_int);
    }	
    
    //Parking brake when in idle state 
    if(!this.started && propeller_rpm < 5)
    {
        this.jsbsim['fcs/center-brake-cmd-norm'] = 1;
        this.jsbsim['fcs/left-brake-cmd-norm'] = 1;
        this.jsbsim['fcs/right-brake-cmd-norm'] = 1;
    }
    else if (this.braking < 0.1)
    {
        this.jsbsim['fcs/center-brake-cmd-norm'] = 0;
        this.jsbsim['fcs/left-brake-cmd-norm'] = 0;
        this.jsbsim['fcs/right-brake-cmd-norm'] = 0;
    } 
}

