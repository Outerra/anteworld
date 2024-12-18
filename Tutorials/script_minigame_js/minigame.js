// Get our js math library
$include("/lib/vecmath.js");

// Game settings (Parameters and Flags)
const INVADERS_SPEED = 1
const MAX_HEARTS = 5
const MAX_PROJECTILES = 4
const PROJECTILE_LOAD_TIME = 1
const TIME_NEAR_END_CURVE = 43

// Paths to objdefs
const NORMAL_INVADER_PATH = "invader_ships/invader_ship_01"
const SPECIAL_INVADER_PATH = "invader_ships/invader_ship_02"
const CANNON_PATH = "cannon/cannon"

// Fonts
let font_xolonium_50_bold 
let font

// Images
let projectile_img
let heart_img

// Flags for object loading status
let invader_01_loaded
let invader_02_loaded
let cannon_loaded

// Screen-related variables
let screen_size
let screen_center

// Time-related variables
let timer
let last_spawn_time

// Invader and level management
let invaders_in_line
let spawning
let spawned_lines
let lvl_stage
let invader_array
let invader_lines_in_group
let stronger_invaders_to_spawn

// Cannon and projectile management
let cannon
let projectiles_loaded
let loaded_time
let cannnon_got
let cannon_geom
let barrel_tip_id
let cannon_pitch_id
let cannon_entered

// Game state variables
let hearts_remaining
let score
let instances_count
let invaders_destroyed
let game_over

// Location and time related variables
let is_loc_ready
let time_set

// End position trigger
let trigger_sensor

// Miscellaneous flags
let exit_game

//------------- Functions --------------

// Function to draw the crosshair on the screen
function draw_crosshair()
{
    //draw_line() function draws 2D line (width and smooth can be set with function set_line_params() in canvas interface)
    // 1. param - start position of the line on x axis of the screen
    // 2. param - start position of the line on y axis of the screen
    // 3. param - end position of the line on x axis of the screen
    // 4. param - end position of the line on y axis of the screen
    // 5. param - color of the line (in range 0-255)
    this.canvas.draw_line(screen_center.x + 10, screen_center.y, (screen_center.x + 30), (screen_center.y), {r: 255, g: 255, b: 255, a: 255});
    this.canvas.draw_line(screen_center.x - 10, screen_center.y, (screen_center.x - 30), (screen_center.y), {r: 255, g: 255, b: 255, a: 255});
    this.canvas.draw_line(screen_center.x, screen_center.y + 10, (screen_center.x), (screen_center.y + 30), {r: 255, g: 255, b: 255, a: 255});
    this.canvas.draw_line(screen_center.x, screen_center.y - 10, (screen_center.x), (screen_center.y - 30), {r: 255, g: 255, b: 255, a: 255});
}

// Function to create an explosion at a specified location
function create_explosion(pos, obj_hit)
{
    // Set emit radius for explosions
    let emit_radius = 2;

    // Use create_solid_particles() function from explosions interface, to create explosion of solid particles
    // 1. param - ECEF world position
    // 2. param - smoke ejection direction (normalized pos for the upward dir)
    // 3. param - radius of the emitter area
    // 4. param - max particle radius
    // 5. param - ejection speed
    // 6. param - spread direction dissipation, tangent of the half-angle (default: 0.4)
    // 7. param - highlight 0:solid, 1+:water spray (default: 0.0)
    // 8. param - age in seconds at the creation time (default: 0)
    // 9. param - base particle color (default: {r:0.03, g:0.02, b:0.01})
    // 10. param - highlight color in rgb (can be >1 for bloom), a: highlight inverse size coefficient (default: {r:40, g:6, b:0, a:10))
    this.explosions.create_solid_particles(pos, {x: 0, y: 0, z: 1}, emit_radius, 0.04 * Math.max(1.0, Math.log2(emit_radius)), 10, 1);

    // Make crater on earth, if earth was hit by projectile
    if(!obj_hit)
    {
        // make_crater()
        // 1. param - ECEF world position
        // 2. param - approximate crater radius
        this.explosions.make_crater(pos, emit_radius*6);
    }
}

