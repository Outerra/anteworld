-- It is needed to implement interface, to be able to use interface functionality (in this case ot.script_module).
implements("ot.script_module")

-- Get our lua math library
local vec_math = require("lib/lua/vec_math")


-- Game settings 
local INVADERS_SPEED = 1
local MAX_HEARTS = 5
local MAX_PROJECTILES = 4
local PROJECTILE_LOAD_TIME = 1
local TIME_NEAR_END_CURVE = 43

-- Paths to objdefs
local NORMAL_INVADER_PATH = "invader_ships/invader_ship_01"
local SPECIAL_INVADER_PATH = "invader_ships/invader_ship_02"
local CANNON_PATH = "cannon/cannon"

-- Fonts
local font_xolonium_50_bold 
local font

-- Images
local projectile_img
local heart_img

-- Flags for object loading status
local invader_01_loaded
local invader_02_loaded
local cannon_loaded

-- Screen-related variables
local screen_size
local screen_center

-- Time-related variables
local timer
local last_spawn_time

-- Invader and level management
local invaders_in_line
local spawning
local spawned_lines
local lvl_stage
local invader_array
local invader_lines_in_group
local stronger_invaders_to_spawn

-- Cannon and projectile management
local cannon
local projectiles_loaded
local loaded_time
local cannnon_got
local cannon_geom
local barrel_tip_id
local cannon_pitch_id
local cannon_entered

-- Game state variables
local hearts_remaining
local score
local instances_count
local invaders_destroyed
local game_over

-- Location and time related variables
local is_loc_ready
local time_set

-- End position trigger
local trigger_sensor

-- Miscellaneous flags
local exit_game

------------- Functions --------------

-- Helper function to check whether a specific value exists in a Lua table
function table_contains(tbl, val)
    -- Iterate over all elements in the table
    for _, v in ipairs(tbl) do
        -- If the element with required value is found, return true
        if v == val then
            return true
        end
    end
    -- If no match was found, return false after completing the loop
    return false
end

-- Helper function to check if all invaders in line were destroyed 
function invaders_in_line_destroyed(tbl)
    for _, v in ipairs(tbl) do
        if not v.removed then
            return false
        end
    end
    return true
end

-- Function to draw the crosshair on the screen
function draw_crosshair(api)
    -- draw_line() function draws 2D line (width and smooth can be set with function set_line_params() in canvas interface)
    -- 1. param - start position of the line on x axis of the screen
    -- 2. param - start position of the line on y axis of the screen
    -- 3. param - end position of the line on x axis of the screen
    -- 4. param - end position of the line on y axis of the screen
    -- 5. param - color of the line (in range 0-255)
    api.canvas:draw_line(screen_center.x + 10, screen_center.y, (screen_center.x + 30), (screen_center.y), {r = 255, g = 255, b = 255, a = 255})
    api.canvas:draw_line(screen_center.x - 10, screen_center.y, (screen_center.x - 30), (screen_center.y), {r = 255, g = 255, b = 255, a = 255})
    api.canvas:draw_line(screen_center.x, screen_center.y + 10, (screen_center.x), (screen_center.y + 30), {r = 255, g = 255, b = 255, a = 255})
    api.canvas:draw_line(screen_center.x, screen_center.y - 10, (screen_center.x), (screen_center.y - 30), {r = 255, g = 255, b = 255, a = 255})
end

-- Function to create an explosion at a specified location
function create_explosion(api, pos, obj_hit)
    -- Set emit radius for explosions
    local emit_radius = 2

    -- Use create_solid_particles() function from explosions interface, to create explosion of solid particles
    -- 1. param - ECEF world position
    -- 2. param - smoke ejection direction (normalized pos for the upward dir)
    -- 3. param - radius of the emitter area
    -- 4. param - max particle radius
    -- 5. param - ejection speed
    -- 6. param - spread direction dissipation, tangent of the half-angle (default: 0.4)
    -- 7. param - highlight 0:solid, 1+:water spray (default: 0.0)
    -- 8. param - age in seconds at the creation time (default: 0)
    -- 9. param - base particle color (default: {r:0.03, g:0.02, b:0.01})
    -- 10. param - highlight color in rgb (can be >1 for bloom), a: highlight inverse size coefficient (default: {r:40, g:6, b:0, a:10))
    api.explosions:create_solid_particles(pos, {x = 0, y = 0, z = 1}, emit_radius, 0.04 * math.max(1.0, math.log(emit_radius) / math.log(2)), 10, 1)

    -- Make crater on earth, if earth was hit by projectile
    if not obj_hit then
        -- make_crater()
        -- 1. param - ECEF world position
        -- 2. param - approximate crater radius
        api.explosions:make_crater(pos, emit_radius*6)
    end
