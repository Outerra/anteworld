-- Version 4 - Sounds

implements("ot.vehicle_script")

SPEED_GAUGE_MIN = 10.0;
RAD_PER_KMH = 0.018325957;
ENGINE_FORCE = 25000.0;
BRAKE_FORCE = 5000.0;
MAX_KMH = 200;
FORCE_LOSS = ENGINE_FORCE / (0.2*MAX_KMH + 1);

wheels = {
    FLwheel = -1, 
    FRwheel = -1, 
    RLwheel = -1, 
    RRwheel = -1, 
};

bones = {
    steer_wheel = -1, 
    speed_gauge = -1, 
    accel_pedal = -1, 
    brake_pedal = -1, 
    driver_door = -1,
};

light_entity = {
    brake_mask = 0, 
    rev_mask = 0, 
    turn_left_mask = 0, 
    turn_right_mask = 0,
    main_light_offset = 0
};

-- Create object containing sound related members
sound_entity = {
    snd_starter = -1, 
    snd_eng_running = -1, 
    snd_eng_stop = -1, 
    src_eng_start_stop = -1, 
    src_eng_running = -1
};

function engine_action(self)
	self.started = not self.started;
	self:fade(self.started and "Engine start" or "Engine stop");
    
    -- Based on the camera mode, choose gain value
    local sound_gain = self.current_camera_mode == 0 and 0.5 or 1;
    
	-- Use set_gain function to set gain value on given emitter
	--  1.parameter - emitter
	--  2.parameter - gain value(this will affect all sounds emitted from this emitter)
	self.snd:set_gain(sound_entity.src_eng_start_stop, sound_gain);

	-- Based on the camera mode, choose reference distance value
	local ref_distance = self.current_camera_mode == 0 and 0.25 or 1;

	-- Use set_ref_distance to set reference distance on given emitter (how far should the sounds be heard)
	--  1.parameter - emitter
	--  2.parameter - reference distance value(this will affect all sounds emitted from this emitter)
	self.snd:set_ref_distance(sound_entity.src_eng_start_stop, ref_distance);

	if self.started == false then 
		-- function "stop" discards all sounds playing on given emitter
		self.snd:stop(sound_entity.src_eng_running);
		self.snd:play_sound(sound_entity.src_eng_start_stop, sound_entity.snd_eng_stop);
        
        self:wheel_force(wheels.FLwheel, 0);
        self:wheel_force(wheels.FRwheel, 0);
	else 
		-- play_sound function is used to play sound once, discarding older sounds
		-- 1.parameter - emitter (source ID) 
		-- 2.parameter - sound (sound ID))
		self.snd:play_sound(sound_entity.src_eng_start_stop, sound_entity.snd_starter);
	end
end

function reverse_action(self, v)
    self.eng_dir = (self.eng_dir >= 0) and -1 or 1;
    self:fade(self.eng_dir > 0 and "Forward" or "Reverse");
    
    self:light_mask(light_entity.rev_mask, self.eng_dir < 0);
end

function hand_brake_action(self, v)
    self.hand_brake_input = self.hand_brake_input == 0 and 1 or 0;
end

function power_action(self, v)
    self.power_input = v;
end

function door_action(self, v)
    local door_dir = {z = -1};
    local door_angle = v * 1.5;
    self.geom:rotate_joint_orig(bones.driver_door, door_angle, door_dir);
end

function passing_lights_action(self, v)
    self:light_mask(0xf, v == 1); 
end

function main_lights_action(self, v)
    self:light_mask(0x3, v == 1, light_entity.main_light_offset);
end

function turn_lights_action(self, v)
    if v == 0 then
        self.left_turn = 0;
        self.right_turn = 0;
    elseif v < 0 then
        self.left_turn = 1; 
        self.right_turn = 0;
    else
        self.left_turn = 0;
        self.right_turn = 1; 
    end
end

function emergency_lights_action(self, v)
    self.emer = self.emer == 0 and 1 or 0; 
end

