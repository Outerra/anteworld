//*****Version 4 - Sounds*****

const SPEED_GAUGE_MIN = 10.0;
const RAD_PER_KMH = 0.018325957;
const ENGINE_FORCE = 25000.0;
const BRAKE_FORCE = 5000.0;
const MAX_KMH = 200;
const FORCE_LOSS = ENGINE_FORCE / (0.2*MAX_KMH + 1);

//radio
let station = 0;
let radio_plugin;



let wheels = {
    FLwheel : -1, 
    FRwheel : -1, 
    RLwheel : -1, 
    RRwheel : -1, 
};

let bones = {
    steer_wheel : -1, 
    speed_gauge : -1, 
    accel_pedal : -1, 
    brake_pedal : -1, 
    driver_door : -1,
};

let light_entity = {
    brake_mask : 0, 
    rev_mask : 0, 
    turn_left_mask : 0, 
    turn_right_mask : 0,
};

//create object containing sound related members
let sound_entity = {
    snd_starter : -1, 
    snd_eng_running : -1, 
    snd_eng_stop : -1, 
    src_eng_start_stop : -1, 
    src_eng_running : -1,
};

function reverse_action(v)
{
    
    //radio
    let radiostate = radio_plugin.toggle_radio();
    
    $log("radio " + radiostate);
     
    
    
    
    
    
    
	this.eng_dir = this.eng_dir >= 0 ? -1 : 1;
	this.fade(this.eng_dir > 0 ? "Forward" : "Reverse");
	this.light_mask(light_entity.rev_mask, this.eng_dir < 0);
}

function engine_action()
{
    
	this.started = this.started === 0 ? 1 : 0;
    
    
    
    //radio
    radio_plugin.set_radio_state(this.started);
    
    
    
    
    
    
	this.fade(this.started === 1  ? "Engine start" : "Engine stop");
    
    //Get camera mode using function get_camera_mode and based on that, set the sound gain value 
	//returns - camera mode (0, when the first person view, inside the vehicle is active)
	let sound_gain = this.get_camera_mode() == 0 ? 0.25 : 1;

	//Use set_gain function to set gain value on given emitter
	// 1.parameter - emitter
	// 2.parameter - gain value(this will affect all sounds emitted from this emitter)
	this.snd.set_gain(sound_entity.src_eng_start_stop, sound_gain);

	//Based on the camera mode, choose reference distance value
	let ref_distance = this.get_camera_mode() == 0 ? 0.25 : 1;

	//Use set_ref_distance to set reference distance on given emitter (how far should the sounds be heard)
	// 1.parameter - emitter
	// 2.parameter - reference distance value(this will affect all sounds emitted from this emitter)
	this.snd.set_ref_distance(sound_entity.src_eng_start_stop, ref_distance);

    
	if(this.started) 
	{
		//play_sound function is used to play sound once, discarding older sounds
		//1.parameter - emitter (source ID) 
		//2.parameter - sound (sound ID))
		this.snd.play_sound(sound_entity.src_eng_start_stop, sound_entity.snd_starter);
	}
	else 
	{
		//function "stop" discards all sounds playing on given emitter
		this.snd.stop(sound_entity.src_eng_running);
		this.snd.play_sound(sound_entity.src_eng_start_stop, sound_entity.snd_eng_stop);
	}
}
 
