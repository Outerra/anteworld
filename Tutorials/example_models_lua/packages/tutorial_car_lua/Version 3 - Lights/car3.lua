-- Version 3 - Lights

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

-- Create object containing light related members
light_entity = {
    brake_mask = 0, 
    rev_mask = 0, 
    turn_left_mask = 0, 
    turn_right_mask = 0,
    main_light_offset = 0
};

function engine_action(self)
	self.started = not self.started;
	self:fade(self.started and "Engine start" or "Engine stop");
    
    if self.started == false then
        self:wheel_force(wheels.FLwheel, 0);
        self:wheel_force(wheels.FRwheel, 0);
    end
end

function reverse_action(self, v)
    self.eng_dir = (self.eng_dir >= 0) and -1 or 1;
    self:fade(self.eng_dir > 0 and "Forward" or "Reverse");
    
    -- Apply reverse light mask (rev_mask) on reverse lights, when engine direction has value -1 (activate reverse lights)
	-- light_mask function is used to turn lights on/off
	-- 1.param - bit mask value - which bits/lights should be affected 
	-- 2.param - condition, when true, given bits/lights will be affected
	-- Every time you press reverse button, parameter "v" switches it's value (in this case between minval and maxval, but it can also switch between positions, if they are defined)	
	-- if v == 1, lights will turn on (bit mask will affect the specified lights) 
    self:light_mask(light_entity.rev_mask, self.eng_dir < 0);
end

function door_action(self, v)
    local door_dir = {z = -1};
    local door_angle = v * 1.5;
    self.geom:rotate_joint_orig(bones.driver_door, door_angle, door_dir);
end