end

-- Function to rotate a vector by a quaternion
function rotate_vector_by_quaternion(v, q)
    -- Convert the vector into a quaternion with a w component of 0, this is necessary for quaternion-vector multiplication.
    local vec_quat = {x = v.x, y = v.y, z = v.z, w = 0}

    -- Compute the conjugate of the quaternion q. The conjugate of a quaternion is used to reverse the rotation
    local quat_conjugated = vec_math.quat.conjugate(q)

    -- Multiply the quaternion q by the vector quaternion, this combines the rotation quaternion with the vector
    local qv = vec_math.quat.mul(q, vec_quat)

    -- Multiply the result by the conjugate of q, this final multiplication applies the rotation to the vector
    local rotated_qv = vec_math.quat.mul(qv, quat_conjugated)

    -- Return the rotated vector in quaternion form
    return rotated_qv
end

-- Function to spawn invaders
function spawn_invader_line(api, invader_path, SPECIAL_INVADER_PATH, num_of_stronger_invaders)
    -- Set default values for optional parameters
    SPECIAL_INVADER_PATH = SPECIAL_INVADER_PATH or NORMAL_INVADER_PATH
    num_of_stronger_invaders = num_of_stronger_invaders or 0

    -- If the invader model is not loaded, exit the function early
    if not invader_01_loaded or not invader_02_loaded then
        return {}
    end

    -- Array to store normal invader instances
    local invaders = {}

    -- Array to store stronger invader instances
    local stronger_invaders = {}

    -- Calculate where to position stronger invaders in the line
    -- Check if there are stronger invaders to be included in the line
    if num_of_stronger_invaders > 0 then
        -- Calculate the spacing between stronger invaders in the line
        local spacing = math.floor(invaders_in_line / num_of_stronger_invaders)

        -- Loop through the number of stronger invaders to place them at specific positions in the line
        for i = 0, num_of_stronger_invaders - 1 do
            -- Calculate the position of each stronger invader in the line
            table.insert(stronger_invaders, (i * spacing) + math.floor(spacing / 2))
        end
    end

    -- Define the start position and orientation for the invader
    local start_loc = {x = -1942405.2868467504, y = 5212454.450893189, z = 3122019.4702613624}
    local start_rot = {x = 0.22153694927692413, y = 0.4521426856517792, z = 0.6030480861663818, w = -0.618725597858429}

    -- Adjust the starting position of the first 3 waves, so that they are closer on the game start
    if #invader_array == 0 then
        start_loc = {x = -1941927.023828117, y = 5212336.678825753, z = 3122498.8268912034}
        start_rot = {x = 0.04354477673768997, y = 0.5016129612922668, z = 0.7856196165084839, w = -0.3595695197582245}
    elseif #invader_array == 1 then
        start_loc = {x = -1942146.710236884, y = 5212422.649997227, z = 3122225.6552367667}
        start_rot = {x = 0.04354477673768997, y = 0.5016129612922668, z = 0.7856196165084839, w = -0.3595695197582245}
    elseif #invader_array == 2 then
        start_loc = {x = -1942171.8067069917, y = 5212432.471147353, z = 3122194.4487372655}
        start_rot = {x = 0.04354477673768997, y = 0.5016129612922668, z = 0.7856196165084839, w = -0.3595695197582245}
    end

    -- Define a local offset vector (distance between invaders)
    local local_offset = {x = -25, y = 0, z = 0}

    -- Rotate the local offset vector by the invader rotation quaternion
    local world_movement = rotate_vector_by_quaternion(local_offset, start_rot)

    -- Loop to create and position invader instances
    for i = 0, invaders_in_line - 1 do
        -- Calculate the new position for the invader by applying the offset
        local start_pos = {
            x = start_loc.x + i * world_movement.x,
            y = start_loc.y + i * world_movement.y,
            z = start_loc.z + i * world_movement.z
        }

        -- Initialize a new invader object
        local invader = {
            instance = nil,  -- Placeholder for the invader's instance
            lives = 1,       -- Default health
            special = false, -- Whether this invader is special (stronger)
            removed = false  -- Whether this invader has been removed from the scene
        }

        -- If the current index corresponds to a stronger invader, spawn stronger invader and set its properties
        if table_contains(stronger_invaders, i) then
            -- Create special invader
            -- 1. param - full model path under packages dir
            -- 2. param - world position of the pivot point
            -- 3. param - orientation of the model
            -- 4. param - if static object should be placed permanently into the world (if instance remains after game has been restarted)
            invader.instance = api:create_instance(SPECIAL_INVADER_PATH, start_pos, start_rot, 4)

            -- Define parameters for special invader
            invader.lives = 2
            invader.special = true
        else
            -- Spawn normal invader with default properties
            invader.instance = api:create_instance(invader_path, start_pos, start_rot, 4)
        end

        -- If the instance creation failed, return an empty table
        if not invader.instance then
            return {}
        end

        -- Add the invader to the array of invaders
        table.insert(invaders, invader)
    end

    -- Return object, containing the array of invaders, spawn time and state
    return {invaders = invaders, spawn_time = timer}