function init_chassis()
{ 
	let wheel_params = {
		radius: 0.31515,
		width: 0.2,
		suspension_max: 0.1,
		suspension_min: -0.03,
		suspension_stiffness: 50.0,
		damping_compression: 0.4,
		damping_relaxation: 0.12,
		grip: 1,
	};
	
	wheels.FLwheel = this.add_wheel('wheel_l0', wheel_params); 
	wheels.FRwheel = this.add_wheel('wheel_r0', wheel_params); 
	wheels.RLwheel = this.add_wheel('wheel_l1', wheel_params); 
	wheels.RRwheel = this.add_wheel('wheel_r1', wheel_params); 

	bones.steer_wheel = this.get_joint_id('steering_wheel');		
	bones.speed_gauge = this.get_joint_id('dial_speed');							
	bones.accel_pedal = this.get_joint_id('pedal_accelerator');
	bones.brake_pedal = this.get_joint_id('pedal_brake');				
	bones.driver_door = this.get_joint_id('door_l0');	
	
	let light_props = {size:0.05, angle:120, edge:0.2, fadeout:0.05, range:70 };
	this.add_spot_light({x:-0.5,y:2.2,z:0.68}, {y:1}, light_props); 	//left front light
	this.add_spot_light({x:0.5,y:2.2,z: 0.68}, {y:1}, light_props);	//right front light

	light_props = { size: 0.07, angle: 160, edge: 0.8, fadeout: 0.05, range: 150, color: { x: 1.0 } };
	this.add_spot_light({x:0.05,y:0,z:0.02}, {y:1}, light_props, "tail_light_l0"); 	//left tail light
	this.add_spot_light({x:0.05,y:0,z:0.02}, {y:1}, light_props, "tail_light_r0"); 	//right tail light  

	light_props = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100, color: { x: 1.0 } };
	let brake_light_offset =  
	this.add_spot_light({x:-0.38,y:-2.11,z: 0.67}, {y:-1}, light_props); 	//left brake light
	this.add_spot_light({x:0.38,y:-2.11,z: 0.67}, {y:-1}, light_props); 	//right brake light
	light_entity.brake_mask = 0b11 << brake_light_offset;

	light_props = { size: 0.04, angle: 120, edge: 0.8, fadeout: 0.05, range: 100 };
	let rev_light_offset =
	this.add_spot_light({x:-0.50,y:-2.11,z: 0.72}, {y:-1}, light_props);	//left reverse light
	this.add_spot_light({x:0.50,y:-2.11,z: 0.72}, {y:-1}, light_props);	//right reverse light
	light_entity.rev_mask = 3 << rev_light_offset;

	light_props = {size:0.1, edge:0.8, fadeout:0, color:{x:0.4,y:0.1,z:0},range:0.004,  intensity:1 };
	let turn_light_offset =
	this.add_point_light({x:-0.71,y:2.25,z:0.63}, light_props); 	//left front turn light 
	this.add_point_light({x:-0.64,y:-2.11,z: 0.72}, light_props); 	//left rear turn light 
	this.add_point_light({x:0.71,y:2.25,z: 0.63}, light_props); 	//right front turn light 
	this.add_point_light({x:0.64,y:-2.11,z: 0.72}, light_props); 	//right rear turn light 
	light_entity.turn_left_mask = 0x3 << turn_light_offset;
	light_entity.turn_right_mask = 0x3 << (turn_light_offset + 2);

	light_props = { size: 0.05, angle: 110, edge: 0.08, fadeout: 0.05, range: 110 };
	let main_light_offset = 
	this.add_spot_light({x:-0.45,y:2.2,z:0.68}, {y:1}, light_props); 	//left main light
	this.add_spot_light({x:0.45,y:2.2,z: 0.68}, {y:1}, light_props);	//right main light


	//Load sound samples (located in "sounds" folder) using load_sound function
	//1.param - string filename (audio file name, possibly with path)
	//returns - sound ID
	sound_entity.snd_starter = this.load_sound("sounds/starter.ogg");		    //will have ID 0
	sound_entity.snd_eng_running = this.load_sound("sounds/eng_running.ogg");	//will have ID 1
	sound_entity.snd_eng_stop = this.load_sound("sounds/eng_stop.ogg");	        //will have ID 2

	//Create sound emitters, using add_sound_emitter function
	//1.param - joint/bone, from which we want the sound to emit
    //2.parameter - sound type: -1 interior only, 0 universal, 1 exterior only
    //3.parameter - reference distance (saturated volume distance)
	//returns - emitter ID
	sound_entity.src_eng_start_stop = this.add_sound_emitter("exhaust_0_end");	//will have ID 0
	sound_entity.src_eng_running = this.add_sound_emitter("exhaust_0_end");	    //will have ID 1
    
    this.register_event("vehicle/engine/reverse", reverse_action); 
	this.register_event("vehicle/engine/on", engine_action);
	this.register_axis("vehicle/controls/open", {minval:0, center:0, vel:0.6}, function(v) {
		let door_dir = {z:-1};
		let door_angle = v * 1.5;
		this.geom.rotate_joint_orig(bones.driver_door, door_angle, door_dir);
        
          
           
         
        //radio
        if(v !== 0)
        {
                //$log("volume" + (v*100));
            radio_plugin.set_volume(v*100);
        }
        
	}); 
	this.register_axis("vehicle/lights/passing", {minval: 0, maxval: 1, vel: 10, center: 0, positions: 2 }, function(v) {
		this.light_mask(0xf, v===1);
        
        
        //radio
        //$log("radio should stop");
        radio_plugin.stop_web_audiostream()
        
        
        
        
        
        
        
	});
	this.register_axis("vehicle/lights/main", {minval: 0, maxval: 1, vel: 10, center: 0, positions: 2 }, function(v) {	
		this.light_mask(0x3, v===1, main_light_offset);
        
         
         
        //radio
            $log("radio playing? "+radio_plugin.web_audiostream_playing());
        
        
        
         
	});
	this.register_axis("vehicle/lights/turn", {minval: -1, maxval: 1, vel: 10, center: 0 }, function(v) {
		
        
        
        
    //radio_plugin.play_audiostream("https://stream.bauermedia.sk/rock-hi.mp3"); 
        
        
        
        
        
        
        
        
        if(v === 0)
		{
			this.left_turn = this.right_turn=0;
		}
		else if(v < 0)
		{
			this.left_turn = 1; this.right_turn = 0;
		}
		else
		{
			this.right_turn = 1; this.left_turn = 0;
		}
	});
	this.register_event("vehicle/lights/emergency", function(v) { 
    station++;
    station = station%8;
    //$log("station "+station);
    
    
    //radio
    radio_plugin.toggle_stations(station);
    
    
    
    
    this.emer ^= 1; });

	return {
		mass: 1120.0,
		com: {x: 0.0, y: 0.0, z: 0.3},
		steering_params:{
			steering_ecf: 50,
		},
	}; 
}
 
