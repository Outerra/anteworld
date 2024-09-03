-- Version 1 - Moving

implements("ot.vehicle_script")

-- Define global variables (global variables will be mainly used to store values, that don't change at runtime by given instance, like const values used for calculations, ID of wheels, sounds, emitters, bones etc.)
ENGINE_FORCE = 25000.0;
BRAKE_FORCE = 5000.0;
MAX_KMH = 200;
FORCE_LOSS = ENGINE_FORCE / (0.2*MAX_KMH + 1);

-- It is better to group related variables within objects, as it enhances modularity and creates structured and understandable code.
wheels = {
    FLwheel = -1, 
    FRwheel = -1, 
    RLwheel = -1, 
    RRwheel = -1
};
-- Note: for debug purpose, it's better to define these variables to -1

-- Invoked when engine has started or stopped (when you turn on/off the engine by pressing 'E'))
function engine_action(self)
    -- Toggle the "started" value every time, this function is called 
	self.started = not self.started;
    -- Write fading message on the screen
	self:fade(self.started and "Engine start" or "Engine stop");
    
    -- To not apply force on wheels, when the engine has stopped 
    if self.started == false then
        self:wheel_force(wheels.FLwheel, 0);
        self:wheel_force(wheels.FRwheel, 0);
    end
end

-- Called, when reverse button is pressed ('R')
-- Function can take "v" parameter through action handler (not used in this case)
function reverse_action(self, v)
	-- Switch the direction every time this function is called
    self.eng_dir = (self.eng_dir >= 0) and -1 or 1;
    self:fade(self.eng_dir > 0 and "Forward" or "Reverse");
end

-- Called, when hand brake button is pressed ('Space')
function hand_brake_action(self, v)
    self.hand_brake_input = self.hand_brake_input == 0 and 1 or 0;
end

-- Called, when power button is pressed ('W')
function power_action(self, v)
    self.power_input = v;
end

function ot.vehicle_script:init_chassis()	
    -- Define physical wheel parameters
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

    -- Bind model wheel joint/bone and add wheel parameters
	-- Function add_wheel returns ID of the wheel
	-- 1.parameter - wheel joint/bone to which you want to bind 
	-- 2.parameter - wheel physical parameters 
	-- Note: You can find model bones in "Scene editor"->"Entity properties"->"Skeleton".
    wheels.FLwheel = self:add_wheel('wheel_l0', wheel_params); --front left wheel (will have ID 0)
	wheels.FRwheel = self:add_wheel('wheel_r0', wheel_params); --front right wheel (will have ID 1)
	wheels.RLwheel = self:add_wheel('wheel_l1', wheel_params); --rear left wheel (will have ID 2)
	wheels.RRwheel = self:add_wheel('wheel_r1', wheel_params); --rear right wheel (will have ID 3)

    -- Add action handlers
    -- In this case call engine_action function, when engine ON/OFF button is presssed ('E') (these inputs are binded in vehicle.cfg file, more info in "Version 0 - Info" or Outerra wiki)
    self:register_event("vehicle/engine/on", engine_action);
	
    -- Call reverse_action function, when reverse button is presssed ('R') 
    self:register_event("vehicle/engine/reverse", reverse_action);
    
    -- Call the hand_brake_action, when hand brake button ('Space') is pressed 
    self:register_event("vehicle/controls/hand_brake", hand_brake_action);
    
    -- Call the power_action, when power button ('W') is pressed 
    self:register_axis("vehicle/controls/power", {minval = 0, center = 100}, power_action); 

    -- Note: the hand brake and power are handled through script, to avoid bug, where parking brake cannot be engaged right after the power input was released 
    -- (because by default, the centering is lower, and takes some time, until the value drops to 0)

    -- Return parameters (if not defined, they will be set to default parameters)
    return {
        mass = 1120.0,
		com = {x = 0.0, y = -0.2, z = 0.3},
		steering_params = {
			steering_ecf = 50,
		}
    }; 
end 
  