end

-- Calculates the number of stronger invaders to be placed in a specific line based on the total number of stronger invaders and the total number of lines.
function calculate_stronger_invaders_per_line(line_index, total_lines, total_stronger_invaders)
    -- Calculate the base number of stronger invaders that each line should receive
    local base_count = math.floor(total_stronger_invaders / total_lines)

    -- Calculate any leftover stronger invaders that couldn't be evenly divided among the lines.
    local extra_invaders = total_stronger_invaders % total_lines

    -- If the current line index is less than the number of extra invaders,
    -- this line gets one extra stronger invader. Otherwise, it just gets the base count
    if line_index < extra_invaders then
        return base_count + 1
    else
        return base_count
    end
end

-- Convert quaternion to its corresponding axis of rotation
function quaternion_to_axis(q)
    local Θ = math.acos(q.w) * 2
    local sinΘ = math.sin(Θ / 2)

    local ax = q.x / sinΘ
    local ay = q.y / sinΘ
    local az = q.z / sinΘ

    return {x = ax, y = ay, z = az}
end

-- Called on game over
function on_game_over(api)
    game_over = true

    -- Open a window, loading the specified HTML
    -- param - relative path, with optional query part (e.g. url?param1&param2 ...)

    -- recognized tokens for relative path:
    -- name - window name
    -- width - initial window width
    -- height - initial window height
    -- x - initial window x position
    -- y - initial window y position
    -- transp - true or 1 if the window should support transparency
    api:open_window("www/GameOverScreen.html?width=" .. screen_size.x .. "&height=" .. screen_size.y)
end

-- Reload the game
function reload_game()
    -- Remove remaining invaders from the scene
    for i = 1, #invader_array do
        for j = 1, #invader_array[i].invaders do
            local invader = invader_array[i].invaders[j]

            if invader.instance and not invader.removed then
                invader.instance:remove_from_scene()
            end
        end
    end

    -- Set values to initial state
    timer = 0
    last_spawn_time = 0
    score = 0
    spawning = true
    invaders_in_line = 6
    lvl_stage = 1
    spawned_lines = 0
    projectiles_loaded = MAX_PROJECTILES
    hearts_remaining = MAX_HEARTS
    loaded_time = 0
    instances_count = 0
    invader_lines_in_group = 1
    stronger_invaders_to_spawn = 0
    invaders_destroyed = 0
    game_over = false
    time_set = false

    trigger_sensor = {}
    invader_array = {}
end

------------- Events --------------

function ot.script_module:on_main_menu(is_paused)
    -- Method for pausing the game mod
    -- param - true, if mod should be paused
    self:pause(is_paused)
end

