-- Version 2 - Bone rotation and movement
-- This tutorial focused on rotating/moving bones using geomob.

implements("ot.vehicle_script")

-- Create  additional global variables
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
    RRwheel = -1
};

-- Create object containing bones/joints
bones = {
    steer_wheel = -1, 
    speed_gauge = -1, 
    accel_pedal = -1, 
    brake_pedal = -1, 
    driver_door = -1
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
end

function door_action(self, v)
    -- Define around which axis and in which direction the door will move
    local door_dir = {z = -1};
    -- Multiplied with 1.5 to fully open the door
    local door_angle = v * 1.5;
    -- rotate_joint_orig function is used to rotate joint by given angle and back to default position
    -- 1.parameter - bone/joint ID
    -- 2.parameter - rotation angle in radians
    -- 3.parameter - rotation axis vector (must be normalized) - axis around which the bone rotates (in this case around Z axis) and the direction of rotation (-1...1)
    self.geom:rotate_joint_orig(bones.driver_door, door_angle, door_dir);
    -- Note: action handlers use geomob (geom) functionality for current instance, which was initialized in init_vehicle.
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
    
    -- Use get_joint_id function to get joint/bone ID
	bones.steer_wheel = self:get_joint_id('steering_wheel');	-- Steering wheel
	bones.speed_gauge = self:get_joint_id('dial_speed');		-- Speed gauge 
	bones.accel_pedal = self:get_joint_id('pedal_accelerator');	-- Accelerator pedal
	bones.brake_pedal = self:get_joint_id('pedal_brake');		-- Brake pedal
	bones.driver_door = self:get_joint_id('door_l0');			-- Driver's door

    self:register_event("vehicle/engine/on", engine_action);
    self:register_event("vehicle/engine/reverse", reverse_action);

    -- Declare additional action handler to open/close driver's door (when 'O' is pressed)
	-- In this case, use register_axis function. Value handlers like this, let you change the opening range, speed and other parameters (more in "Version 0 - Info" or on Outerra Wiki) 
	self:register_axis("vehicle/controls/open", {minval = 0, maxval = 1, center = 0, vel = 0.6}, door_action); 
    
    -- Warning: "register_axis" events are called with value 0 when the script loads or reloads. 
    -- To prevent issues with functionality that toggles values, you can, for example, add a check to ignore these initial events by adding condition "if v == 0 then return end"

    return {
        mass = 1120.0,
		com = {x = 0.0, y = -0.2, z = 0.3},
		steering_params = {
			steering_ecf = 50,
		}
    };
end

function ot.vehicle_script:init_vehicle()  

    -- Get instance geometry interface, which will be used for current instance (to rotate bone, move bone, etc. )
    -- get_geomob function is used to get instance geometry interface
	-- parameter - ID of geometry object (default 0)
	self.geom = self:get_geomob(0);

    self.started = false;
	self.eng_dir = 1;
    self.braking_power = 0;
    
	self:set_fps_camera_pos({x = -0.4, y = 0.16, z = 1.3});
end

function ot.vehicle_script:update_frame(dt, engine, brake, steering, parking)

    -- Define additional local variables
	local brake_dir = {x = 1};
	-- Brake pedal rotation angle will depend on the brake value
	local brake_angle = brake * 0.4;	
	-- You can also use more than one axis
	local accel_dir = {y = (-engine * 0.02), z = (-engine * 0.02)}
    
    --Rotate brake pedal
	self.geom:rotate_joint_orig(bones.brake_pedal, brake_angle, brake_dir);
	
	-- move_joint_orig function is used to move joint to given position
	-- 1.parameter - joint you want to move
	-- 2.parameter - movement axis and direction
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
    
    
    -- Rotate speed gauge
	if kmh > SPEED_GAUGE_MIN then
        self.geom:rotate_joint_orig(bones.speed_gauge, (kmh - SPEED_GAUGE_MIN) * RAD_PER_KMH, {x = 0,y = 1,z = 0});    
    end
    
    steering = steering * 0.3;
    
    self:steer(wheels.FLwheel, steering);
    self:steer(wheels.FRwheel, steering);
    
    -- Rotate steering wheel 
	-- 1.prameter - bone/joint you want to rotate
	-- 2.parameter - rotation angle 
	-- 3.parameter (must be in {} brackets) - axis, around which you want to rotate (in this case you rotate around Z axis) and the direction of rotation (-1 or 1))
	self.geom:rotate_joint_orig(bones.steer_wheel, 10.5*steering, {z = 1});
    
    if parking ~= 0 then 
        self.braking_power = BRAKE_FORCE; 
    elseif brake ~= 0 then
        self.braking_power = brake * BRAKE_FORCE;
    else 
        self.braking_power = 0;
    end
    
    self.braking_power = self.braking_power + 200;
    
    self:wheel_brake(-1, self.braking_power);
    
    -- Function used for "animating" wheels
	self:animate_wheels();
	-- This method simplifies the animation of wheels for basic cases, without needing to animate the model via the geomob
end


