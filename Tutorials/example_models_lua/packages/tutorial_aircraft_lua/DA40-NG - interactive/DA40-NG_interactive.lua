-- This is short tutorial on scripting functionality for interactive controls.

-- It is build on the original DA40-NG.lua aircraft tutorial, we recommed to review that one first.
-- You can find tutorials on interactive controls on page https://github.com/Outerra/anteworld/wiki#interactive

implements("ot.aircraft_script")

PI = 3.14159265358979323846;

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

meshes = {
    prop_blur = -1,
	blade_one = -1,
	blade_two = -1,
	blade_three = -1,
};

sounds = {
    rumble = -1,
    eng_int = -1,
    eng_ext = -1,
    prop_int = -1,
    prop_ext = -1,
};

sources = {
    rumble_int = -1,
    rumble_ext = -1,
    eng_int = -1,
    eng_ext = -1,
    prop_int = -1,
    prop_ext = -1,
};

-- Declare action variables
actions = {
    act_throttle_lever = -1,
    act_rudder_pedal_L = -1,
    act_rudder_pedal_R = -1,
    act_brake_pedal_L = -1,
    act_brake_pedal_R = -1,
};


function clamp(val, min, max) 
	if val < min then return min; 
	elseif val > max then return max; 
	else return val;
    end
end

function engine(self)
    self.started = not self.started;
    
    if self.started == true then
        self.jsbsim:set_property('propulsion/starter_cmd', 1);
        self.jsbsim:set_property('propulsion/magneto_cmd', 1);
    else
        self.jsbsim:set_property('propulsion/starter_cmd', 0);
        self.jsbsim:set_property('propulsion/magneto_cmd', 0);
    end
end

function landing_lights_action(self, v)  
    self:light_mask( 0x3, v > 0);
end

function nav_lights_action(self, v) 
	self:light_mask( 0x3, v > 0, self.nav_light_offset)
end


-- Modify existing action handling functions

-- Aileron action (keyboard input 'A' and 'D'), and also knob ( controlled in interactive mode with "roll_lever")
function aileron_action(self, v) 
    self.jsbsim:set_property('fcs/aileron-cmd-norm', v);
    
    -- Bone animations were moved here from update_frame to avoid bugs (e.g. when the knob is grabbed in interactive mode, it is in conflict with the bone position setting/animating through script)
    self.geom:rotate_joint_orig(bones.aileron_right, self.jsbsim:get_property('fcs/right-aileron-pos-rad'), {x = -1});
    self.geom:rotate_joint_orig(bones.aileron_left, self.jsbsim:get_property('fcs/left-aileron-pos-rad'), {x = 1});
end

-- Elevator action (keyboard input 'W' and 'S')
function elevator_action(self, v)
    self.jsbsim:set_property('fcs/elevator-cmd-norm', -v);
    
    -- Bone animations were moved here from update_frame to avoid bugs (e.g. when the knob is grabbed in interactive mode, it is in conflict with the bone position setting/animating through script)
    self.geom:rotate_joint_orig(bones.elevator_right, self.jsbsim:get_property('fcs/elevator-pos-rad'), {x = 1});
    self.geom:rotate_joint_orig(bones.elevator_left, self.jsbsim:get_property('fcs/elevator-pos-rad'), {x = 1});
    
end

-- Brake action (keyboard input 'B')
function brake_action(self, v)
    -- Previous code for setting properties was moved to the brake pedals knobs, and those knobs are set instead (they then set the properties)
    -- Set target values of left and right brake pedal actions
    self:set_action_value(actions.act_brake_pedal_L, v, false)
    self:set_action_value(actions.act_brake_pedal_R, v, false)
end

-- Add functions for handling actions