-- Event for communicating between HTML window and script
-- This function is called from the HTML script
function ot.script_module:on_set_value_num(a, b, value)
    -- If the function is called with certain combination of parameters 'a' and 'b', do corresponding action
    if a == 0 and b == 0 then
        -- Reload game was selected
        reload_game()
    end

    if a == 0 and b == 1 then
        -- Exit game was selected
        exit_game = value
    end
    
    return 0
end

-- Event for communicating between HTML window and script
-- This function is called from the HTML script
function ot.script_module:on_get_value_num(param_a, param_b, value)
    -- If the function is called with certain combination of parameters 'a' and 'b', do corresponding action
    if param_a == 0 and param_b == 0 then
        -- Sends value to HTML script
        -- Send number of destroyed invaders (to show on game over screen)
        return {_ret = 0, value = invaders_destroyed}
    end

    if param_a == 0 and param_b == 1 then
        -- Send score (to show on game over screen)
        return {_ret = 0, value = score}
    end
end


-- Event called on mouse button press
function ot.script_module:on_mouse_button(mouse_button, state, modifiers)
    -- If button is released, do nothing
    if not state then
        return false
    end

    -- When left mouse button is pressed
    if mouse_button == 0 then
    
        -- Check if there are projectiles loaded in the cannon
        if projectiles_loaded >= 1 then
            -- Get camera direction
            local camera_direction = self:get_camera_dir()

            -- Multiply to set speed
            camera_direction.x = camera_direction.x * 400
            camera_direction.y = camera_direction.y * 400
            camera_direction.z = camera_direction.z * 400

            if cannon then
                -- Get barrel tip bone ECEF position
                local barrel_tip_pos_ECEF = cannon_geom:get_joint_ecef_pos(barrel_tip_id)

                -- Launch a ballistic tracer/projectile, using launch_tracer() function from explosions interface
                -- 1. param - launch position
                -- 2. param - launch speed vector
                -- 3. param - tracer size
                -- 4. param - tracer color
                -- 5. param - fadeout emission reduction parameter for each older point on the trail (default: 0.5)
                -- 6. param - length of the trail in seconds (default: 0.2)
                -- 7. param - time [s] of tracer existence: <=0 means until hitting the ground (default: 0.0)
                -- 8. param - age of the tracer. Affects trail length, fall speed (default: 0)
                -- 9. param - tracer id to reuse (default: 0xffffffffUL)
                -- 10. param - object id to be dragged by tracer (default: ())
                -- 11. param - custom value (default: 0)
                -- returns - trace id
                self.explosions:launch_tracer(barrel_tip_pos_ECEF, camera_direction, 100, {x = 0.5, y = 0.5, z = 0.5, w = 1})
            end

            -- When all projectiles were loaded, while shooting, set the "loaded_time", so that following projectile starts loading
            if projectiles_loaded == MAX_PROJECTILES then
                loaded_time = timer
            end

            -- Decrease loaded projectiles count
            projectiles_loaded = projectiles_loaded - 1
        end
    end
   
    return true     
end

-- Event called when an object is preloaded (using preload_object() function)
function ot.script_module:on_preload_object_done(model_path)
    -- Set flags when objects are preloaded
    if model_path == NORMAL_INVADER_PATH then
        invader_01_loaded = true
    elseif model_path == SPECIAL_INVADER_PATH then
        invader_02_loaded = true
    elseif model_path == CANNON_PATH then
        cannon_loaded = true
    end
end

-- Event called when the script is reloaded
function ot.script_module:on_reload()
    -- Call function to reset game state
    reload_game()
    
    return true
end