// Function to rotate a vector by a quaternion
function rotate_vector_by_quaternion(v, q)
{
    // Convert the vector into a quaternion with a w component of 0, this is necessary for quaternion-vector multiplication.
    let vec_quat = {x:v.x, y:v.y, z:v.z, w:0};

    // Compute the conjugate of the quaternion q. The conjugate of a quaternion is used to reverse the rotation
    let quat_conjugated = Quaternion.conjugate(q);

    // Multiply the quaternion q by the vector quaternion, this combines the rotation quaternion with the vector
    let qv = Quaternion.mul(q, vec_quat);

    // Multiply the result by the conjugate of q, this final multiplication applies the rotation to the vector
    let rotated_qv = Quaternion.mul(qv, quat_conjugated);

    // Return the rotated vector in quaternion form
    return rotated_qv;
}

// Function to spawn invaders
function spawn_invader_line(invader_path, SPECIAL_INVADER_PATH = NORMAL_INVADER_PATH, num_of_stronger_invaders = 0)
{
    // If the invader model is not loaded, exit the function early
    if (!invader_01_loaded || !invader_01_loaded)
    {
        return {};
    }

    // Array to store normal invader instances
    let invaders = [];

    // Array to store stronger invader instances
    let stronger_invaders = [];

    // Calculate where to position stronger invaders in the line
    // Check if there are stronger invaders to be included in the line
    if (num_of_stronger_invaders > 0)
    {
        // Calculate the spacing between stronger invaders in the line
        let spacing = Math.floor(invaders_in_line / num_of_stronger_invaders);

        // Loop through the number of stronger invaders to place them at specific positions in the line
        for (let i = 0; i < num_of_stronger_invaders; i++)
        {
            // Calculate the position of each stronger invader in the line
            stronger_invaders.push((i * spacing) + Math.floor(spacing / 2));
        }
    }


    // Define the start position and orientation for the invader
    let start_loc = {x:-1942405.2868467504,y:5212454.450893189,z:3122019.4702613624}
    let start_rot = {x:0.22153694927692413,y:0.4521426856517792,z:0.6030480861663818,w:-0.618725597858429}

    // Adjust the starting position of the first 3 waves, so that they are closer on the game start
    if(invader_array.length == 0 )
    {
        start_loc = {x:-1941927.023828117,y:5212336.678825753,z:3122498.8268912034};
        start_rot = {x:0.04354477673768997,y:0.5016129612922668,z:0.7856196165084839,w:-0.3595695197582245};
    }
    else if(invader_array.length == 1)
    {
        start_loc = {x:-1942146.710236884,y:5212422.649997227,z:3122225.6552367667};
        start_rot = {x:0.04354477673768997,y:0.5016129612922668,z:0.7856196165084839,w:-0.3595695197582245};
    }
    else if(invader_array.length == 2)
    {
        start_loc = {x:-1942171.8067069917,y:5212432.471147353,z:3122194.4487372655};
        start_rot = {x:0.04354477673768997,y:0.5016129612922668,z:0.7856196165084839,w:-0.3595695197582245};
    }

    // Define a local offset vector (distance between invaders)
    let local_offset = {x:-25, y:0, z:0};

     // Rotate the local offset vector by the invader rotation quaternion
    let world_movement = this.rotate_vector_by_quaternion(local_offset, start_rot);

    // Loop to create and position invader instances
    for (let i = 0; i < invaders_in_line; i++)
    {
        // Calculate the new position for the invader by applying the offset
        let start_pos = {
            x: start_loc.x + i * world_movement.x,
            y: start_loc.y + i * world_movement.y,
            z: start_loc.z + i * world_movement.z
        };

        // Initialize a new invader object
        let invader = {
            instance: null,  // Placeholder for the invader's instance
            lives : 1,       // Default health
            special : false, // Whether this invader is special (stronger)
            removed : false  // Whether this invader has been removed from the scene
        };

        // If the current index corresponds to a stronger invader, spawn stronger invader and set it's properties
        if (stronger_invaders.includes(i))
        {
            // Create special invader 
            // 1. param - full model path under packages dir
            // 2. param - world position of the pivot point
            // 3. param - orientation of the model
            // 4. param - if static object should be placed permanently into the world (if instance remains after game has been restarted)
            invader.instance = this.create_instance(SPECIAL_INVADER_PATH, start_pos, start_rot, 4);

            // Define parameters for special invader
            invader.lives = 2;
            invader.special = true;
        }
        else
        {
            // Spawn normal invader with defaul properties
            invader.instance = this.create_instance(invader_path, start_pos, start_rot, 4);
        }

        // If the instance creation failed, return an empty object
        if(!invader.instance)
        {
            return {};
        }

        // Add the invader to the array of invaders
        invaders.push(invader);
    }

    // Return object, containing the array of invaders, spawn time and state
    return { invaders: invaders, spawn_time: timer};
}