-- Throttle action (keyboard inputs 'PgUp' and 'PgDn')
function throttle_action(self, v)
    -- function "set_action_value" is used for setting the target value of the action handler and executing it's code
    -- 1.param - action id (returned from functions register_handler, register_axis, etc., when defining them )
    -- 2.param - target value, to which the action should go (the value is not set instantly, the value of the action handler changes based on it's own velocity and acceleration, until it reaches the target value )
    -- 3.param - used in set_action_value function, to determine, if the value should be set (if true, centering of the action is disabled)

    self:set_action_value(actions.act_throttle_lever, v, true);
    
    -- another function to set action handler's target value is "set_instant_action_value", but it is mainly used for setting the target value of the action handler without executing it's code (when "notify" is false)
    -- 1.param - action id (returned from functions register_handler, register_axis, etc., when defining them )
    -- 2.param - target value, to which the action should go (the value is not set instantly, the value of the action handler changes based on it's own velocity and acceleration, until it reaches the target value )
    -- 3.param - used in **set_instant_action_value** to determine, if the action should be invoked or not. 
    --         - if true, the code in the action handler is executed (same as using set_action_value)
    --         - if false, the action handler's value is changed, without executing it's code (useful when working with interactive knobs, because changing the knob's action value means, that the knob (elements like switch, lever, etc.) in the model will move/rotate)
end

-- Rudder action (keyboard inputs 'Z' and 'X')
function rudder_action(self, v)
    -- Based on, if the value is moving in positive or negative direction (as the rudder values are moving between -1 and 1), set the value of corresponding action handler.
    -- If the value is moving in positive direction ('X' was pressed), set value "v" (this handler's value) to the action handler "act_rudder_pedal_R" (action will also be called)
    -- If the value is moving in negative direction ('Z' was pressed), set value "-v" (this handler's value, but negative) to the action handler "act_rudder_pedal_R" (action will also be called)
    self:set_action_value(v > 0 and actions.act_rudder_pedal_R or actions.act_rudder_pedal_L, v > 0 and v or -v, false)
    
    -- Note: in this case, the "act_rudder_pedal_R" needs to be set to positive value and "act_rudder_pedal_L" to negative value, so that they will be animated correctly (interactive model elements are animated, when their value changes)
end

-- Knobs
 
-- Following knobs are called either when the knob is controlled in interactive mode (for example when throttle lever is grabbed and moved), 
-- or when their value is changed from script, using function "set_action_value" of "set_instant_action_value"
    

-- Throttle lever interactive knob 
function throttle_knob(self, v)
    -- Set throttle jsbsim property
    self.jsbsim:set_property('fcs/throttle-cmd-norm', v);
    -- Write fading message on the screen, showing the throttle value
    self:fade("Throttle: " .. math.floor(v * 100) .. "%")
end

-- Rudder left pedal interactive knob 
function rudder_knob_L(self, v)
    -- Set rudder jsbsim property 
    self.jsbsim:set_property('fcs/rudder-cmd-norm', v);
end

-- Rudder right pedal interactive knob 
function rudder_knob_R(self, v)
    -- Set rudder jsbsim property 
    self.jsbsim:set_property('fcs/rudder-cmd-norm', -v);
end

-- Brake left pedal interactive knob 
function brake_knob_L(self, v)
    -- Store braking value
    self.braking = v;
    -- Set left brake jsbsim property 
    self.jsbsim:set_property('fcs/left-brake-cmd-norm', v);
end

-- Brake right pedal interactive knob 
function brake_knob_R(self, v)
    -- Store braking value
    self.braking = v;
    -- Set right brake jsbsim property 
    self.jsbsim:set_property('fcs/right-brake-cmd-norm', v);
end


function ot.aircraft_script:init_chassis()
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
	
	meshes.prop_blur = self:get_mesh_id('propel_blur');
	meshes.blade_one = self:get_mesh_id('main_blade_01#0@0');
	meshes.blade_two = self:get_mesh_id('main_blade_02#0@0');
	meshes.blade_three = self:get_mesh_id('main_blade_03#0@0');

	local light_params = {color = {x = 1, y = 1, z = 1}, angle = 100, size = 0.1, edge = 0.25, intensity = 5, fadeout = 0.05};
	self:add_spot_light({x = 4.5, y = 1.08, z = 0.98}, {x = -0.1, y = 1, z = 0.3}, light_params);
	self:add_spot_light({x = -4.5, y = 1.08, z = 0.98}, {x = 0.1, y = 1, z = 0.3}, light_params);


	light_params = {color = {x = 0, y = 1, z = 0}, size = 0.035, edge = 1, range = 0.0001, intensity = 20, fadeout = 0.1};
	self.nav_light_offset =
	self:add_point_light({x = 5.08, y = 0.18, z = 1.33}, light_params);
	light_params.color = {x = 1, y = 0, z = 0};
	self:add_point_light({x = -5.08, y = 0.18, z = 1.33}, light_params);
	

    sounds.rumble = self:load_sound("Sounds/engine/engn1.ogg");
	sounds.eng_ext = self:load_sound("Sounds/engine/engn1_out.ogg");
	sounds.prop_ext = self:load_sound("Sounds/engine/prop1_out.ogg");

	sounds.eng_int = self:load_sound("Sounds/engine/engn1_inn.ogg"); 
	sounds.prop_int = self:load_sound("Sounds/engine/prop1_out.ogg");
	
    sources.rumble_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
	sources.eng_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
	sources.prop_int = self:add_sound_emitter_id(bones.propeller, -1, 0.5);
    
    sources.rumble_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
	sources.eng_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
	sources.prop_ext = self:add_sound_emitter_id(bones.propeller, 1, 3);
    
	self:register_axis("air/lights/landing_lights", {minval = 0, maxval = 1, vel = 10, center = 0 }, landing_lights_action );
    self:register_axis("air/lights/nav_lights", {minval = 0, maxval = 1, vel = 10, center = 0 }, nav_lights_action );
   	  
    self:register_event("air/engines/on", engine);
	self:register_axis("air/controls/elevator", { minval = -1, maxval = 1, center = 0.5, vel = 0.5, positions = 0 }, elevator_action );
    self:register_axis("air/controls/brake", { minval = 0}, brake_action );
    self:register_axis("air/controls/aileron", { minval = -1, maxval = 1, center = 0.5, vel = 0.5, positions = 0 }, aileron_action );
	
    -- Register handlers
    
    -- Actions 
    
	self:register_axis("air/engines/throttle", {center = 0.0}, throttle_action );
    self:register_axis("air/controls/rudder", {}, rudder_action );
    
    -- Knobs

    -- Following knobs are called either when the knob is controlled in interactive mode (for example when throttle lever is grabbed and moved), 
    -- or when their value is changed from script, using function "set_action_value" of "set_instant_action_value"
    -- Note: when knob handler's value is changed through script, the knob (interactive element in model) moves to the set position. This way the knobs can be "animated" without using animating functions from geomob... 
    
    -- Note: in this case the pitch/roll handle rotates without the need of scripting (animating through geomob), because it's binded with actions in the model. 
    -- Similarly, the throttle lever, pedals, and switches in the model will still animate even if not defined in the script, but they won't perform the intended functionality. 
    
    -- If knob does have defined action name, then that name is used as the action name.
    -- Example: this model has interactive knob called "roll_lever", which has action name defined as "air/controls/aileron" (defined action in air.cfg IOMap configuration) assigned to it, therefore it is referenced to as "air/controls/aileron".
    -- If knob does not have defined action name, automatic name is assigned, so that it can be refered by script.
    -- Format of automatic generated name is "knob_action_[bone_name]", for example for bone attribute with name (knob name) "pedal_left" it is "knob_action_rudder_pedal_left".
    
    -- Throttle lever interactive knob 
    -- In this case, the knob has action name "air/engine/throttle", even though that action does not exist in air.cfg IOMap configuration (has "engine" instead of "engines" in name)
    -- Therefore handler for throttle action ("air/engines/throttle") has been previously added, so that it can react on changes in the throttle value. (for example when controlled through keyboard input).
    actions.act_throttle_lever = self:register_axis("air/engine/throttle", {}, throttle_knob );
    
    -- Rudder left pedal interactive knob 
    actions.act_rudder_pedal_L = self:register_handler("knob_action_rudder_pedal_left", rudder_knob_L );
    -- Rudder right pedal interactive knob 
    actions.act_rudder_pedal_R = self:register_handler("knob_action_rudder_pedal_right", rudder_knob_R );
    -- Brake left pedal interactive knob 
    actions.act_brake_pedal_L = self:register_handler("knob_action_brake_pedal_left", brake_knob_L );
    -- Brake right pedal interactive knob 
    actions.act_brake_pedal_R = self:register_handler("knob_action_brake_pedal_right", brake_knob_R );
  
    -- List of existing interactive knobs on the model can be found in Outerra -> Plugins -> Entity properties -> Bone attributes (model has to be selected).
  
	return {
		mass = 1310,
		com = {x = 0.0, y = 0.0, z = 0.2},
	};
end

function ot.aircraft_script:initialize()
	self.geom = self:get_geomob(0);
	self.jsbsim = self:jsb();
	self.snd = self:sound();
	
	self:set_fps_camera_pos({x = 0, y = 1, z = 1.4});
    
    self.started = false;
    self.braking = 0;
	
	self.jsbsim:set_property('propulsion/starter_cmd', 0);
	self.jsbsim:set_property('propulsion/magneto_cmd', 0);
	self.jsbsim:set_property('fcs/throttle-cmd-norm[0]', 0);
    self.jsbsim:set_property('fcs/mixture-cmd-norm', 0);
	self.jsbsim:set_property('fcs/right-brake-cmd-norm', 0);
	self.jsbsim:set_property('fcs/left-brake-cmd-norm', 0);
    
    self.snd:set_pitch(sources.rumble_ext, 1);
    self.snd:set_pitch(sources.rumble_int, 1);
    self.snd:set_pitch(sources.eng_ext, 1);
    self.snd:set_pitch(sources.eng_int, 1);
    self.snd:set_pitch(sources.prop_ext, 1);
    self.snd:set_pitch(sources.prop_int, 1);
    
    self.snd:set_gain(sources.rumble_ext, 1);
    self.snd:set_gain(sources.rumble_int, 1);
    self.snd:set_gain(sources.eng_ext, 1);
    self.snd:set_gain(sources.eng_int, 1);
    self.snd:set_gain(sources.prop_ext, 1);
    self.snd:set_gain(sources.prop_int, 1);
end

function ot.aircraft_script:update_frame(dt)
	local propeller_rpm = self.jsbsim:get_property('propulsion/engine[0]/propeller-rpm');
	local wheel_speed = self.jsbsim:get_property('gear/unit[0]/wheel-speed-fps');
    
    -- Note: elevator, ailerons, rudder pedals, throttle handle and brake pedals animations were either removed (if there is no need to animate other parts, than the knobs, e.g. throttle handle and brake pedals), 
    -- or moved into handlers, to avoid bugs (e.g. when the knob is grabbed in interactive mode, it is in conflict with the bone position setting/animating through script)
    self.geom:rotate_joint(bones.propeller, dt * (2 * PI) * (propeller_rpm), {y = 1});
	self.geom:rotate_joint(bones.wheel_front, dt * PI * (wheel_speed / 5), {x = -1});
	self.geom:rotate_joint(bones.wheel_right, dt * PI * (wheel_speed / 5), {x = -1});
	self.geom:rotate_joint(bones.wheel_left, dt * PI * (wheel_speed / 5), {x = -1});
	self.geom:rotate_joint_orig(bones.rudder, self.jsbsim:get_property('fcs/rudder-pos-rad'), {z = -1});
	self.geom:rotate_joint_orig(bones.flap_left, self.jsbsim:get_property('fcs/flap-pos-rad'), {x = 1,y = 0,z = 0});
    self.geom:rotate_joint_orig(bones.flap_right, self.jsbsim:get_property('fcs/flap-pos-rad'), {x = 1,y = 0,z = 0});

	self.geom:set_mesh_visible_id(meshes.prop_blur, propeller_rpm > 200.0);
	self.geom:set_mesh_visible_id(meshes.blade_one, propeller_rpm < 300.0);
	self.geom:set_mesh_visible_id(meshes.blade_two, propeller_rpm < 300.0);
	self.geom:set_mesh_visible_id(meshes.blade_three, propeller_rpm < 300.0);
	
	if propeller_rpm > 0 then
		if self:get_camera_mode() == 0 then 	
			self.snd:stop(sources.eng_ext); 
            self.snd:stop(sources.prop_ext);
            self.snd:stop(sources.rumble_ext);
            
			self.snd:set_pitch(sources.eng_int, clamp(1 + propeller_rpm/4000, 1, 2));
			self.snd:set_gain(sources.eng_int, clamp(propeller_rpm/5000, 0, 0.5));
			
			if self.snd:is_playing(sources.eng_int) == false then 
				self.snd:play_loop(sources.eng_int, sounds.eng_int);
			end
            
			self.snd:set_gain(sources.prop_int, clamp(propeller_rpm/7000, 0, 0.5));
			
			if self.snd:is_playing(sources.prop_int) == false then 
				self.snd:play_loop(sources.prop_int, sounds.prop_int);
            end
			
			self.snd:set_gain (sources.rumble_int, clamp(propeller_rpm/6000, 0, 0.5));
			
            if self.snd:is_playing(sources.rumble_int) == false then
				self.snd:play_loop(sources.rumble_int, sounds.rumble);
            end
		else 
			self.snd:stop(sources.eng_int);
			self.snd:stop(sources.prop_int);
			self.snd:stop(sources.rumble_int);
            
			self.snd:set_pitch(sources.eng_ext, clamp(1 + propeller_rpm/1000, 1, 3));
			self.snd:set_gain (sources.eng_ext, clamp(propeller_rpm/450, 0.05, 2));
			
            if self.snd:is_playing(sources.eng_ext) == false then 
				self.snd:play_loop(sources.eng_ext, sounds.eng_ext);
            end
            
			self.snd:set_gain (sources.prop_ext, clamp(propeller_rpm/900, 0, 2));
            
            if self.snd:is_playing(sources.prop_ext) == false then 
				self.snd:play_loop(sources.prop_ext, sounds.prop_ext);
			end
			
			self.snd:set_gain (sources.rumble_ext, clamp(propeller_rpm/1200, 0, 2));
			
			if self.snd:is_playing(sources.rumble_ext) == false then
				self.snd:play_loop(sources.rumble_ext, sounds.rumble);
			end
		end
	else 
		self.snd:stop(sources.eng_ext);
		self.snd:stop(sources.eng_int);
        self.snd:stop(sources.prop_ext);
		self.snd:stop(sources.prop_int);
		self.snd:stop(sources.rumble_ext);
		self.snd:stop(sources.rumble_int);
    end
    
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