-- Initialization
function ot.script_module:on_initialize()
    -- Get explosions interface
    self.explosions = query_interface("lua.ot.explosions.get")

    -- Get canvas interface
    self.canvas = query_interface("lua.ot.canvas.create", true, true)

    -- Get screen size from world interface
    screen_size = self:screen_size()

    -- Calculate screen center
    screen_center = {x = screen_size.x / 2, y = screen_size.y / 2}

    -- Set initial values
    hearts_remaining = MAX_HEARTS
    projectiles_loaded = MAX_PROJECTILES
    cannon = nil
    cannon_geom = nil
    game_over = false
    spawning = true
    cannon_got = false
    cannon_entered = false
    time_set = false
    invader_01_loaded = false 
    invader_02_loaded = false 
    cannon_loaded = false
    exit_game = false
    is_loc_ready = false
    time_set = false
    invader_01_loaded = false; 
    invader_02_loaded = false; 
    cannon_loaded = false;
    is_loc_ready = false;  
    exit_game = false;
    timer = 0
    last_spawn_time = 0
    score = 0
    invaders_in_line = 6
    lvl_stage = 1
    spawned_lines = 0
    loaded_time = 0
    instances_count = 0
    invaders_destroyed = 0
    invader_lines_in_group = 1
    stronger_invaders_to_spawn = 0

    invader_array = {}
    trigger_sensor = {}

    -- Set smooth and width parameters of canvas lines
    -- 1. param - width of the canvas lines
    -- 2. param - smooth size of the canvas lines
    self.canvas:set_line_params(3, 1)

    -- Load fonts from fnt file
    -- param - path to fnt file
    font_xolonium_50_bold = self.canvas:load_font("ui/xolonium_50_bold.fnt")
    font = self.canvas:load_font("ui/hud.fnt")

    -- Load images
    -- param - path to imgset file
    projectile_img = self.canvas:load_image("projectile.imgset/projectile")
    heart_img = self.canvas:load_image("heart-icon.imgset/heart")

    -- Use preload_object() function from script_module api, to preload object
    -- event on_preload_object_done() from script_module api is called when object is preloaded
    self:preload_object(NORMAL_INVADER_PATH)
    self:preload_object(SPECIAL_INVADER_PATH)
    self:preload_object(CANNON_PATH)

    -- Jump to a location that was saved in the game campos
    -- 1. param - save name
    -- 2. param - if date/time is contained in the location file, apply it
    -- returns - true if the location was found
    self:load_location("minigame_location", false)

    -- Create sensor of sphere shape
    -- This sensor is used as trigger objects on the end position (checks if an invader reached the end position)
    -- 1. param - position
    -- 2. param - rotation
    -- 3. param - radius
    self:create_sensor({x = -1941638.9198791014, y = 5211794.163754703, z = 3123587.0791307855}, {x = 0, y = 0, z = 0, w = 1}, 100)

    -- Function needs to return a boolean
    return true
end