//Calculates the number of stronger invaders to be placed in a specific line based on the total number of stronger invaders and the total number of lines.
function calculate_stronger_invaders_per_line(line_index, total_lines, total_stronger_invaders)
{
    // Calculate the base number of stronger invaders that each line should receive
    let base_count = Math.floor(total_stronger_invaders / total_lines);

    // Calculate any leftover stronger invaders that couldn't be evenly divided among the lines.
    let extra_invaders = total_stronger_invaders % total_lines;

    // If the current line index is less than the number of extra invaders,
    // this line gets one extra stronger invader. Otherwise, it just gets the base count
    return line_index < extra_invaders ? base_count + 1 : base_count;
}

// Convert quaternion to its corresponding axis of rotation
function quaternion_to_axis(q)
{
    let Θ = Math.acos(q.w) * 2;
    let sinΘ = Math.sin(Θ / 2);

    let ax = q.x / sinΘ;
    let ay = q.y / sinΘ;
    let az = q.z / sinΘ;

    return { x: ax, y: ay, z: az };
}

// Called on game over
function on_game_over()
{
    game_over = true;

    //Open a browser window, loading specified html
    // param - relative path, with optional query part (e.g. url?param1&param2 ...)
    
    // recognized tokens for relative path:
    // name - window name
    // width - initial window width
    // height - initial window height
    // x - initial window x position
    // y - initial window y position
    // transp - true or 1 if the window should support transparency
    this.open_window("www/GameOverScreen.html?width=" + screen_size.x + "&height=" + screen_size.y);
}

// Reload the game
function reload_game(script_module)
{
    //Remove remaining invaders from the scene
    for (let i = 0; i < invader_array.length; i++)
    {
        for (let j = 0; j < invader_array[i].invaders.length; j++)
        {
            let invader = invader_array[i].invaders[j];

            if (invader.instance && invader.removed == false)
            {
                invader.instance.remove_from_scene();
            }
        }
    }
 
    // Set values to initial state
    timer = 0;
    last_spawn_time = 0;
    score = 0;
    spawning = true;
    invaders_in_line = 6;
    lvl_stage = 1;
    spawned_lines = 0;
    projectiles_loaded = MAX_PROJECTILES;
    hearts_remaining = MAX_HEARTS;
    loaded_time = 0;
    instances_count = 0;
    invader_lines_in_group = 1;
    stronger_invaders_to_spawn = 0;
    invaders_destroyed = 0;
    game_over = false;
    time_set = false;

    trigger_sensor = []
    invader_array = [];
}

//------------- Events --------------

// Events called, when Esc menu is opened/closed
function on_main_menu(is_paused)
{
    // Method for pausing the game mod
    // param - true, if mod should be paused
    this.pause(is_paused);
}

// Event for communicating between html window and script
// This function is called from the html script
function on_set_value_num(a, b, value) 
{
    // If the function is called with certain combination of parameters 'a' and 'b', do corresponding action
    if (a == 0 && b == 0)
    {
        //Reload game was selected
        reload_game(this);
    }
    
    if (a == 0 && b == 1)
    {
        //Exit game was selected
        exit_game = value;
    }
}

// Event for communicating between html window and script
// This function is called from the html script
function on_get_value_num(param_a, param_b, value)
{
    // If the function is called with certain combination of parameters 'a' and 'b', do corresponding action
    if (param_a == 0 && param_b == 0)
    {
        // Sends value to html script
        return {value: invaders_destroyed};
    }

    if (param_a == 0 && param_b == 1)
    {
        return {value: score};
    }
}