function passing_lights_action(self, v)
    -- Note: this affects first 4 defined lights (because we didn't give an offset as 3. parameter). In this case it will affect 2 front lights and 2 tail lights, because every light is represented as 1 bit and as 1.parameter we used hexadecimal 0x0..0xf which works with 4 bits (0000....1111), we can also use 0x00..0xff which will work with first 8 bits
    self:light_mask(0xf, v == 1); 
end

function main_lights_action(self, v)
    -- Another way to use light mask, is to give light offset (in this case light_entity.main_light_offset) as 3. parameter, from this offset the bit mask will affect next bits/lights
    self:light_mask(0x3, v == 1, light_entity.main_light_offset);
end

function turn_lights_action(self, v)
    -- Function used to toggle the left/right turn lights
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
    -- Function used to toggle the emergency lights
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
    
    -- Define light parameters and assign them to "light_props"
	-- Light parameters you can use:	
	-- size (float)	 	 - diameter of the light source (reflector)
	-- angle (float)	 - field of light [degrees] (ignored for point lights)
	-- edge (float) 	 - soft edge coefficient (0..1), portion of the light area along the edges where light fades to make it soft (ignored for point lights)
	-- color (float3) 	 - RGB color of the light emitter
	-- intensity (float) - light intensity
	-- range (float) 	 - effective light range
	-- fadeout (float) 	 - time to fade after turning off
	local light_props = {size = 0.05, angle = 120, edge = 0.2, range = 70, fadeout = 0.05 };
	-- If either intensity or range are specified alone, the other one is computed automatically. If neither one is specified, the intensity is taken from the color value, otherwise the colors are normalized. If both intensity and range are specified, the light can have shorter range to avoid leaking outside of cockpits etc.
	-- Note: intensity of light is affected not only by "intensity" param, but also by angle and range
    
    -- Use add_spot_light function to add lights
	-- 1.parameter - the model-space offset relative to bone or model pivot
	-- 2.parameter - light direction
	-- 3.parameter - light properties
    -- 4.parameter - string name of the bone, to which you want to bind the light (this will make the lights offset and direction to be relative to the defined bone instead of model pivot)
	-- Add front lights (offset relative to model pivot is given for the lights and direction is set to forward by {y = 1})
	self:add_spot_light({x = -0.55, y = 2.2, z = 0.68}, {y = 1}, light_props);  -- left front light
	self:add_spot_light({x = 0.55, y = 2.2, z = 0.68}, {y = 1}, light_props);	-- right front light

	-- Change the light properties in "light_props" and use them for another lights
	light_props = { size = 0.07, angle = 160, edge = 0.8, color = { x = 1.0 }, range = 150, fadeout = 0.05 };
	
	-- Add tail lights
	self:add_spot_light({x = -0.05, y = -0.06, z = 0}, {y = 1}, light_props, "tail_light_l0");  -- left tail light
	self:add_spot_light({x = 0.05, y = -0.06, z = 0}, {y = 1}, light_props, "tail_light_r0");  -- right tail light 	
	-- Warning: In this case, the direction of this light is now opposite to front lights, even though direction is still {y = 1}, because the light is now relative to tail light bone, which has opposite direction
	
	-- Add brake lights
	light_props = { size = 0.04, angle = 120, edge = 0.8, color = { x = 1.0 }, range = 100, fadeout = 0.05 };
	
	-- Here's another example regarding light direction: while the brake lights are relative to the model pivot, the direction is specified as {y = -1}, indicating the opposite direction, therefore the lights will illuminate in the backward direction
	-- Add brake lights and store the offset of the first light in "brake_light_offset"
	local brake_light_offset =  
	self:add_spot_light({x = -0.43, y = -2.11, z = 0.62}, {y = -1}, light_props);   -- left brake light (0b01)
	self:add_spot_light({x = 0.43, y = -2.11, z = 0.62}, {y = -1}, light_props);    -- right brake light (0b10)
	
	-- Now we have to specify bit mask (brake_mask), for that we have to use bit logic
	-- Our 2 brake lights are defined as follows:
	-- 	01 (binary) - 1 (decimal) - left brake light 
	-- 	10 (binary) - 2 (decimal) - right brake light 
	-- 	11 (binary) - 3 (decimal) - both brake lights  
	-- We want the bit mask (brake_mask) to affect both lights, therefore the given value will be "3" (in this case, the value is written in decimal system, but it can also be written in hexadecimal).
	-- Also we want, that the mask starts affecting lights from the first brake light, therefore we have to "left shift" the bit mask by brake light offset
    -- For bit shifting, you can use bit library
    light_entity.brake_mask = bit.lshift(3, brake_light_offset)
    
	-- Add reverse lights
	light_props = { size = 0.04, angle = 120, edge = 0.8, range = 100, fadeout = 0.05 };
    
	local rev_light_offset =
	self:add_spot_light({x = -0.5, y = -2.11, z = 0.715}, {y = -1}, light_props);	 --left reverse light (0b01)
	self:add_spot_light({x = 0.5, y = -2.11, z = 0.715}, {y = -1}, light_props);	 --right reverse light (0b10)
	
	--  01 (binary) -> 1 (decimal) - left reverse light 
	--  10 (binary) -> 2 (decimal) - right reverse light 
	--  11 (binary) -> 3 (decimal) - both reverse lights 
	-- We want both lights to shine, when we hit the reverse button (in this case, the value is written in decimal system)
	light_entity.rev_mask = bit.lshift(3, rev_light_offset);

	-- Add turn signal lights
	-- In this case I used add_point_light() function, because we don't need this light to shine in specific direction
	-- add_point_light() also takes parameters position, and light properties, same as add_spot_light(), but without direction 
	light_props = {size = 0.1, edge = 0.8, intensity = 1, color = {x = 0.4, y = 0.1, z = 0}, range = 0.004, fadeout = 0 };
    
	local turn_light_offset =
	self:add_point_light({x = -0.71, y = 2.23, z = 0.62}, light_props); 	-- left front turn light (0b0001)
	self:add_point_light({x = -0.66, y = -2.11, z = 0.715}, light_props); 	-- left rear turn light (0b0010)
	self:add_point_light({x = 0.71, y = 2.23, z = 0.62}, light_props);  	-- right front turn light (0b0100)
	self:add_point_light({x = 0.66, y = -2.11, z = 0.715}, light_props); 	-- right rear turn light (0b1000)
	
	-- 0001 (binary) -> 0x1 (hexadecimal) - left front turn light
	-- 0010 (binary) -> 0x2 (hexadecimal) - left rear turn light
	-- 0011 (binary) -> 0x3 (hexadecimal) - left rear + left front (left side)
	-- 0100 (binary) -> 0x4 (hexadecimal) - right front turn light
	-- 0101 (binary) -> 0x5 (hexadecimal) - right front + left front (front side)
	-- 0110 (binary) -> 0x6 (hexadecimal) - right front + left rear 
	-- 0111 (binary) -> 0x7 (hexadecimal) - right front + left rear + left front 
	-- 1000 (binary) -> 0x8 (hexadecimal) - right rear turn light
	-- 1001 (binary) -> 0x9 (hexadecimal) - right rear + left front
	-- 1010 (binary) -> 0xa (hexadecimal) - right rear + left rear (rear side) 
	-- 1011 (binary) -> 0xb (hexadecimal) - right rear + left rear + left front
	-- 1100 (binary) -> 0xc (hexadecimal) - right rear + right front (right side)
	-- 1101 (binary) -> 0xd (hexadecimal) - right rear + right front + left front
	-- 1110 (binary) -> 0xe (hexadecimal) - right rear + right front + left rear
	-- 1111 (binary) -> 0xf (hexadecimal) - all turn lights (emergency lights)
	-- We want lights on the side of the car to glow, when we hit the corresponding left or right turn signal button (in this case, the value is written in hexadecimal system )
	-- When the left turn signal button was pressed, we want turn lights on the left side to glow 
	light_entity.turn_left_mask = bit.lshift(0x3, turn_light_offset);
	-- When the right turn signal button was pressed, we want turn lights on the right side to glow 
	-- Add number of previous left turn lights to the offset (or you can make another offset for right turn lights and use that....) 
	light_entity.turn_right_mask = bit.lshift(0x3, turn_light_offset + 2);
	
	-- Add main lights
	-- Here you don't have to identify lights for bit mask, because they were added as 4.parameter in add_spot_light() function while creating action handler
	light_props = { size = 0.05, angle = 110, edge = 0.08, range = 110, fadeout = 0.05 };
	light_entity.main_light_offset = 
	self:add_spot_light({x = -0.45, y = 2.2, z = 0.68}, {y = 1}, light_props);  -- left main light
	self:add_spot_light({x = 0.45, y = 2.2, z = 0.68}, {y = 1}, light_props);   -- right main light

    self:register_event("vehicle/engine/on", engine_action);
    self:register_event("vehicle/engine/reverse", reverse_action);

	-- Handle this action, when emergency lights buttons are pressed ('Shift' + 'W')
	self:register_event("vehicle/lights/emergency", emergency_lights_action);

	self:register_axis("vehicle/controls/open", {minval = 0, maxval = 1, center = 0, vel = 0.6}, door_action); 

    -- Add additional action handlers
	-- Handle this action, when passing lights button is pressed ('L')
	self:register_axis("vehicle/lights/passing", {minval = 0, maxval = 1, center = 0, vel = 10, positions = 2 }, passing_lights_action);
	
	-- This action is handled, when you press Ctrl + L 
	self:register_axis("vehicle/lights/main", {minval = 0, maxval = 1, center = 0, vel = 10, positions = 2 }, main_lights_action);

	-- Handle this action, when turn signals buttons are pressed ('Shift' + 'A' or 'Shift' + 'D')
	-- Turn signals can have -1/0/1 values, when 'Shift' + 'A' is pressed, the "v" value switches between 0 and -1, but when 'Shift' + 'D' is pressed, the value moves between 0 and 1.
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

    -- Initialize variables needed for light functionality
	self.time = 0;
	self.left_turn = 0;
    self.right_turn = 0;
    self.emer = 0;

	self.geom = self:get_geomob(0);
    self.started = false;
	self.eng_dir = 1;
    self.braking_power = 0;
    
	self:set_fps_camera_pos({x = -0.4, y = 0.16, z = 1.3});
end

function ot.vehicle_script:update_frame(dt, engine, brake, steering, parking)

	local brake_dir = {x = 1};
	local brake_angle = brake * 0.4;	
	local accel_dir = {y = (-engine * 0.02), z = (-engine * 0.02)}
    
	self.geom:rotate_joint_orig(bones.brake_pedal, brake_angle, brake_dir);
	self.geom:move_joint_orig(bones.accel_pedal, accel_dir)

    local kmh = math.abs(self:speed() * 3.6);

    if self.started == true then
		local redux = self.eng_dir >= 0 and 0.2 or 0.6;
		engine = ENGINE_FORCE * math.abs(engine);
		local force = (kmh >= 0) == (self.eng_dir >= 0) and (engine / (redux * kmh + 1)) or engine;
        force = force - FORCE_LOSS;
		force = math.max(0.0, math.min(force, engine));
		engine = force * self.eng_dir;
    
        self:wheel_force(wheels.FLwheel, engine);
        self:wheel_force(wheels.FRwheel, engine);
    end  
    
    
	if kmh > SPEED_GAUGE_MIN then
        self.geom:rotate_joint_orig(bones.speed_gauge, (kmh - SPEED_GAUGE_MIN) * RAD_PER_KMH, {x = 0,y = 1,z = 0});    
    end
    
    steering = steering * 0.3;
    
    self:steer(wheels.FLwheel, steering);
    self:steer(wheels.FRwheel, steering);
    
    self.geom:rotate_joint_orig(bones.steer_wheel, 10.5*steering, {z = 1});
    
    -- Apply brake light mask (brake_mask) on brake lights, when brake value is bigger than 0
	-- Note: add this code before adding rolling friction to brakes.
	self:light_mask(light_entity.brake_mask, brake > 0);
    
    if parking ~= 0 then 
        self.braking_power = BRAKE_FORCE; 
    elseif brake ~= 0 then
        self.braking_power = brake * BRAKE_FORCE;
    else 
        self.braking_power = 0;
    end
    
    self.braking_power = self.braking_power + 200;
    
    self:wheel_brake(-1, self.braking_power);
    
    if self.left_turn or self.right_turn or self.emer then
		-- When turn/emergency lights are turned on, calculate blinking time (in this case between 0 and 1) for turn signal lights
		self.time = (self.time + dt) % 1;
        
        -- Checks, to get boolean values (following light_mask conditions need boolean true/false values)
        local left_turn_active = self.left_turn == 1;
        local right_turn_active = self.right_turn == 1; 
        local emer_active = self.emer == 1;
        -- For turn lights blinking effect
        local blink = self.time > 0.47 and true or false;
        
		-- Apply light mask for turn lights, depending of which action was handled (left turn lights, right turn lights or all turn lights (emergency)), which will then turn on and off, depending on the "blink" value (true or false)
		self:light_mask(light_entity.turn_left_mask, (blink and (left_turn_active or emer_active)));
		self:light_mask(light_entity.turn_right_mask, (blink and (right_turn_active or emer_active)));
	else
		-- To turn off the active turn lights
		self:light_mask(light_entity.turn_left_mask, false);
		self:light_mask(light_entity.turn_right_mask, false);
        self.time = 0;
    end
    
	self:animate_wheels();
end