-- Similar to simulation_step from vehicle_script API (called 60 times per second)
function ot.script_module:before_simulation_step(dtns, ns_sim)

    -- Check if location is ready
    -- returns - true if last loaded location is ready
    is_loc_ready = self:is_location_ready()

    -- Return if the location is not loaded
    if not is_loc_ready then
        return
    end
    
    -- Set time, if not yet set (to not start the minigame at night...)
    if not time_set then
        -- Set the time of the day
        -- 1. param - day of the year
        -- 2. param - time of the day in seconds
        -- 3. param - if true set UTC, false set solar time for current location
        self:set_time(1, 36000, false)
        
        time_set = true
    end

    if cannon_loaded and not cannon_entered then
        if cannon == nil then
            -- Spawn cannon
            local cannon_start_pos = {x = -1941613.3096685943, y = 5212175.574482916, z = 3123162.568296084}
            local cannon_start_rot = {x = -0.49617794156074524, y = -0.09487587213516235, z = 0.14400778710842133, w = 0.8509216904640198}
            cannon = self:create_static_instance(CANNON_PATH, cannon_start_pos, cannon_start_rot, 2)
        else
            -- Get cannon geometry
            cannon_geom = cannon:get_geomob()

            -- Get cannon bones
            if cannon_geom ~= nil then
                barrel_tip_id = cannon_geom:get_joint("barrel_tip")
                cannon_pitch_id = cannon_geom:get_joint("cannon_pitch")
            
                -- Enter cannon
                cannon:enter()
                
                cannon_entered = true
            end
        end
    end

    -- Do not continue until cannon is loaded
    if not cannon then
        return
    end

    -- Do not continue on game over
    if game_over then
        return
    -- When all lives are lost, set game over
    elseif hearts_remaining <= 0 then
        game_over = true
        on_game_over(self)

        return
    end

    -- Calculate dt
    local dt = dtns / 1000000000
    -- Increment the timer by the delta time
    timer = timer + dt

    -- Load projectile after some time
    if timer - loaded_time > PROJECTILE_LOAD_TIME and projectiles_loaded < MAX_PROJECTILES then
        loaded_time = timer
        projectiles_loaded = projectiles_loaded + 1
    end

    -- Check if invaders got to end position (if they triggered the sensor on end position)
    -- Method "get_tiggered_sensors" returns an array of objects that triggered the sensor in the current frame
    local trigger_sensor = self:get_tiggered_sensors()

    -- If an invader triggered the sensor, loop through the invaders to find the triggered one, based on its id
    if #trigger_sensor > 0 then
        for t = #trigger_sensor, 1, -1 do
            -- Get the id of the invader that triggered the sensor
            local triggered_id = trigger_sensor[t].trigger_entity_id

            -- Get the invader object from the triggered id
            local triggered_invader_obj = self:get_object(triggered_id)

            -- Check if object was successfully received
            if triggered_invader_obj ~= nil then
                local break_outer_loop = false
            
                -- Loop through the invaders
                for i = #invader_array, 1, -1 do
                
                    if break_outer_loop == true then
                        break
                    end
                    
                    local invaders_line = invader_array[i].invaders

                    for j = #invaders_line, 1, -1 do
                        local invader = invaders_line[j]

                        -- Skip the invaders that are removed
                        if invader.removed then
                            goto continue_invader
                        end

                        -- Check if the invader is the one that triggered the sensor, by comparing the id
                        if triggered_invader_obj:id() == invader.instance:id() then
                            -- Set the "removed" flag
                            invader.removed = true
                            -- Remove the invader
                            self:remove_object(triggered_id)

                            local current_line_invaders = invader_array[i].invaders
                            
                            -- Check if all invaders are removed
                            if invaders_in_line_destroyed(current_line_invaders) then    
                                -- If all invaders in the current line have been removed, remove the line from invader_array
                                table.remove(invader_array, i)
                            end

                            -- Decrease hp
                            hearts_remaining = hearts_remaining - 1

                            -- If the triggered invader was found, break out of the outer loop
                            break_outer_loop = true
                        end

                        ::continue_invader::
                    end
                end
            end
        end
    end

    -- Spawn multiple lines of invaders each second
    if lvl_stage <= 2 or (spawning and timer - last_spawn_time > 1) then
        -- Initialize invader line object
        local invader_line = {}

        -- Calculate number of stronger invaders in group
        local stronger_invaders_in_group = math.floor((lvl_stage - 2) * 1.2)

        -- Calculate number of stronger invaders per line
        local stronger_invaders_per_line = calculate_stronger_invaders_per_line(spawned_lines, invader_lines_in_group, stronger_invaders_in_group)

        -- Calculate total number of invaders
        local total_invaders = invader_lines_in_group * invaders_in_line

        -- If the number of stronger invaders in group is bigger than normal invaders, add an additional invader line
        if stronger_invaders_in_group > total_invaders / 2 then
            invader_lines_in_group = invader_lines_in_group + 1
        end

        -- Start spawning stronger invaders on the 3rd wave
        if lvl_stage >= 3 then
            stronger_invaders_per_line = stronger_invaders_per_line
        else
            stronger_invaders_per_line = 0
        end

        -- Spawn lines of invaders
        invader_line = spawn_invader_line(self, NORMAL_INVADER_PATH, SPECIAL_INVADER_PATH, stronger_invaders_per_line)

        -- Adjust the spawn time for early waves
        if lvl_stage == 1 then
            invader_line.spawn_time = invader_line.spawn_time - 26 * (1 / INVADERS_SPEED)
        elseif lvl_stage == 2 then
            invader_lines_in_group = 2

            if spawned_lines == 0 then
                invader_line.spawn_time = invader_line.spawn_time - 13 * (1 / INVADERS_SPEED)
            elseif spawned_lines == 1 then
                invader_line.spawn_time = invader_line.spawn_time - 11.5 * (1 / INVADERS_SPEED)
            end
        end

        -- Return if invader_line is empty
        if next(invader_line) == nil then
            return
        end

        -- Push invader line into invader array
        table.insert(invader_array, invader_line)
        last_spawn_time = timer
        spawned_lines = spawned_lines + 1

        -- Stop spawning when the desired number of invader lines has been spawned
        if spawned_lines == invader_lines_in_group then
            if lvl_stage >= 3 then
                spawning = false
            end

            spawned_lines = 0
            lvl_stage = lvl_stage + 1
        end
    -- Start spawning additional group of invaders after a certain time
    elseif not spawning and timer - last_spawn_time > 12 then
        spawning = true
    end

    -- Get projectiles that hit something
    local landed_projectile = self:get_landed_projectiles()

    -- If projectile hit something
    if #landed_projectile > 0 then
        for _, projectile in ipairs(landed_projectile) do
            local hit_pos = projectile.pos
            local hit_id = projectile.hitid

            -- Check if the hit object was not earth
            if hit_id ~= 0xffffffff then                
                local break_outer_loop = false
            
                for i = 1, #invader_array do
                
                    if break_outer_loop == true then
                        break
                    end
                
                    for j = 1, #invader_array[i].invaders do
                        local invader = invader_array[i].invaders[j]

                        -- Check if the hit ID matches the invader's ID
                        if hit_id == invader.instance:id() then
                            local invader_pos = invader.instance:get_pos()
                            local hit_pos_ecef = {
                                x = invader_pos.x + hit_pos.x,
                                y = invader_pos.y + hit_pos.y,
                                z = invader_pos.z + hit_pos.z
                            }

                            -- Create explosion
                            create_explosion(self, hit_pos_ecef, true)

                            invader.lives = invader.lives - 1

                            -- If invader has no lives remaining
                            if invader.lives <= 0 then
                                invaders_destroyed = invaders_destroyed + 1

                                -- Add score based on the invader type
                                if invader.special then
                                    score = score + 5
                                else
                                    score = score + 1
                                end

                                invader.removed = true
                                invader.instance:remove_from_scene()

                                local current_line_invaders = invader_array[i].invaders
                                
                                -- Check if all invaders in line are are removed
                                if invaders_in_line_destroyed(current_line_invaders) then    
                                    -- If all invaders in the current line have been removed, remove the line from invader_array
                                    table.remove(invader_array, i)
                                end
                            end
                            
                            break_outer_loop = true
                        end
                    end
                end
            else
                -- Create explosion for non-invader hits
                create_explosion(self, hit_pos, false)
            end
        end
    end

    -- Check if invader model was loaded
    if not invader_01_loaded then
        return
    end

    -- Check if there are any invaders in the array
    if #invader_array > 0 then
        for i = #invader_array, 1, -1 do
            local line_timer = timer - invader_array[i].spawn_time
            local invaders_line = invader_array[i].invaders

            for index = #invaders_line, 1, -1 do
                local invader = invaders_line[index]

                if invader.removed then
                    goto continue_invader_2
                end

                local invader_pos = invader.instance:get_pos()
                local invader_rot = invader.instance:get_rot()

                local distance_traveled = line_timer * INVADERS_SPEED
                local local_movement = {x = 0, y = INVADERS_SPEED * dt * 30, z = 0}

                -- Define the axis of rotation and angle
                local axis = {x = 0, y = 0, z = -1}
                local angle = 0

                -- Adjust movement and rotation at different points in the path
                if distance_traveled > 1 and distance_traveled < 8.7 then
                    angle = 0.0015 * INVADERS_SPEED
                    local_movement = {x = 0, y = (0.4 + index / 22) * INVADERS_SPEED, z = 0}
                elseif distance_traveled > 45.5 and distance_traveled < 70 then
                    local_movement = {x = 0, y = (0.4 + index / 40) * INVADERS_SPEED, z = 0}
                    angle = 0.00082 * INVADERS_SPEED
                end

                -- Rotate invader and apply movement
                
                local angle2 = angle / 2
                local sin_ang = math.sin(angle2)

                local rot = {x = 0, y = 0 , z = 0, w = 0}
                rot.x = axis.x * sin_ang
                rot.y = axis.y * sin_ang
                rot.z = axis.z * sin_ang
                rot.w = math.cos(angle2)

                local final_rot = vec_math.quat.mul(invader_rot, rot)
                invader.instance:set_rot(final_rot)

                local world_movement = rotate_vector_by_quaternion(local_movement, invader_rot)

                local new_position = {
                    x = invader_pos.x + world_movement.x * dt / (1 / 60),
                    y = invader_pos.y + world_movement.y * dt / (1 / 60),
                    z = invader_pos.z + world_movement.z * dt / (1 / 60)
                }

                invader.instance:set_pos(new_position)

                ::continue_invader_2::
            end
        end
    end

    -- Update remaining invaders count
    instances_count = 0
    for i = 1, #invader_array do
        for _, invader in ipairs(invader_array[i].invaders) do
            if not invader.removed then
                instances_count = instances_count + 1
            end
        end
    end