function init_vehicle(reload)
{	
	this.time = 0;
	this.left_turn = this.right_turn = this.emer = 0;
	this.geom = this.get_geomob(0);
	this.started = 0;
	this.eng_dir = 1;
  	this.set_fps_camera_pos({x:-0.4, y:0.0, z:1.2});
	
	//use sound function to get sound interface
	this.snd = this.sound();
    
    //set initial sound values
	this.snd.set_pitch(sound_entity.src_eng_start_stop, 1);
	this.snd.set_pitch(sound_entity.src_eng_running, 1);
	this.snd.set_gain(sound_entity.src_eng_start_stop, 1);
	this.snd.set_gain(sound_entity.src_eng_running, 1);
	this.snd.set_ref_distance(sound_entity.src_eng_start_stop, 1);
	this.snd.set_ref_distance(sound_entity.src_eng_running, 1);
       
        
    $log("reloaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaad "+reload);
    radio_plugin = $query_interface( 'xt::js::radio.create'); 
    
    radio_plugin.reset_values();
}

function update_frame(dt, engine, brake, steering, parking)
{
	let brake_dir = {x:1};
	let brake_angle = brake*0.4;	
	let accel_dir = {y:(-engine*0.02), z:(-engine*0.02)}
	this.geom.rotate_joint_orig(bones.brake_pedal, brake_angle, brake_dir);
	this.geom.move_joint_orig(bones.accel_pedal, accel_dir)
	
	let kmh = this.speed()*3.6;
	
    //set reference distance
	let ref_distance = this.get_camera_mode() == 0 ? 0.25 : 1;
	this.snd.set_ref_distance(sound_entity.src_eng_running, ref_distance);
	this.snd.set_ref_distance(sound_entity.src_eng_start_stop, ref_distance);
    
	if(this.started === 1)
	{
        let redux = this.eng_dir >= 0 ? 0.2 : 0.6;
        engine = ENGINE_FORCE * Math.abs(engine);
        let force = (kmh >= 0) === (this.eng_dir >= 0 ) 
            ? engine / (redux * Math.abs(kmh) + 1)
            : engine;
        force -= FORCE_LOSS;
        force = Math.max(0.0, Math.min(force, engine));
        engine = force * this.eng_dir;
        
        //Move only when there is no sound playing on given emitter (to not be able to move when car is starting, but after the starter sound ends)
        if(this.snd.is_playing(sound_entity.src_eng_start_stop))
        {
            engine = 0;
        } //If car has started and there isn't active loop on given emitter, play loop
        else
        {
            //Calculate and set volume pitch and gain for emitter
            //max_rpm function returns rpm of the fastest revolving wheel
            let rpm = this.max_rpm();
            let pitch = Math.abs(kmh/40) + Math.abs(rpm/200.0);
            let pitch_rpm = rpm > 0 ? Math.floor(pitch) : 0;
            pitch += (0.5 * pitch_rpm) - pitch_rpm;
            //Use set_pitch function to set pitch value on given emitter
            //1.parameter - emitter 
            //2.parameter - pitch value (this will affect all sounds emitted from this emitter)
            this.snd.set_pitch(sound_entity.src_eng_running, (0.5 * pitch) + 1.0);

            //set gain
            this.snd.set_gain(sound_entity.src_eng_running, (0.25 * pitch) + 0.5);    

            //play_loop function is used to play sound in loop, breaking previous sounds
            //1.parameter - emitter (source ID)
            //2.parameter - sound (sound ID))
            if(!this.snd.is_looping(sound_entity.src_eng_running))
            {
                this.snd.play_loop(sound_entity.src_eng_running, sound_entity.snd_eng_running);
            }
            
            //another loop function, that can be used is enqueue_loop
            //1.parameter - emitter (source ID) 
            //2.parameter - sound (sound ID)
            //can take 3.parameter - bool value (default: true), if true - previous loops will be removed, otherwise the new sound is added to the loop chain
            //example: this.snd.enqueue_loop(sound_entity.src_eng_running, sound_entity.snd_eng_running);
        }
	}
	
	this.wheel_force(wheels.FLwheel, engine);
	this.wheel_force(wheels.FRwheel, engine);
	
	if(kmh > SPEED_GAUGE_MIN)
	{
        this.geom.rotate_joint_orig(bones.speed_gauge, (kmh - SPEED_GAUGE_MIN) * RAD_PER_KMH, {x:0,y:1,z:0});    
    }
	
	steering *= 0.3;
	this.steer(wheels.FLwheel, steering);	//front left wheel
	this.steer(wheels.FRwheel, steering);	//front right wheel
	this.geom.rotate_joint_orig(bones.steer_wheel, 10.5*steering, {z:1});

	this.light_mask(light_entity.brake_mask, brake > 0);
 
	brake *= BRAKE_FORCE; 
	brake += 200;
	this.wheel_brake(-1, brake);

	if(this.left_turn || this.right_turn || this.emer)
	{
		this.time += dt;
		let blt = this.time*0.85;
		let blink = (blt - Math.floor(blt)) > 0.47 ? 1 : 0;
		
		this.light_mask(light_entity.turn_left_mask, blink&(this.left_turn|this.emer));
		this.light_mask(light_entity.turn_right_mask, blink&(this.right_turn|this.emer));
	}
	else
	{
		this.light_mask(light_entity.turn_left_mask, false);
		this.light_mask(light_entity.turn_right_mask, false);
	}
    
    this.animate_wheels();
     
     
      
       
    
    //$log("stream state "+radio_plugin.get_autostream_state());
    //$log("volume "+radio_plugin.get_volume(0));
    
    //this.time +=1;
    //$log(this.time);
}