function ot.vehicle_script:init_chassis()
    
    local wheel_params = {
		radius = 0.31515,
		width = 0.2,
		suspension_max = 0.1,
		suspension_min = -0.04,
		suspension_stiffness = 50.0,
		damping_compression = 0.4,
		damping_relaxation = 0.12,
		grip = 1,
	}

    wheels.FLwheel = self:add_wheel('wheel_l0', wheel_params); 
	wheels.FRwheel = self:add_wheel('wheel_r0', wheel_params); 
	wheels.RLwheel = self:add_wheel('wheel_l1', wheel_params); 
	wheels.RRwheel = self:add_wheel('wheel_r1', wheel_params); 

	bones.steer_wheel = self:get_joint_id('steering_wheel');	
	bones.speed_gauge = self:get_joint_id('dial_speed');		
	bones.accel_pedal = self:get_joint_id('pedal_accelerator');
	bones.brake_pedal = self:get_joint_id('pedal_brake');		
	bones.driver_door = self:get_joint_id('door_l0');			
    
	local light_props = {size = 0.05, angle = 120, edge = 0.2, range = 70, fadeout = 0.05 };

	self:add_spot_light({x = -0.55, y = 2.2, z = 0.68}, {y = 1}, light_props); 
	self:add_spot_light({x = 0.55, y = 2.2, z = 0.68}, {y = 1}, light_props);	

	light_props = { size = 0.07, angle = 160, edge = 0.8, color = { x = 1.0 }, range = 150, fadeout = 0.05 };
	
	self:add_spot_light({x = -0.05, y = -0.06, z = 0}, {y = 1}, light_props, "tail_light_l0"); 
	self:add_spot_light({x = 0.05, y = -0.06, z = 0}, {y = 1}, light_props, "tail_light_r0"); 
	
	light_props = { size = 0.04, angle = 120, edge = 0.8, color = { x = 1.0 }, range = 100, fadeout = 0.05 };
	
	local brake_light_offset =  
	self:add_spot_light({x = -0.43, y = -2.11, z = 0.62}, {y = -1}, light_props); 
	self:add_spot_light({x = 0.43, y = -2.11, z = 0.62}, {y = -1}, light_props);  

	light_entity.brake_mask = bit.lshift(0x3, brake_light_offset);

	light_props = { size = 0.04, angle = 120, edge = 0.8, range = 100, fadeout = 0.05 };
	local rev_light_offset =
	self:add_spot_light({x = -0.5, y = -2.11, z = 0.715}, {y = -1}, light_props);	
	self:add_spot_light({x = 0.5, y = -2.11, z = 0.715}, {y = -1}, light_props);	
	
	light_entity.rev_mask = bit.lshift(3, rev_light_offset);

	light_props = {size = 0.1, edge = 0.8, intensity = 1, color = {x = 0.4, y = 0.1, z = 0}, range = 0.004, fadeout = 0 };
	local turn_light_offset =
	self:add_point_light({x = -0.71, y = 2.23, z = 0.62}, light_props); 
	self:add_point_light({x = -0.66, y = -2.11, z = 0.715}, light_props); 
	self:add_point_light({x = 0.71, y = 2.23, z = 0.62}, light_props);  
	self:add_point_light({x = 0.66, y = -2.11, z = 0.715}, light_props); 
	
	light_entity.turn_left_mask = bit.lshift(0x3, turn_light_offset);

	light_entity.turn_right_mask = bit.lshift(0x3, (turn_light_offset + 2));

	light_props = { size = 0.05, angle = 110, edge = 0.08, range = 110, fadeout = 0.05 };
	light_entity.main_light_offset = 
	self:add_spot_light({x = -0.45, y = 2.2, z = 0.68}, {y = 1}, light_props); 
	self:add_spot_light({x = 0.45, y = 2.2, z = 0.68}, {y = 1}, light_props);	 
    
    -- Load sound samples (located in "Sounds" folder) using load_sound function
	-- 1.param - string filename (audio file name, possibly with path)
	-- returns - sound ID
	sound_entity.snd_starter = self:load_sound("Sounds/starter.ogg");		    -- will have ID 0
	sound_entity.snd_eng_running = self:load_sound("Sounds/eng_running.ogg");	-- will have ID 1
	sound_entity.snd_eng_stop = self:load_sound("Sounds/eng_stop.ogg");	        -- will have ID 2

	-- Create sound emitters, using add_sound_emitter function
	-- 1.param - joint/bone, from which we want the sound to emit
    -- 2.parameter - sound type: -1 interior only, 0 universal, 1 exterior only
    -- 3.parameter - reference distance (saturated volume distance)
	-- returns - emitter ID
	sound_entity.src_eng_start_stop = self:add_sound_emitter("exhaust_0_end");	-- will have ID 0
	sound_entity.src_eng_running = self:add_sound_emitter("exhaust_0_end");	    -- will have ID 1

    self:register_event("vehicle/engine/on", engine_action);
    self:register_event("vehicle/engine/reverse", reverse_action);
    self:register_event("vehicle/controls/hand_brake", hand_brake_action);
	self:register_event("vehicle/lights/emergency", emergency_lights_action);

	self:register_axis("vehicle/controls/power", {minval = 0, center = 100}, power_action); 
	self:register_axis("vehicle/controls/open", {minval = 0, maxval = 1, center = 0, vel = 0.6}, door_action); 
	self:register_axis("vehicle/lights/passing", {minval = 0, maxval = 1, center = 0, vel = 10, positions = 2 }, passing_lights_action);
	self:register_axis("vehicle/lights/main", {minval = 0, maxval = 1, center = 0, vel = 10, positions = 2 }, main_lights_action);
	self:register_axis("vehicle/lights/turn", {minval = -1, maxval = 1, center = 0, vel = 10 }, turn_lights_action);

    return {
        mass = 1120.0,
		com = {x = 0.0, y = -0.2, z = 0.3},
		steering_params = {
			steering_ecf = 50,
		}
    };