end

 

function ot.script_module:visual_update(dtrender, dtsim, dtinterpolate)

    if not cannon then
        -- Fill the whole screen with black rectangle, until the cannon is loaded
        -- 1. param - start of the rectangle on x axis
        -- 2. param - start of the rectangle on y axis
        -- 3. param - width of the rectangle
        -- 4. param - height of the rectangle
        canvas:fill_rect(0, 0, screen_size.x, screen_size.y, {r = 0, g = 0, b = 0, a = 255})

        -- Draw loading text on screen, until the cannon is loaded
        -- 1. param - font
        -- 2. param - position on x axis (bottom left corner of the text)
        -- 3. param - position on y axis (bottom left corner of the text)
        -- 4. param - text
        -- 5. param - color
        canvas:draw_text(font, screen_center.x - 70, screen_center.y - 7, "Loading minigame... ", {r = 255, g = 255, b = 255, a = 255})

        return
    end

    -- Return if the location is not loaded
    if not is_loc_ready then
        return
    end

    -- Draw crosshair
    draw_crosshair(self)

    -- Draw background behind the score
    self.canvas:fill_rect(20, screen_size.y - 20, 210 + 40 * string.len(tostring(score)), -50, {r = 15, g = 0, b = 60, a = 255})

    -- Draw score on the screen
    self.canvas:draw_text(font_xolonium_50_bold, 30, screen_size.y - 67, "score: " .. score, {r = 0, g = 150, b = 150, a = 255})

    -- Draw number of remaining invaders on the screen
    self.canvas:draw_text(font, screen_size.x - 250, screen_size.y - 67, "Invaders remaining: " .. instances_count, {r = 0, g = 0, b = 0, a = 255})

    -- Draw projectiles on the screen
    -- 1. param - Image ID
    -- 2. param - position on x axis (bottom left corner of the image)
    -- 3. param - position on y axis (bottom left corner of the image)
    -- 4. param - width
    -- 5. param - height
    -- 6. param - color
    for p = 0, MAX_PROJECTILES - 1 do
        self.canvas:draw_image_wh(projectile_img, 30, 20 + (40 * p), 110, 30, {r = 255, g = p < projectiles_loaded and 255 or 0, b = p < projectiles_loaded and 255 or 0, a = 255})
    end

    -- Draw lives on the screen
    for h = 0, MAX_HEARTS - 1 do
        self.canvas:draw_image_wh(heart_img, screen_size.x - 50 - (40 * h), 20, 40, 40, {r = 255, g = 255, b = 255, a = h < hearts_remaining and 255 or 100})
    end
end

function ot.script_module:on_final_camera(pos, rot)
    -- Return if the location is not loaded
    if not is_loc_ready then
        return
    end

    -- Check if cannon and it's geometry were successfully loaded
    if cannon == nil or cannon_geom == nil then
        return
    end

    -- Get cannon inverse rotation
    local cannon_rot_inv = vec_math.quat.inverse(cannon_geom:get_rot())

    -- Multiply the cannon inverse rotation with the camera rotation
    rot = vec_math.quat.mul(cannon_rot_inv, rot)

    -- Get axis
    local axis = quaternion_to_axis(rot)

    -- Calculate angle
    local angle = 2 * math.acos(rot.w)

    -- Rotate cannon
    cannon_geom:rotate_joint_orig(cannon_pitch_id, angle, {x = axis.x, y = axis.y, z = axis.z})
end