-- Define per-instance parameters in init_vehicle function
function ot.vehicle_script:init_vehicle()  

    -- Initialize variables for your instance (use "self." keyword for changes to affect only instance of sample car, you are currently in) 
	-- self.started creates "started" variable for all instances of sample car, but future changes will affect only the current instance of an object, in which the code is being executed
    self.started = false;
	self.eng_dir = 1;
    self.power_input = 0;
    self.braking_power = 0;
    self.hand_brake_input = 1;
    
    -- "self:" is also used to refer to already defined functions from vehicle interface, such as set_fps_camera_pos function (other members can be found on Outerra wiki) 

	-- Function "set_fps_camera_pos" set's the camera position, when FPS mode is active
    -- 1. parameter - model-space position from the pivot (when the joint id as 2. parameter is not specified, otherwise it works as offset, relative to the joint position)
    -- 2. parameter - bone/joint id (optional), to set fps camera position to joint position 
    -- 3. parameter - joint rotation mode (if the offset should be set based on joint orientation or not, where 0 - Enabled, 1 - Disabled )
	self:set_fps_camera_pos({x = -0.4, y = 0.16, z = 1.3});
    
    -- Example of using bone, to set the FPS camera position
    -- self:set_fps_camera_pos({x = 0, y = 0, z = 0}, self:get_joint_id("fps_cam_bone"), 1);
end

-- Invoked each frame to handle the internal state of the object
function ot.vehicle_script:update_frame(dt, engine, brake, steering, parking)

    -- Define local variable and use speed function to get current speed in m/s (to get km/h, multiply the value by 3.6) 
    -- Make it absolute, so that the value is always positive, even when moving in reverse
    local kmh = math.abs(self:speed() * 3.6);

    -- Calculate force, but only if the car has started
    if self.started == true then
        -- Calculate force, which will be applied on wheels to move the car
		local redux = self.eng_dir >= 0 and 0.2 or 0.6;
        -- You can use math library
		local eng_power = ENGINE_FORCE * self.power_input;
        -- Determine the force value based on whether the car should move forward or backward
		local force = (kmh >= 0) == (self.eng_dir >= 0) and (eng_power / (redux * kmh + 1)) or eng_power;
		-- Add wind resistance
        force = force - FORCE_LOSS;
        -- Make sure, that force can not be negative
		force = math.max(0.0, math.min(force, eng_power));
		
        -- Calculate force and direction, which will be used to add force to wheels
		force = force * self.eng_dir;
        
        -- Release the hand brake automatically, when accelerating while started
        if self.hand_brake_input ~= 0 and force > 0 then
            self.hand_brake_input = 0
        end
        
        -- Use wheel_force function to apply propelling force on wheel
        -- wheel_force is used to apply a propelling force on wheel and move the car.
        -- 1.parameter - wheel, you want to affect (takes the wheel ID, in this case, the car has front-wheel drive)
        -- 2.parameter - force, you want to exert on the wheel hub in forward/backward  direction (in Newtons)
        self:wheel_force(wheels.FLwheel, force);
        self:wheel_force(wheels.FRwheel, force);
        -- You can also use -1 as 1.parameter (instead wheel ID), this will affect all wheels
        -- Example: this.wheel_force(-1, force)
        
    end  
    
    -- Define steering sensitivity
    steering = steering * 0.3;

    -- Steer wheels by setting angle
	-- 1.parameter - wheel you want to affect (it takes the wheel ID number as argument)
	-- 2.parameter - angle in radians to steer the wheel
    self:steer(wheels.FLwheel, steering);	--front left wheel
	self:steer(wheels.FRwheel, steering);	--front right wheel
    -- You can also use -2 as 1.parameter (instead of wheel ID), this will affect first two wheels
	-- Example: this.steer(-2, steering)
    
    -- Set the braking value, which will be applied on wheels, based on the type of brake 
    if self.hand_brake_input ~= 0 then 
        -- Apply full braking force when the parking brake is engaged 
        self.braking_power = BRAKE_FORCE; 
    elseif brake ~= 0 then
        -- Apply proportional braking force when the regular brake is engaged
        self.braking_power = brake * BRAKE_FORCE;
    else 
        self.braking_power = 0;
    end
    
    -- Rolling friction
    self.braking_power = self.braking_power + 200;

    -- wheel_brake function is used to apply braking force on given wheels
	-- 1.parameter - wheel ID (in this case we want to affect all wheels, therefore -1)
	-- 2.parameter - braking force, you want to exert on the wheel hub (in Newtons)
    self:wheel_brake(-1, self.braking_power);
end