// Event called on mouse button press
function on_mouse_button( mouse_button, state, modifiers )
{
    // If button is released, do nothing
    if(!state)
    {
        return;
    }

    // When left mouse button is pressed
    if(mouse_button == 0)
    {
        // Check if there are projectiles loaded in the cannon
        if( projectiles_loaded >= 1)
        {
            // Get camera direction
            let camera_direction = this.get_camera_dir();

            // Multiply to set speed
            camera_direction.x *=400;
            camera_direction.y *=400;
            camera_direction.z *=400;

            if(cannon)
            {
                // Get barrel tip bone ECEF position
                let barrel_tip_pos_ECEF = cannon_geom.get_joint_ecef_pos(barrel_tip_id);

                // Launch a ballistic tracer/projectile, using launch_tracer() function from explosions interface
                // 1. param - launch position
                // 2. param - launch speed vector
                // 3. param - tracer size
                // 4. param - tracer color
                // 5. param - fadeout emission reduction parameter for each older point on the trail (default: 0.5)
                // 6. param - length of the trail in seconds (default: 0.2)
                // 7. param - time [s] of tracer existence: <=0 means until hitting the ground (default: 0.0)
                // 8. param - age of the tracer. Affects trail length, fall speed (default: 0)
                // 9. param - tracer id to reuse (default: 0xffffffffUL)
                // 10. param - object id to be dragged by tracer (default: ())
                // 11. param - custom value (default: 0)
                // returns - trace id
                this.explosions.launch_tracer( barrel_tip_pos_ECEF, camera_direction, 100, {x:0.5,y:0.5,z:0.5, w:1})
            }

            // When all projectiles were loaded, while shooting, set the "loaded_time", so that following projectile starts loading
            if(projectiles_loaded == MAX_PROJECTILES)
            {
                loaded_time = timer;
            }

            // Decrease loaded projectiles count
            projectiles_loaded --;
        }
    }
}

// Event called, when an object is preloaded (using preload_object() function)
function on_preload_object_done(model_path)
{
    // Set flags, when objects are preloaded
    if(model_path == NORMAL_INVADER_PATH)
    {
        invader_01_loaded = true;
    }
    else if(model_path == SPECIAL_INVADER_PATH)
    {
        invader_02_loaded = true;
    }
    else if(model_path == CANNON_PATH)
    {
        cannon_loaded = true;
    }
}

// Event called, when changed script is reloaded
function on_reload()
{
    // Call function to reset game state
    reload_game(this);
}

// Initialization
function on_initialize()
{
    // Bind the functions to ensure that 'this' inside the function refers to the script module object.
    // This way, the methods from the C++ interface can be called from inside the function.
    // Without this binding, calling 'this_interface_method()' inside these functions wouldn't work.  
    this.create_explosion = create_explosion;
    this.draw_crosshair = draw_crosshair;
    this.rotate_vector_by_quaternion = rotate_vector_by_quaternion;
    this.spawn_invader_line = spawn_invader_line;
    this.calculate_stronger_invaders_per_line = calculate_stronger_invaders_per_line;
    this.quaternion_to_axis = quaternion_to_axis;
    this.on_game_over = on_game_over;
    
    // Get explosions interface
    this.explosions = this.$query_interface('ot::js::explosions.get');

    // Get canvas interface
    this.canvas = this.$query_interface('ot::js::canvas.create', true, true);

    // Get screen size from world interface
    screen_size = this.screen_size();

    // Calculate screen center from
    screen_center = {x: screen_size.x / 2, y: screen_size.y / 2};

    // Set initial values
    hearts_remaining = MAX_HEARTS;
    projectiles_loaded = MAX_PROJECTILES;
    cannon = null;
    cannon_geom = null;
    game_over = false;
    spawning = true;
    cannnon_got = false;
    cannon_entered = false;
    time_set = false;
    invader_01_loaded = false; 
    invader_02_loaded = false; 
    cannon_loaded = false;
    is_loc_ready = false;  
    exit_game = false;
    timer = 0;
    last_spawn_time = 0;
    score = 0;
    invaders_in_line = 6;
    lvl_stage = 1;
    spawned_lines = 0;
    loaded_time = 0;
    instances_count = 0;
    invaders_destroyed = 0;
    invader_lines_in_group = 1;
    stronger_invaders_to_spawn = 0;

    invader_array = [];
    trigger_sensor = [];

    // Set smooth and width parameters of canvas lines
    // 1. param - width od the canvas lines
    // 2. param - smooth size of the canvas lines
    this.canvas.set_line_params(3,1);

    // Load fonts from fnt file
    // param - path to fnt file
    font_xolonium_50_bold = this.canvas.load_font( "ui/xolonium_50_bold.fnt" );
    font = this.canvas.load_font( "ui/hud.fnt" );

    // Load images
    // param - path to imgset file ()
    projectile_img = this.canvas.load_image("projectile.imgset/projectile");
    heart_img = this.canvas.load_image("heart-icon.imgset/heart");

    // Use preload_object() function from script_module api, to preload object (event on_preload_object_done() from script_module api is called, when object is preloaded)
    this.preload_object(NORMAL_INVADER_PATH);
    this.preload_object(SPECIAL_INVADER_PATH);
    this.preload_object(CANNON_PATH);

    // Jump to a location that was saved in game campos
    // 1. param - save name
    // 2. param - if date/time is contained in the location file, apply it
    // returns - true if the location was found
    this.load_location("minigame_location", false);

    // Create sensor of sphere shape
    // This sensor is used as trigger objects on end position (checks, if an invader got to end position)
    // 1. param - position
    // 2. param - rotation
    // 3. param - radius
    this.create_sensor({x:-1941638.9198791014,y:5211794.163754703,z:3123587.0791307855}, {x: 0, y: 0, z: 0, w: 1}, 100);

    // Function needs to return bool
    return true;
}