end

function ot.vehicle_script:init_vehicle()  

    -- Initialize variables needed for sound functionality
    self.current_camera_mode = 0;
    self.previous_cam_mode = 0;

	self.time = 0;
	self.left_turn = 0;
    self.right_turn = 0;
    self.emer = 0;

	self.geom = self:get_geomob(0);
    self.started = false;
	self.eng_dir = 1;
    self.power_input = 0;
    self.braking_power = 0;
    self.hand_brake_input = 1;
    
	self:set_fps_camera_pos({x = -0.4, y = 0.16, z = 1.3});
   
    -- Use sound function to get sound interface
	self.snd = self:sound();
    
    -- Set initial sound values
	self.snd:set_pitch(sound_entity.src_eng_start_stop, 1);
	self.snd:set_pitch(sound_entity.src_eng_running, 1);
	self.snd:set_gain(sound_entity.src_eng_start_stop, 1);
	self.snd:set_gain(sound_entity.src_eng_running, 1);
	self.snd:set_ref_distance(sound_entity.src_eng_start_stop, 1);
	self.snd:set_ref_distance(sound_entity.src_eng_running, 1);
end

function ot.vehicle_script:update_frame(dt, engine, brake, steering, parking)

	local brake_dir = {x = 1};
	local brake_angle = brake * 0.4;	
	local accel_dir = {y = (-self.power_input * 0.02), z = (-self.power_input * 0.02)}
    
	self.geom:rotate_joint_orig(bones.brake_pedal, brake_angle, brake_dir);
	self.geom:move_joint_orig(bones.accel_pedal, accel_dir)

    local kmh = math.abs(self:speed() * 3.6);
    
    -- Get_camera_mode returns current camera mode (0 - FPS camera mode, 1 - TPS camera mode, 2 - TPS follow camera mod )
    self.current_camera_mode = self:get_camera_mode();
    
    -- To not set reference distance every frame, check if the camera mode has changed
    if self.previous_cam_mode ~= self.current_camera_mode then   
        local ref_distance;
        -- Choose reference distance, based on current camera mode
        if self.current_camera_mode == 0 then
            ref_distance = 0.25;
        else
            ref_distance = 1;
        end    
        -- Set reference distance
        self.snd:set_ref_distance(sound_entity.src_eng_running, ref_distance);

        -- set self.previous_cam_mode to current camera mode
        self.previous_cam_mode = self.current_camera_mode;
    end

    if self.started == true then
		local redux = self.eng_dir >= 0 and 0.2 or 0.6;
		local eng_power = ENGINE_FORCE * self.power_input;
		local force = (kmh >= 0) == (self.eng_dir >= 0) and (eng_power / (redux * kmh + 1)) or eng_power;
        force = force - FORCE_LOSS;
		force = math.max(0.0, math.min(force, eng_power));
		force = force * self.eng_dir;
        
        -- Move only when there is no sound playing on given emitter (to not be able to move when car is starting, but after the starter sound ends)
        if self.snd:is_playing(sound_entity.src_eng_start_stop) then
            force = 0;
        --If the car has started and there isn't active loop on given emitter, play loop
        else
            -- Calculate and set volume pitch and gain for emitter
            -- max_rpm function returns rpm of the fastest revolving wheel
            local rpm = self:max_rpm();
            local speed_modulation = kmh/40 + math.abs(rpm/200.0);
            local pitch_gain_factor = rpm > 0 and math.floor(speed_modulation) or 0;
            local pitch_gain = speed_modulation + (0.5 * pitch_gain_factor) - pitch_gain_factor;
            -- Use set_pitch function to set pitch value on given emitter
            -- 1.parameter - emitter 
            -- 2.parameter - pitch value (this will affect all sounds emitted from this emitter)
            self.snd:set_pitch(sound_entity.src_eng_running, (0.5 * pitch_gain) + 1.0);

            -- Set gain
            self.snd:set_gain(sound_entity.src_eng_running, (0.25 * pitch_gain) + 0.5);    

            -- play_loop function is used to play sound in loop, breaking previous sounds
            -- 1.parameter - emitter (source ID)
            -- 2.parameter - sound (sound ID))
            if self.snd:is_looping(sound_entity.src_eng_running) == false then
                self.snd:play_loop(sound_entity.src_eng_running, sound_entity.snd_eng_running);
            end
            
            -- another loop function, that can be used is enqueue_loop
            -- 1.parameter - emitter (source ID) 
            -- 2.parameter - sound (sound ID)
            -- Can take 3.parameter - bool value (default: true), if true - previous loops will be removed, otherwise the new sound is added to the loop chain
            -- Example: self.snd:enqueue_loop(sound_entity.src_eng_running, sound_entity.snd_eng_running);
        end
        
        if self.hand_brake_input ~= 0 and force > 0 then
            self.hand_brake_input = 0
        end
        
        self:wheel_force(wheels.FLwheel, force);
        self:wheel_force(wheels.FRwheel, force);
    end  
    
    
	if kmh > SPEED_GAUGE_MIN then
        self.geom:rotate_joint_orig(bones.speed_gauge, (kmh - SPEED_GAUGE_MIN) * RAD_PER_KMH, {x = 0,y = 1,z = 0});    
    end
    
    steering = steering * 0.3;
    
    self:steer(wheels.FLwheel, steering);
    self:steer(wheels.FRwheel, steering);
    
    self.geom:rotate_joint_orig(bones.steer_wheel, 10.5*steering, {z = 1});
	
	self:light_mask(light_entity.brake_mask, brake > 0);
    
    if self.hand_brake_input ~= 0 then 
        self.braking_power = BRAKE_FORCE; 
    elseif brake ~= 0 then
        self.braking_power = brake * BRAKE_FORCE;
    else 
        self.braking_power = 0;
    end
    
    self.braking_power = self.braking_power + 200;
    
    self:wheel_brake(-1, self.braking_power);
    
    if self.left_turn or self.right_turn or self.emer then
		self.time = (self.time + dt) % 1;
        local left_turn_active = self.left_turn == 1;
        local right_turn_active = self.right_turn == 1; 
        local emer_active = self.emer == 1;
        local blink = self.time > 0.47 and true or false;
        
		self:light_mask(light_entity.turn_left_mask, (blink and (left_turn_active or emer_active)) );
		self:light_mask(light_entity.turn_right_mask, (blink and (right_turn_active or emer_active)) );
	else
		self:light_mask(light_entity.turn_left_mask, false);
		self:light_mask(light_entity.turn_right_mask, false);
        self.time = 0;
    end
    
	self:animate_wheels();
end