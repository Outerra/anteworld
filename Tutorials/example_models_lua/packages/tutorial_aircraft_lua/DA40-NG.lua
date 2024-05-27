-- Welcome to the second tutorial on Lua programming in Outerra.
-- We recommend reviewing previous tutorial (tutorial_car_lua) before proceeding, as well as the corresponding tutorial on the Outerra/anteworld GitHub page ( https://github.com/Outerra/anteworld/wiki/Tutorial-2-%E2%80%90-aircraft-(Lua) ), as it contains detailed information.

-- It is needed to implement interface, to be able to use interface functionality (in this case ot.aircraft_script), such as init_chassis, initialize, update_frame etc.
implements("ot.aircraft_script")

-- Define global variables
-- Used for joint rotation
PI = 3.14159265358979323846;

-- Create objects and initialize it's properties with default values (-1 for debug purpose)
-- Bones
bones = {
    propeller = -1,
    wheel_front = -1,
    wheel_right = -1,
    wheel_left = -1,
    elevator_right = -1,
    elevator_left = -1,
    rudder = -1,
    aileron_right = -1,
    aileron_left = -1,
    rudder_pedal_left = -1,
    rudder_pedal_right = -1,
    brake_pedal_left = -1,
    brake_pedal_right = -1,
    throttle_handle = -1,
    flap_left = -1,
    flap_right = -1,  
};

-- Meshes
meshes = {
    prop_blur = -1,
	blade_one = -1,
	blade_two = -1,
	blade_three = -1,
};

-- Sounds 
sounds = {
    rumble = -1,
    eng_int = -1,
    eng_ext = -1,
    prop_int = -1,
    prop_ext = -1,
};

-- Sound sources
sources = {
    rumble_int = -1,
    rumble_ext = -1,
    eng_int = -1,
    eng_ext = -1,
    prop_int = -1,
    prop_ext = -1,
};

-- Used to clamp values
function clamp(val, min, max) 
	if val < min then return min; 
	elseif val > max then return max; 
	else return val;
    end
end

-- Gets called when engine starts/stops
function engine(self)
    -- toggle the value every time this function is called 
    self.started = not self.started;
    
    if self.started == true then
        -- For JSBSim aircraft to start, the starter and magneto needs to be activated
        -- Turn on engine (0 - turn off, 1 - turn on)
        self.jsbsim:set_property('propulsion/starter_cmd', 1);
        -- debug tip: when the magneto isn't activated, it results in decreased piston engine power, with the rpm not exceeding 1000
        self.jsbsim:set_property('propulsion/magneto_cmd', 1);
    else
        self.jsbsim:set_property('propulsion/starter_cmd', 0);
        self.jsbsim:set_property('propulsion/magneto_cmd', 0);
    end
end
-- "jsbsim:set_property(property, value)" is used, to set property (belonging to JSBSim interface) to given value. 
-- For information on JSBSim, visit the JSBSim & Aeromatic page (https://github.com/Outerra/anteworld/wiki/JSBSim-&-Aeromatic).
-- For a list of usable JSBSim properties, refer to the JSBSim properties page (https://github.com/Outerra/anteworld/wiki/JSBSim-properties).

function landing_lights_action(self, v)  
    self:light_mask( 0x3, v > 0);
end

function nav_lights_action(self, v) 
	self:light_mask( 0x3, v > 0, self.nav_light_offset)
end

function aileron_action(self, v) 
    self.jsbsim:set_property('fcs/aileron-cmd-norm', v);
end

function elevator_action(self, v)
    self.jsbsim:set_property('fcs/elevator-cmd-norm', -v);
end

function brake_action(self, v)
    self.braking = v;
    self.jsbsim:set_property('fcs/center-brake-cmd-norm', v);
    self.jsbsim:set_property('fcs/left-brake-cmd-norm', v);
    self.jsbsim:set_property('fcs/right-brake-cmd-norm', v);
end

-- Invoked once, to define the chassis for all instances
function ot.aircraft_script:init_chassis()
	-- Get joins/bones
	bones.propeller = self:get_joint_id('propel');
	bones.wheel_right = self:get_joint_id('right_wheel');
	bones.wheel_front = self:get_joint_id('front_wheel');
	bones.wheel_left = self:get_joint_id('left_wheel');
	bones.elevator_right = self:get_joint_id('elevator_right'); 
	bones.elevator_left = self:get_joint_id('elevator_left');
	bones.rudder = self:get_joint_id('rudder');
	bones.aileron_right = self:get_joint_id('aileron_right'); 
	bones.aileron_left = self:get_joint_id('aileron_left');
    bones.flap_left = self:get_joint_id('flap_left');
    bones.flap_right = self:get_joint_id('flap_right');
	bones.rudder_pedal_left = self:get_joint_id('rudder_pedal_left');
	bones.rudder_pedal_right = self:get_joint_id('rudder_pedal_right');
	bones.brake_pedal_left = self:get_joint_id('brake_pedal_left');
	bones.brake_pedal_right = self:get_joint_id('brake_pedal_right');
	bones.throttle_handle = self:get_joint_id('throttle_lever');
	
	-- Get mesh, for that use function get_mesh_id
	-- 1.parameter - mesh name
	meshes.prop_blur = self:get_mesh_id('propel_blur');
	meshes.blade_one = self:get_mesh_id('main_blade_01#0@0');
	meshes.blade_two = self:get_mesh_id('main_blade_02#0@0');
	meshes.blade_three = self:get_mesh_id('main_blade_03#0@0');

	-- Add lights
	-- Landing lights
	local light_params = {color = {x = 1, y = 1, z = 1}, angle = 100, size = 0.1, edge = 0.25, intensity = 5, fadeout = 0.05};
	self:add_spot_light({x = 4.5, y = 1.08, z = 0.98}, {x = -0.1, y = 1, z = 0.3}, light_params);
	self:add_spot_light({x = -4.5, y = 1.08, z = 0.98}, {x = 0.1, y = 1, z = 0.3}, light_params);

	-- Navigation lights
	light_params = {color = {x = 0, y = 1, z = 0}, size = 0.035, edge = 1, range = 0.0001, intensity = 20, fadeout = 0.1};
	self.nav_light_offset =
	--Right green navigation light
	self:add_point_light({x = 5.08, y = 0.18, z = 1.33}, light_params);
	-- Left red navigation light
	light_params.color = {x = 1, y = 0, z = 0};
	self:add_point_light({x = -5.08, y = 0.18, z = 1.33}, light_params);
	

	-- Load sounds
  	-- engine rumble - in this case it's used for the outside sounds and also the inside
    sounds.rumble = self:load_sound("Sounds/engine/engn1.ogg");
	
	-- Interior sounds - will be heard from inside the plane
    -- engine
	sounds.eng_ext = self:load_sound("Sounds/engine/engn1_out.ogg");
    -- propeller
	sounds.prop_ext = self:load_sound("Sounds/engine/prop1_out.ogg");

	-- Exterior sounds - will be heard from outside the plane 
    -- engine
	sounds.eng_int = self:load_sound("Sounds/engine/engn1_inn.ogg"); 
    -- propeller
	sounds.prop_int = self:load_sound("Sounds/engine/prop1_out.ogg");
	
	-- Add sound emitters
    -- Interior emitters
	-- For interior rumbling sound
    sources.rumble_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
    -- For interior engine sounds
	sources.eng_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
	-- For interior propeller sounds
	sources.prop_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
    
    -- Exterior emitters
    -- For exterior rumbling sound
    sources.rumble_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
	-- For exterior engine sounds 
	sources.eng_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
	-- For exterior propeller sounds 
	sources.prop_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
    
    -- Note: it is possible, to use one emitter for interior and exterior, like in previous tutorial
    
    -- Add action handlers
	self:register_axis("air/lights/landing_lights", {minval = 0, maxval = 1, vel = 10, center = 0 }, landing_lights_action );
    self:register_axis("air/lights/nav_lights", {minval = 0, maxval = 1, vel = 10, center = 0 }, nav_lights_action );
   	
    -- Note: by default, JSBSim actions are handled internally, allowing gameplay without direct user intervention. 
    -- However, sometimes it is needed to customize or fine-tune aircraft behavior, therefore in this case, the following handlers are handled in script.
    self:register_event("air/engines/on", engine);
    self:register_axis("air/controls/aileron", { minval = -1, maxval = 1, center = 0.5, vel = 0.5, positions = 0 }, aileron_action );
	self:register_axis("air/controls/elevator", { minval = -1, maxval = 1, center = 0.5, vel = 0.5, positions = 0 }, elevator_action );
    self:register_axis("air/controls/brake", { minval = 0}, brake_action );
  
	return {
		mass = 1310,
		com = {x = 0.0, y = 0.0, z = 0.2},
	};
end

--Function initialize() is used to define per-instance parameters (similarly how vehicles use init_vehicle())
function ot.aircraft_script:initialize()
	-- Get geomob interface
	self.geom = self:get_geomob(0);
	
	-- Get JSBSim interface, to be able to work with JSBSim properties
	self.jsbsim = self:jsb();
	
	-- Get sound interface
	self.snd = self:sound();
	
	-- Set first person camera position 
	self:set_fps_camera_pos({x = 0, y = 1, z = 1.4});
    
    self.started = false;
    self.braking = 0;
	
	-- Set initial value for JSBSim properties
	-- Turn off engine (commands are normalized)
	self.jsbsim:set_property('propulsion/starter_cmd', 0);
    -- Turn off magneto
	self.jsbsim:set_property('propulsion/magneto_cmd', 0);
	-- Set initial throttle value to 0 
	self.jsbsim:set_property('fcs/throttle-cmd-norm[0]', 0);
    -- Set initial throttle value to 0 
    self.jsbsim:set_property('fcs/mixture-cmd-norm', 0);
	-- Set initial right brake value to 0 
	self.jsbsim:set_property('fcs/right-brake-cmd-norm', 0);
	-- Set initial left brake value to 0 
	self.jsbsim:set_property('fcs/left-brake-cmd-norm', 0);
    
    -- Set default pitch value for sound emitters
    self.snd:set_pitch(sources.rumble_ext, 1);
    self.snd:set_pitch(sources.rumble_int, 1);
    self.snd:set_pitch(sources.eng_ext, 1);
    self.snd:set_pitch(sources.eng_int, 1);
    self.snd:set_pitch(sources.prop_ext, 1);
    self.snd:set_pitch(sources.prop_int, 1);
    
    -- Set default gain value for sound emitters
    self.snd:set_gain(sources.rumble_ext, 1);
    self.snd:set_gain(sources.rumble_int, 1);
    self.snd:set_gain(sources.eng_ext, 1);
    self.snd:set_gain(sources.eng_int, 1);
    self.snd:set_gain(sources.prop_ext, 1);
    self.snd:set_gain(sources.prop_int, 1);
end

-- Invoked each frame to handle the internal state of the object
function ot.aircraft_script:update_frame(dt)
    -- "jsbsim:get_property(property)" is used, to get JSBSim property value 
	-- Get engine rpm from JSBSim
	local propeller_rpm = self.jsbsim:get_property('propulsion/engine[0]/propeller-rpm');
	-- Get wheel speed from JSBSim
	local wheel_speed = self.jsbsim:get_property('gear/unit[0]/wheel-speed-fps');
	-- Get elevator rotation in radians
	local elev_pos_rad = self.jsbsim:get_property('fcs/elevator-pos-rad');
	-- rotate_joint - incremental, bone rotates by given angle every time, this function is called (given angle is added to joint current orientation)
	
    self.geom:rotate_joint(bones.propeller, dt * (2 * PI) * (propeller_rpm), {y = 1});
	self.geom:rotate_joint(bones.wheel_front, dt * PI * (wheel_speed / 5), {x = -1});
	self.geom:rotate_joint(bones.wheel_right, dt * PI * (wheel_speed / 5), {x = -1});
	self.geom:rotate_joint(bones.wheel_left, dt * PI * (wheel_speed / 5), {x = -1});
    
    -- rotate_joint_orig - rotate to given angle (from default orientation)
    self.geom:rotate_joint_orig(bones.elevator_right, elev_pos_rad, {x = 1});
	self.geom:rotate_joint_orig(bones.elevator_left, elev_pos_rad, {x = 1});
	self.geom:rotate_joint_orig(bones.rudder, self.jsbsim:get_property('fcs/rudder-pos-rad'), {z = -1});
	self.geom:rotate_joint_orig(bones.aileron_right, self.jsbsim:get_property('fcs/right-aileron-pos-rad'), {x = -1});
	self.geom:rotate_joint_orig(bones.aileron_left, self.jsbsim:get_property('fcs/left-aileron-pos-rad'), {x = 1});
	self.geom:rotate_joint_orig(bones.flap_left, self.jsbsim:get_property('fcs/flap-pos-rad'), {x = 1,y = 0,z = 0});
    self.geom:rotate_joint_orig(bones.flap_right, self.jsbsim:get_property('fcs/flap-pos-rad'), {x = 1,y = 0,z = 0});
    self.geom:rotate_joint_orig(bones.brake_pedal_left, self.jsbsim:get_property('fcs/left-brake-cmd-norm') * 0.7, {x = -1});
	self.geom:rotate_joint_orig(bones.brake_pedal_right, self.jsbsim:get_property('fcs/right-brake-cmd-norm') * 0.7, {x = -1});

	-- Rudder has 2 pedals for turning, therefore depending on the rudder position value from JSBSim, the left or right pedal will move
	if self.jsbsim:get_property('fcs/rudder-pos-rad') > 0 then
		self.geom:move_joint_orig(bones.rudder_pedal_left, {y = (self.jsbsim:get_property('fcs/rudder-pos-rad')/5)} );
	elseif self.jsbsim:get_property('fcs/rudder-pos-rad') < 0 then
		self.geom:move_joint_orig(bones.rudder_pedal_right, {y = -(self.jsbsim:get_property('fcs/rudder-pos-rad')/5)} );
    end
	
	-- Move throttle handle, depending on the throttle command value 
	self.geom:move_joint_orig(bones.throttle_handle, {y = (self.jsbsim:get_property('fcs/throttle-cmd-norm[0]') / 7)});
	
	-- Note: in this case the pitch/roll handle rotates without the need of scripting, because it's binded with inputs in the model
	
	-- Use set_mesh_visible_id() function, to set propeller blur mesh visible, when condition is true and invisible otherwise
	-- 1.param - mesh ID
	-- 2.param - condition
	self.geom:set_mesh_visible_id(meshes.prop_blur, propeller_rpm > 200.0);
	self.geom:set_mesh_visible_id(meshes.blade_one, propeller_rpm < 300.0);
	self.geom:set_mesh_visible_id(meshes.blade_two, propeller_rpm < 300.0);
	self.geom:set_mesh_visible_id(meshes.blade_three, propeller_rpm < 300.0);
	
    -- When engine runs, control sounds, based on the camera position & sound effects, based on engine RPM
	if propeller_rpm > 0 then
		-- Interior
		if self:get_camera_mode() == 0 then 	
			-- Turn off the exterior emitters, when player camera is inside the plane
			self.snd:stop(sources.eng_ext); 
            self.snd:stop(sources.prop_ext);
            self.snd:stop(sources.rumble_ext);
            
			-- Calculate and set pitch for interior engine emitter (clamped between 1 and 2)
			self.snd:set_pitch(sources.eng_int, clamp(1 + propeller_rpm/4000, 1, 2));
			-- Calculate and set gain for interior engine emitter (clamped between 0 and 0.5)
			self.snd:set_gain(sources.eng_int, clamp(propeller_rpm/5000, 0, 0.5));
			
            -- If there is no sound already playing on given emitter, play loop
			if self.snd:is_playing(sources.eng_int) == false then 
				-- Play interior engine sound in a loop
				self.snd:play_loop(sources.eng_int, sounds.eng_int);
			end
            
			-- Set gain for interior propeller emitter
			self.snd:set_gain(sources.prop_int, clamp(propeller_rpm/7000, 0, 0.5));
			
			if self.snd:is_playing(sources.prop_int) == false then 
				-- Play interior propeller sound in a loop
				self.snd:play_loop(sources.prop_int, sounds.prop_int);
            end
			
			-- Set gain for interior rumble emitter
			self.snd:set_gain (sources.rumble_int, clamp(propeller_rpm/6000, 0, 0.5));
			
            if self.snd:is_playing(sources.rumble_int) == false then
				-- Play rumble sound in a loop on interior emitter
				self.snd:play_loop(sources.rumble_int, sounds.rumble);
            end
        -- Exterior
		else 
			-- Turn off the interior emitters, when camera is outside the plane
			self.snd:stop(sources.eng_int);
			self.snd:stop(sources.prop_int);
			self.snd:stop(sources.rumble_int);
            
			-- Set pitch for exterior engine emitter
			self.snd:set_pitch(sources.eng_ext, clamp(1 + propeller_rpm/1000, 1, 3));
			-- Set gain for exterior engine emitter
			self.snd:set_gain (sources.eng_ext, clamp(propeller_rpm/450, 0.05, 2));
			
            if self.snd:is_playing(sources.eng_ext) == false then 
				-- Play exterior engine sound in a loop
				self.snd:play_loop(sources.eng_ext, sounds.eng_ext);
            end
            
            -- Set gain for exterior propeller emitter
			self.snd:set_gain (sources.prop_ext, clamp(propeller_rpm/900, 0, 2));
            
            if self.snd:is_playing(sources.prop_ext) == false then 
				-- Play exterior propeller sound in a loop
				self.snd:play_loop(sources.prop_ext, sounds.prop_ext);
			end
			
			-- Set gain for exterior rumble emitter
			self.snd:set_gain (sources.rumble_ext, clamp(propeller_rpm/1200, 0, 2));
			
			if self.snd:is_playing(sources.rumble_ext) == false then
				-- Play rumble sound in a loop on exterior emitter
				self.snd:play_loop(sources.rumble_ext, sounds.rumble);
			end
		end
	else 
		-- Stop all sounds playing on given emitters, when engine is turned off and propeller stops rotating
		self.snd:stop(sources.eng_ext);
		self.snd:stop(sources.eng_int);
        self.snd:stop(sources.prop_ext);
		self.snd:stop(sources.prop_int);
		self.snd:stop(sources.rumble_ext);
		self.snd:stop(sources.rumble_int);
    end
    
    -- Parking brake when in idle state 
    if self.started == false and propeller_rpm < 5 then
        self.jsbsim:set_property('fcs/center-brake-cmd-norm', 1);
        self.jsbsim:set_property('fcs/left-brake-cmd-norm', 1);
        self.jsbsim:set_property('fcs/right-brake-cmd-norm', 1);
    elseif self.braking < 0.1 then
        self.jsbsim:set_property('fcs/center-brake-cmd-norm', 0);
        self.jsbsim:set_property('fcs/left-brake-cmd-norm', 0);
        self.jsbsim:set_property('fcs/right-brake-cmd-norm', 0);
    end
end