// Similiar to simulation_step from vehicle_script api (called 60 times per second)
function before_simulation_step(dtns, ns_sim)
{
    // check if location is ready
    // returns - true if last loaded location is ready
    is_loc_ready = this.is_location_ready();

    // Return if the location is not loaded
    if(!is_loc_ready)
    {
        return;
    }
    
    // Set time, if not yet set (to not start the minigame at night...)
    if(!time_set)
    {
        // Set the time of the day
        // 1. param - day of the year
        // 2. param - time of the day in seconds
        // 3. param - if true set UTC, false set solar time for current location
        this.set_time(1, 36000, false);
        
        time_set = true;
    }

    if(cannon_loaded && cannon_entered === false)
    {
        if (cannon == null) 
        {
            // Spawn cannon
            let cannon_start_pos = {x:-1941613.3096685943,y:5212175.574482916,z:3123162.568296084};
            let cannon_start_rot = {x:-0.49617794156074524,y:-0.09487587213516235,z:0.14400778710842133,w:0.8509216904640198};
            cannon = this.create_static_instance(CANNON_PATH, cannon_start_pos, cannon_start_rot, 2);
        }
        else
        {
            // Get cannon geometry
            cannon_geom = cannon.get_geomob();
            // Get cannon bones
            if(cannon_geom != null)
            {
                barrel_tip_id = cannon_geom.get_joint("barrel_tip")
                cannon_pitch_id = cannon_geom.get_joint("cannon_pitch");
            
                // Enter cannon
                cannon.enter();
                
                cannon_entered = true;
            }
        }

    }

    // Do not continue until cannon is loaded
    if(!cannon)
    {
        return;
    }
    
    // Do not continue on game over
    if(game_over == true)
    {
        return;
    }
    // When all lives are lost, set game over
    else if(hearts_remaining <= 0)
    {
        game_over = true;
        this.on_game_over();

        return;
    }

    // Calculate dt
    let dt = dtns/1000000000
    // Increment the timer by the delta time
    timer += dt;

    // Load projectile after some time
    if( timer - loaded_time > PROJECTILE_LOAD_TIME && projectiles_loaded < MAX_PROJECTILES )
    {
        loaded_time = timer;
        projectiles_loaded ++;
    }

    // Check if invaders got to end position (if they triggered the sensor on end position)
    // Method "get_tiggered_sensors" returns an array of objects, that triggered the sensor in current frame
    trigger_sensor = this.get_tiggered_sensors();

    // If an invader triggered the sensor, loop through the invaders to find the triggered one, based on it's id
    if(trigger_sensor.length > 0)
    {
        for (let t = trigger_sensor.length - 1; t >= 0; t--)
        {
            // Get the id of the invader, that triggered the sensor
            let triggered_id = trigger_sensor[t].trigger_entity_id;

            // Get the invader object from the triggered id
            let triggered_invader_obj = this.get_object(triggered_id);

            // Check if object was successfully received
            if (triggered_invader_obj != null)
            {
                // Loop through the invaders
                outer_loop: for (let i = invader_array.length - 1; i >= 0; i--)
                {
                    let invaders_line = invader_array[i].invaders;

                    for (let j = invaders_line.length - 1; j >= 0; j--)
                    {
                        let invader = invaders_line[j];

                        // Skip the invaders, that are removed
                        if (invader.removed == true)
                        {
                            continue;
                        }

                        // Check if the invader is the one, that triggered the sensor, by comparing the id
                        if (triggered_invader_obj.id() == invader.instance.id())
                        {
                            // Set the "removed" flag
                            invader.removed = true;
                            // Remove the invader
                            // param - entity id
                            this.remove_object(triggered_id);

                            // If all invaders in the current line have been removed (the entire invader line is empty), remove the line element (invader line) from the invader_array
                            if (invaders_line.every(inv => inv.removed))
                            {
                                invader_array.splice(i, 1);
                            }

                            // Decrease hp
                            hearts_remaining--;

                            // If the triggered invader was found, break out of the outer loop, to not continue looping through the arrays
                            break outer_loop;
                        }
                    }
                }
            }
        }
    }

        // Spawn multiple lines of invaders each second
    if(lvl_stage <= 2 ||  spawning == true && timer - last_spawn_time > 1 )
    {
        // Initialize invader line object
        let invader_line = {};

        // Calculate number of stronger invaders in group
        let stronger_invaders_in_group = Math.floor((lvl_stage-2) * 1.2);

        // Calculate number of stronger invaders per line
        let stronger_invaders_per_line = this.calculate_stronger_invaders_per_line(spawned_lines, invader_lines_in_group, stronger_invaders_in_group);

        // Calculate total number of invaders
        let total_invaders = invader_lines_in_group * invaders_in_line;

        // If the number of stronger invaders in group is bigger than number of normal invaders, add additional invader line to the group
        if(stronger_invaders_in_group > (total_invaders / 2))
        {
            invader_lines_in_group++;
        }

        // Start spawning stronger invaders on the 3. wave
        stronger_invaders_per_line = lvl_stage >= 3 ? stronger_invaders_per_line : 0;

        // Spawn lines of normal and stronger invaders
        invader_line = this.spawn_invader_line(NORMAL_INVADER_PATH, SPECIAL_INVADER_PATH, stronger_invaders_per_line);

        // This is made to spawn first 2 groups of invaders closer, when the game starts
        if(lvl_stage === 1)
        {
            // As the invaders are spawned closer, the spawn time needs to be adjusted, so that they follow their route correcty (based on their timer and speed)
            invader_line.spawn_time -= 26 * (1 / INVADERS_SPEED);
        }
        else if(lvl_stage === 2)
        {
            // Spawn 2 lines of normal invaders in the group
            invader_lines_in_group = 2;

             // Adjust the spawn time
            if(spawned_lines == 0)
            {
                invader_line.spawn_time -=  13 * (1 / INVADERS_SPEED);
            }
            else if(spawned_lines == 1)
            {
                invader_line.spawn_time -= 11.5 * (1 / INVADERS_SPEED);
            }
        }

        // Return if the invader_line is empty
        if(Object.keys(invader_line).length == 0)
        {
            return;
        }

        // Push invader line into invader array
        invader_array.push(invader_line);
        // Store time, when the latest invader line has been spawned
        last_spawn_time = timer;
        // Increment number of spawned line
        spawned_lines++;

        // Stop spawning, when desired number of invader lines has been spawned
        if(spawned_lines === invader_lines_in_group)
        {
            // First 2 waves are spawned instantly, therefore they ignore the spawning rules
            if(lvl_stage >= 3)
            {
                spawning = false;
            }

            // Reset number of already spawned lines
            spawned_lines = 0;

            // Increase difficulty
            lvl_stage++;
        }
    }
    // Start spawning additional group of invaders after certain time
    else if(spawning == false && timer - last_spawn_time > 12)
    {
        spawning = true;
    }


    // Get projectiles, that hit something
    // Method "get_landed_projectiles" returns an array of landed projectiles (tracers) in current frame
    let landed_projectile = this.get_landed_projectiles();

    // If projectile hit something
    if (landed_projectile.length > 0) {
        // Loop through each hit
        landed_projectile.forEach(projectile => {
            // Get hit position
            //Note: When an object is hit, then the position returned, is relative to model space. Otherwise returns world position
            let hit_pos = projectile.pos;

            // Get ID of hit object (earth returns -1)
            let hit_id = projectile.hitid;

            // Check if the hit object was not earth
            if (hit_id != -1)
            {
                // Loop through each invader line
                outer_loop: for (let i = 0; i < invader_array.length; i++)
                {
                    // Loop through each invader in the current line
                    for (let j = 0; j < invader_array[i].invaders.length; j++)
                    {
                        // Check if the hit ID matches the current invader's ID
                        if (hit_id == invader_array[i].invaders[j].instance.id())
                        {
                            // Current invader
                            let invader = invader_array[i].invaders[j];
                            // Get invaders ECEF position
                            let invader_pos = invader.instance.get_pos();

                            // Calculate ECEF hit position, by adding relative hit position to invaders ECEF position (ECEF position needed for creating explosion...)
                            // Note: returned relative hit position is oriented in world space, so there is no need to change rotation
                            let hit_pos_ecef = {x: (invader_pos.x + hit_pos.x ) , y: (invader_pos.y + hit_pos.y ), z: (invader_pos.z + hit_pos.z) }

                            //Create explosion
                            this.create_explosion(hit_pos_ecef, true);

                            // Decrease the number of invader lives
                            invader.lives -= 1;

                            // If invader has no lives remaining
                            if (invader.lives <= 0)
                            {
                                // Increase the destroyed invaders count
                                invaders_destroyed++;

                                // Add score, based on the invader type
                                if(invader.special == true)
                                {
                                    score += 5;
                                }
                                else
                                {
                                    score += 1;
                                }

                                // Mark the hit invader as removed
                                invader.removed = true;
                                // Remove the hit invader from the scene
                                invader.instance.remove_from_scene();

                                // Check if all invaders in the current line are "removed", if so, remove the invader line from the array
                                if (invader_array[i].invaders.every(inv => inv.removed))
                                {
                                    invader_array.splice(i, 1);
                                }
                            }

                            // Break out of both loops
                            break outer_loop;
                        }
                    }
                }
            }
            else
            {
                //Create explosion
                this.create_explosion(hit_pos, false);
            }
        });
    }

    //Check if invader model was loaded
    if(!invader_01_loaded)
    {
        return;
    }

    // Check if there are any invaders in the array
    if (invader_array.length > 0 )
    {
        // Loop through each invader line in reverse order
        for (let i = invader_array.length - 1; i >= 0; i--)
        {
            // Calculate the time since the invader line was spawned
            let line_timer = timer - invader_array[i].spawn_time;

            // Get the array of invaders in the current line
            let invaders_line = invader_array[i].invaders;

            // Loop through each invader in the current line in reverse order
            for (let index = invaders_line.length - 1; index >= 0; index--)
            {
                let invader = invaders_line[index];

                // If the invader is marked as removed, skip to the next invader
                if(invader.removed == true)
                {
                    continue;
                }

                // Get the current position and rotation of the invader
                let invader_pos = invader.instance.get_pos();
                let invader_rot = invader.instance.get_rot();

                let distance_traveled = line_timer * INVADERS_SPEED;

                // Define the local movement vector
                let local_movement = {x: 0, y: INVADERS_SPEED * dt * 30, z: 0};

                // Define the axis of rotation and initialize the angle
                let axis = {x:0,y:0,z:-1};
                let angle = 0;

                // Adjust the angle and the movement speed when at the first curve
                if (distance_traveled > 1 && distance_traveled < 8.7)
                {
                    angle = 0.0015 * INVADERS_SPEED;
                    // Calculate the position for the invader by applying the offset based on the index (i) of the invader
                    // The offset is dependent on the index to ensure that invaders with higher indices move more,
                    // allowing for correct movement and spacing when turning
                    local_movement = {x:0, y:(0.4 + index/22) * INVADERS_SPEED, z:0};
                }
                // Adjust the angle and the movement speed when at the end curve
                else if (distance_traveled > 45.5 && distance_traveled < 70)
                {
                    local_movement = {x:0, y:(0.4 + index/40) * INVADERS_SPEED , z:0};
                    angle = 0.00082 * INVADERS_SPEED;
                }

                // Create a quaternion for the rotation based on the axis and angle
                let rot = Quaternion.setFromAxisAngle(axis,angle);
                // Multiply the current invader rotation by the new rotation quaternion
                let final_rot = Quaternion.mul(invader_rot, rot)

                // Set the new rotation for the invader
                invader.instance.set_rot(final_rot);

                // Rotate the local movement vector by the invader rotation quaternion
                let world_movement = this.rotate_vector_by_quaternion(local_movement, invader_rot);

                // Calculate the new position for the invader by applying the world movement and index coefficient
                let new_position = {
                    x: invader_pos.x + world_movement.x * dt/(1/60),
                    y: invader_pos.y + world_movement.y * dt/(1/60),
                    z: invader_pos.z + world_movement.z * dt/(1/60)
                };

                // Move the invader
               invader.instance.set_pos(new_position);
            };
        };
    }

    // Update the remaining invaders count
    instances_count = invader_array.reduce((total, line) => {
        let activeInvaders = line.invaders.filter(invader => !invader.removed);
        return total + activeInvaders.length;
    }, 0);
}



// Called each frame, similiar to "update_frame" from vehicle_script
function visual_update( dtrender, dtsim, dtinterpolate )
{
    if(!cannon)
    {
        // Fill the whole screen with black rectangle, until the cannon is loaded
        // 1. param - start of the rectangle on x axis
        // 2. param - start of the rectangle on y axis
        // 3. param - width of the rectangle
        // 4. param - height of the rectangle
        this.canvas.fill_rect(0, 0, screen_size.x, screen_size.y, {r: 0,g: 0,b: 0, a:255});

        // Draw loading text on screen, until the cannon is loaded
        // 1. param - font
        // 2. param - position on x axis (bottom left corner of the text)
        // 3. param - position on y axis (bottom left corner of the text)
        // 4. param - text
        // 5. param - color
        this.canvas.draw_text(font, screen_center.x - 70, screen_center.y - 7, "Loading minigame... ", {r: 255,g: 255,b: 255, a:255});

        return;
    }

    // Return if the location is not loaded
    if(!is_loc_ready)
    {
        return;
    }

    this.draw_crosshair();

    // Draw background behind the score
    this.canvas.fill_rect(20, screen_size.y - 20, 210 + 40 * score.toString().length, -50, {r: 15,g: 0,b: 60, a:255});

    // Draw score on the screen
    this.canvas.draw_text(font_xolonium_50_bold, 30, screen_size.y - 67, "score: "+ score, {r: 0,g: 150,b: 150, a:255});
    // Draw number of remaining invaders on the screen
    this.canvas.draw_text(font, screen_size.x - 250, screen_size.y - 67, "Invaders remaining: "+ instances_count, {r: 0,g: 0,b: 0, a:255});

    //Draw projectiles on the screen
    // 1. param - Image ID
    // 2. param - position on x axis (bottom left corner of the image)
    // 3. param - position on y axis (bottom left corner of the image)
    // 4. param - width
    // 5. param - height
    // 6. param - color
    for( let p = 0; p < MAX_PROJECTILES; p++)
    {
        this.canvas.draw_image_wh(projectile_img, 30,20 + (40 * p), 110, 30,{r:255, g: p < projectiles_loaded ? 255 : 0, b: p < projectiles_loaded ? 255 : 0, a: 255});
    }


    //Draw lives on the screen
    for( let h = 0; h < MAX_HEARTS; h++)
    {
        this.canvas.draw_image_wh(heart_img, screen_size.x - 50 - (40 * h) ,20, 40, 40,{r:255, g:255, b:255, a: h < hearts_remaining ? 255 : 100});
    }
}

// Invoked after the final camera position in the frame is known
function on_final_camera(pos, rot)
{
    // Return if the location is not loaded
    if(!is_loc_ready)
    {
        return;
    }

    // Check if cannon and it's geometry were successfully loaded
    if(cannon == null || cannon_geom == null)
    {
        return;
    }

    // Get cannon inverse rotaion
    let cannon_rot_inv = Quaternion.inverse(cannon_geom.get_rot());

    // Multiply the cannon inverse rotation with the camera rotation
    rot = Quaternion.mul(cannon_rot_inv, rot);

    // Get axis
    let axis = this.quaternion_to_axis(rot);

    // Calculate angle
    let angle = 2 * Math.acos(rot.w);

    // Rotate cannon
    cannon_geom.rotate_joint_orig(cannon_pitch_id, angle, {x:axis.x, y:axis.y, z:axis.z});
}

