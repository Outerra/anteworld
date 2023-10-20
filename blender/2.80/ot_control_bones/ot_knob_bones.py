bl_info = {
    "name": "Outerra Bone knob element",
    "author": "Outerra",
    "version": (0, 2, 0),
    "blender": (2, 80, 0),
    "location": "Properties > Bone",
    "description": "Helper functionality for knob elements defnition on bones.",
    "tracker_url": "",
    "category": "Properties"
}


import bpy
from bpy.app.handlers import persistent

CONTROL_TYPES = (
    ('button', 'Button', 'Button.'),
    ('slider', 'Slider', 'Slider.'),
    ('knob', 'Knob', 'Knob.'),
    ('lever', 'Lever', 'Lever.'),
    ('stick', 'Stick', 'Stick.'),
)

ANIMATION_TYPES = (
    ('rotateX', 'Rotate X', 'Rotate around bone X axis.'),
    ('rotateY', 'Rotate Y', 'Rotate around bone Y axis.'),
    ('rotateZ', 'Rotate Z', 'Rotate around bone Z axis.'),
    ('translateX', 'Translate X', 'Translate in direction of bone X axis.'),
    ('translateY', 'Translate Y', 'Translate in direction of bone Y axis.'),
    ('translateZ', 'Translate Z', 'Translate in direction of bone Z axis.'),
)

class DEFAULT_VALUES:
    vel = 1.0
    acc = 1000.0
    center = 0.0
    min = -1.0
    max = 1.0
    positions = 0
    channel = 0
    
CONTROL_TYPES_DEFAULTS = (
    {'min' : 0, 'max' : 1, 'vel' : 1000, 'acc' : 1000, 'center' : 1000, 'positions' : 2, 'anim_type' : 'translateY', 'anim_min' : 0, 'anim_max' : 1}, #button
    {'min' : 0, 'max' : 1, 'vel' : 1000, 'acc' : 1000, 'center' : 1000, 'positions' : 2, 'anim_type' : 'translateX', 'anim_min' : 0, 'anim_max' : 1}, #slider
    {'min' : 0, 'max' : 1, 'vel' : 1000, 'acc' : 1000, 'center' : 1000, 'positions' : 2, 'anim_type' : 'rotateY', 'anim_min' : 0, 'anim_max' : 90}, #knob
    {'min' : 0, 'max' : 1, 'vel' : 1000, 'acc' : 1000, 'center' : 1000, 'positions' : 2, 'anim_type' : 'rotateX', 'anim_min' : 0, 'anim_max' : 45}, #lever
    {'min' : 0, 'max' : 1, 'vel' : 1000, 'acc' : 1000, 'center' : 1000, 'positions' : 3, 'anim_type' : 'rotateX', 'anim_min' : -15, 'anim_max' : 15}, #stick
)
    
TYPE_PROPERTY_NAME = "ot_knob_type"
HANDLES_PROPERTY_NAME = "ot_knob_handles"

ACTION_NAME_PROPERTY_NAME = "ot_knob_action_name"
ACTION_VELOCITY_PROPERTY_NAME = "ot_knob_action_velocity"
ACTION_ACCELERATION_PROPERTY_NAME = "ot_knob_action_acceleration"
ACTION_CENTERING_PROPERTY_NAME = "ot_knob_action_centering"
ACTION_MIN_VALUE_PROPERTY_NAME = "ot_knob_action_min_value"
ACTION_MAX_VALUE_PROPERTY_NAME = "ot_knob_action_max_value"
ACTION_POSITIONS_PROPERTY_NAME = "ot_knob_action_positions"
ACTION_CHANNEL_PROPERTY_NAME = "ot_knob_action_channel"

ANIMATION_TYPE_PROPERTY_NAME = "ot_knob_anim_type"
ANIMATION_MIN_VALUE_PROPERTY_NAME = "ot_knob_anim_min"
ANIMATION_MAX_VALUE_PROPERTY_NAME = "ot_knob_anim_max"

ALL_PROPERTY_NAMES = {
    TYPE_PROPERTY_NAME,
    HANDLES_PROPERTY_NAME,
    ACTION_NAME_PROPERTY_NAME,
    ACTION_VELOCITY_PROPERTY_NAME,
    ACTION_ACCELERATION_PROPERTY_NAME,
    ACTION_CENTERING_PROPERTY_NAME,
    ACTION_MIN_VALUE_PROPERTY_NAME,
    ACTION_MAX_VALUE_PROPERTY_NAME,
    ACTION_POSITIONS_PROPERTY_NAME,
    ACTION_CHANNEL_PROPERTY_NAME,
    ANIMATION_TYPE_PROPERTY_NAME,
    ANIMATION_MIN_VALUE_PROPERTY_NAME,
    ANIMATION_MAX_VALUE_PROPERTY_NAME
}

s_last_active_bone = None

def get_bone(context):
    if context.mode == 'EDIT_ARMATURE' and len(context.selected_bones) > 0:
        return context.selected_bones[0]
    return None


def on_use_control_change(self, context):
    global s_last_active_bone;
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    #print("(" + str(threading.get_ident()) + ") use callback, bone: " + (s_last_active_bone.name if s_last_active_bone != None else "None") + " -> " + (bone.name if bone != None else "None"))
    if bone is None:
        s_last_active_bone = None
        return
    
    if props.use_control:
        """bone[TYPE_PROPERTY_NAME] = props.type
        if props.use_handles:
            bone[HANDLES_PROPERTY_NAME] = props.handles

        if props.use_action:
            bone[ACTION_NAME_PROPERTY_NAME] = props.action
        if props.use_vel:
            print("(" + str(threading.get_ident()) + ")    vel changed: " + str(bone[ACTION_VELOCITY_PROPERTY_NAME]) + " -> " + str(props.vel))
            bone[ACTION_VELOCITY_PROPERTY_NAME] = props.vel
        if props.use_acc:
            bone[ACTION_ACCELERATION_PROPERTY_NAME] = props.acc
        if props.use_center:
            bone[ACTION_CENTERING_PROPERTY_NAME] = props.center
        if props.use_min:
            bone[ACTION_MIN_VALUE_PROPERTY_NAME] = props.min
        if props.use_max:
            bone[ACTION_MAX_VALUE_PROPERTY_NAME] = props.max
        if props.use_positions:
            bone[ACTION_POSITIONS_PROPERTY_NAME] = props.positions
        if props.use_channel:
            bone[ACTION_CHANNEL_PROPERTY_NAME] = props.channel
                
        if props.use_anim:
            bone[ANIMATION_TYPE_PROPERTY_NAME] = props.anim_type
        if props.use_anim_min:
            bone[ANIMATION_MIN_VALUE_PROPERTY_NAME] = props.anim_min
        if props.use_anim_max:
            bone[ANIMATION_MAX_VALUE_PROPERTY_NAME] = props.use_anim_max"""
    else:
        for prop_name in ALL_PROPERTY_NAMES:
            bone.pop(prop_name, None)
            
def on_type_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    bone[TYPE_PROPERTY_NAME] = props.type
        
# handles callbacks
def on_handles_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_handles:
        bone[HANDLES_PROPERTY_NAME] = props.handles
    else:
        bone.pop(HANDLES_PROPERTY_NAME, None)
        
# action callbacks
def on_action_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
        
    if props.use_action:
        bone[ACTION_NAME_PROPERTY_NAME] = props.action
    else:
        bone.pop(ACTION_NAME_PROPERTY_NAME, None)
        
# velocity callback
def on_vel_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_vel:
        bone[ACTION_VELOCITY_PROPERTY_NAME] = props.vel
    else:
        bone.pop(ACTION_VELOCITY_PROPERTY_NAME, None)
            
# acceleration callback
def on_acc_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
        
    if props.use_acc:
        bone[ACTION_ACCELERATION_PROPERTY_NAME] = props.acc
    else:
        bone.pop(ACTION_ACCELERATION_PROPERTY_NAME, None)
            
# center callback
def on_center_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_center:
        bone[ACTION_CENTERING_PROPERTY_NAME] = props.center
    else:
        bone.pop(ACTION_CENTERING_PROPERTY_NAME, None)
            
# minval callback
def on_min_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_min:
        bone[ACTION_MIN_VALUE_PROPERTY_NAME] = props.min
    else:
        bone.pop(ACTION_MIN_VALUE_PROPERTY_NAME, None)
            
# maxval callback
def on_max_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_max:
        bone[ACTION_MAX_VALUE_PROPERTY_NAME] = props.max
    else:
        bone.pop(ACTION_MAX_VALUE_PROPERTY_NAME, None)
            
# positions callback
def on_positions_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_positions:
        bone[ACTION_POSITIONS_PROPERTY_NAME] = props.positions
    else:
        bone.pop(ACTION_POSITIONS_PROPERTY_NAME, None)
            
# channel callback
def on_channel_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_channel:
        bone[ACTION_CHANNEL_PROPERTY_NAME] = props.channel
    else:
        bone.pop(ACTION_CHANNEL_PROPERTY_NAME, None)
            
# anim type callback
def on_anim_type_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_anim:
        bone[ANIMATION_TYPE_PROPERTY_NAME] = props.anim_type
    else:
        bone.pop(ANIMATION_TYPE_PROPERTY_NAME, None)
            
# anim min callback
def on_anim_min_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_anim_min:
        bone[ANIMATION_MIN_VALUE_PROPERTY_NAME] = props.anim_min
    else:
        bone.pop(ANIMATION_MIN_VALUE_PROPERTY_NAME, None)
            
# anim max callback
def on_anim_max_change(self, context):
    bone = get_bone(context)
    props = context.scene.ot_control_element_prop
    
    if props.use_anim_max:
        bone[ANIMATION_MAX_VALUE_PROPERTY_NAME] = props.anim_max
    else:
        bone.pop(ANIMATION_MAX_VALUE_PROPERTY_NAME, None)


# properties used because of unable to show string custom property as enum
class OT_control_element_properties(bpy.types.PropertyGroup):
    use_control: bpy.props.BoolProperty(name="Use knob", description="serialize controlbone", default=False, update=on_use_control_change)
    type: bpy.props.EnumProperty(name="Hand pose type", description="Element type", items=CONTROL_TYPES, update=on_type_change)
    
    use_handles: bpy.props.BoolProperty(name="Use handles", description="serialize this field", default=False, update=on_handles_change)
    handles: bpy.props.StringProperty(name="Handles", description="Comma separated list of bone names", default="", maxlen=1024, update=on_handles_change)


    use_action: bpy.props.BoolProperty(name="Use action", description="serialize this field", default=False, update=on_action_change)
    action: bpy.props.StringProperty(name="Action", description="IOmap action", default="", maxlen=1024, update=on_action_change)

    use_vel: bpy.props.BoolProperty(name="Use velocity", description="serialize this field", default=False, update=on_vel_change)
    vel: bpy.props.FloatProperty(name="Velocity", description="Velocity", default=DEFAULT_VALUES.vel, min=0.01, max=1000, update=on_vel_change)

    use_acc: bpy.props.BoolProperty(name="Use acceleration", description="serialize this field", default=False, update=on_acc_change)
    acc: bpy.props.FloatProperty(name="Acceleration", description="Acceleration", default=DEFAULT_VALUES.acc, min=0.01, max=1000, update=on_acc_change)

    use_center: bpy.props.BoolProperty(name="Use centering", description="serialize this field", default=False, update=on_center_change)
    center: bpy.props.FloatProperty(name="Centering", description="Centering", default=DEFAULT_VALUES.center, min=0.0, max=1000, update=on_center_change)

    use_min: bpy.props.BoolProperty(name="Use minimum", description="serialize this field", default=False, update=on_min_change)
    min: bpy.props.FloatProperty(name="Minimum", description="Minimum value", default=DEFAULT_VALUES.min, min=-6.0, max=0.0, update=on_min_change)

    use_max: bpy.props.BoolProperty(name="Use maximum", description="serialize this field", default=False, update=on_max_change)
    max: bpy.props.FloatProperty(name="Maximum", description="Maximum value", default=DEFAULT_VALUES.max, min=0.0, max=6.0, update=on_max_change)

    use_positions: bpy.props.BoolProperty(name="Use positions", description="serialize this field", default=False, update=on_positions_change)
    positions: bpy.props.IntProperty(name="Position count", description="Count of positions on <min,max> range", default=DEFAULT_VALUES.positions, min=0, max=255, update=on_positions_change)

    use_channel: bpy.props.BoolProperty(name="Use channel", description="serialize this field", default=False, update=on_channel_change)
    channel: bpy.props.IntProperty(name="Channel number", description="Channel number, must be supported by the handler", default=DEFAULT_VALUES.channel, min=0, max=7, update=on_channel_change)


    use_anim: bpy.props.BoolProperty(name="Use animation", description="serialize this group", default=False, update=on_anim_type_change)
    anim_type: bpy.props.EnumProperty(name="Type", description="Type of transformation of bone to animate", items=ANIMATION_TYPES, update=on_anim_type_change)

    use_anim_min: bpy.props.BoolProperty(name="Use animation min", description="serialize this field", default=False, update=on_anim_min_change)
    anim_min: bpy.props.FloatProperty(name="Minimum", description="Minimum value transformation can be", default=-1.0, min=-1000, max=1000, update=on_anim_min_change)

    use_anim_max: bpy.props.BoolProperty(name="Use animation max", description="serialize this field", default=False, update=on_anim_max_change)
    anim_max: bpy.props.FloatProperty(name="Maximum", description="Maximum value transformation can be", default=1.0, min=-1000, max=1000, update=on_anim_max_change)
    
    def clear(self):
        self.handles = ''
        self.action = ''
        self.vel = DEFAULT_VALUES.vel
        self.acc = DEFAULT_VALUES.acc
        self.cen = DEFAULT_VALUES.cen
        self.min = DEFAULT_VALUES.min
        self.max = DEFAULT_VALUES.max
        self.positions = DEFAULT_VALUES.positions
        self.channel = DEFAULT_VALUES.channel
        
    def has_bone_props(self, bone):
        if TYPE_PROPERTY_NAME not in bone:
            return False
        
        has_some = False
        has_some = has_some or (HANDLES_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_NAME_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_VELOCITY_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_ACCELERATION_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_CENTERING_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_MIN_VALUE_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_MAX_VALUE_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_POSITIONS_PROPERTY_NAME in bone)
        has_some = has_some or (ACTION_CHANNEL_PROPERTY_NAME in bone)
        has_some = has_some or (ANIMATION_TYPE_PROPERTY_NAME in bone)
        has_some = has_some or (ANIMATION_MIN_VALUE_PROPERTY_NAME in bone)
        has_some = has_some or (ANIMATION_MAX_VALUE_PROPERTY_NAME in bone)
        return has_some
        
    def set_to_bone(self, bone):
        if not self.use_control:
            return
        
        bone[TYPE_PROPERTY_NAME] = self.type
        
        if self.use_handles:
            bone[HANDLES_PROPERTY_NAME] = self.handles
            
        if self.use_action:
            bone[ACTION_NAME_PROPERTY_NAME] = self.action
            
        if self.use_vel:
            bone[ACTION_VELOCITY_PROPERTY_NAME] = self.vel
            
        if self.use_acc:
            bone[ACTION_ACCELERATION_PROPERTY_NAME] = self.acc
            
        if self.use_center:
            bone[ACTION_CENTERING_PROPERTY_NAME] = self.center
            
        if self.use_min:
            bone[ACTION_MIN_VALUE_PROPERTY_NAME] = self.min
            
        if self.use_max:
            bone[ACTION_MAX_VALUE_PROPERTY_NAME] = self.max
            
        if self.use_positions:
            bone[ACTION_POSITIONS_PROPERTY_NAME] = self.positions
            
        if self.use_channel:
            bone[ACTION_CHANNEL_PROPERTY_NAME] = self.channel
            
        if self.use_anim:
            bone[ANIMATION_TYPE_PROPERTY_NAME] = self.anim_type
            
        if self.use_anim_min:
            bone[ANIMATION_MIN_VALUE_PROPERTY_NAME] = self.anim_min
            
        if self.use_anim_max:
            bone[ANIMATION_MAX_VALUE_PROPERTY_NAME] = self.anim_max
        
    def set_from_bone(self, bone):
        if TYPE_PROPERTY_NAME in bone:
            if any(bone[TYPE_PROPERTY_NAME] == i[0] for i in CONTROL_TYPES):
                self.type = bone[TYPE_PROPERTY_NAME]
                self.use_control = True
            else:
                self.type = CONTROL_TYPES[0][0];
                self.use_control = False
        else:
            self.use_control = False
            
        if not self.use_control:
            return
                
        if HANDLES_PROPERTY_NAME in bone:
            self.handles = bone[HANDLES_PROPERTY_NAME]
            self.use_handles = True
        else:
            self.use_handles = False
            
            
        if ACTION_NAME_PROPERTY_NAME in bone:
            self.action = bone[ACTION_NAME_PROPERTY_NAME]
            self.use_action = True
        else:
            self.use_action = False
            
        if ACTION_VELOCITY_PROPERTY_NAME in bone:
            self.vel = bone[ACTION_VELOCITY_PROPERTY_NAME]
            self.use_vel = True
        else:
            self.use_vel = False
            
        if ACTION_ACCELERATION_PROPERTY_NAME in bone:
            self.acc = bone[ACTION_ACCELERATION_PROPERTY_NAME]
            self.use_acc = True
        else:
            self.use_acc = False
            
        if ACTION_CENTERING_PROPERTY_NAME in bone:
            self.center = bone[ACTION_CENTERING_PROPERTY_NAME]
            self.use_center = True
        else:
            self.use_center = False
            
        if ACTION_MIN_VALUE_PROPERTY_NAME in bone:
            self.min = bone[ACTION_MIN_VALUE_PROPERTY_NAME]
            self.use_min = True
        else:
            self.use_min = False
            
        if ACTION_MAX_VALUE_PROPERTY_NAME in bone:
            self.max = bone[ACTION_MAX_VALUE_PROPERTY_NAME]
            self.use_max = True
        else:
            self.use_max = False
            
        if ACTION_POSITIONS_PROPERTY_NAME in bone:
            self.positions = bone[ACTION_POSITIONS_PROPERTY_NAME]
            self.use_positions = True
        else:
            self.use_positions = False
            
        if ACTION_CHANNEL_PROPERTY_NAME in bone:
            self.channel = bone[ACTION_CHANNEL_PROPERTY_NAME]
            self.use_channel = True
        else:
            self.use_channel = False
            
            
        if ANIMATION_TYPE_PROPERTY_NAME in bone:
            if any(bone[ANIMATION_TYPE_PROPERTY_NAME] == i[0] for i in ANIMATION_TYPES):
                self.anim_type = bone[ANIMATION_TYPE_PROPERTY_NAME]
                self.use_anim = True
            else:
                self.anim_type = ANIMATION_TYPES[0][0];
                self.use_anim = False
        else:
            self.use_anim = False
                
        if ANIMATION_MIN_VALUE_PROPERTY_NAME in bone:
            self.anim_min = bone[ANIMATION_MIN_VALUE_PROPERTY_NAME]
            self.use_anim_min = True
        else:
            self.use_anim_min = False
            
        if ANIMATION_MAX_VALUE_PROPERTY_NAME in bone:
            self.anim_max = bone[ANIMATION_MAX_VALUE_PROPERTY_NAME]
            self.use_anim_max = True
        else:
            self.use_anim_max = False
            

# popup window for initial bone knob properties
class OT_WM_OT_create_knob(bpy.types.Operator):
    """Create new knob definition"""
    bl_idname = "wm.create_knob"
    bl_label = "Create knob definition"
    
    OPTIONS = (
        ('none', 'Choose type', 'Choose from types below.'),
        ('push_button', 'Push button', 'Simple tactile button.'),                                   #one  state     button pose
        ('latching_button', 'Latching button', 'Button maintain state until pressed again.'),       #two  states    button pose
        ('pull_push_button', 'Pull-push button', 'Usually active on pull and inactive on push.'),   #two  states    button pose?
        ('rocker_switch', 'Rocker switch', 'Common on/off two state switch.'),                      #two  states    button pose
        ('toggle_switch', 'Toggle switch', 'Small lever switch.'),                                  #two  states    slider pose
        ('slide_switch', 'Slide switch', 'Switch that slides on axis.'),                            #mult states    slider pose
        ('rotary_switch', 'Rotary switch', 'Ideal for multiple state switching.'),                  #mult states    knob   pose
        ('lever', 'Big lever', 'Usually used for large machines.'),                                 #mult states    lever  pose
        #special controls under
        ('gear_stick', 'Gear stick', 'Vehicle transmission lever.'),                                #mult states    stick  pose
    )
    
    DEFAULTS = (
        {'type': 'button', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 1000, 'positions': 2, 'anim_type': 'translateY', 'anim_min': 0, 'anim_max': 1}, #push_button
        {'type': 'button', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'translateY', 'anim_min': 0, 'anim_max': 1}, #latching_button
        {'type': 'button', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'translateY', 'anim_min': 0, 'anim_max': -1}, #pull_push_button
        {'type': 'button', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'rotateX', 'anim_min': 0, 'anim_max': 10}, #rocker_switch
        {'type': 'slider', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'rotateX', 'anim_min': 0, 'anim_max': 10}, #toggle_switch
        {'type': 'slider', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'translateX', 'anim_min': 0, 'anim_max': 1}, #slide_switch
        {'type': 'knob', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'rotateY', 'anim_min': 0, 'anim_max': 90}, #rotary_switch
        {'type': 'lever', 'min': 0, 'max': 1, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 2, 'anim_type': 'rotateX', 'anim_min': 0, 'anim_max': 45}, #lever
        #special controls under
        {'type': 'stick', 'min': 0, 'max': 2, 'vel': 1000, 'acc': 1000, 'center': 0, 'positions': 3, 'anim_type': 'rotateX', 'anim_min': -15, 'anim_max': 15}, #gear_stick
    )
    
    def on_type_change(self, context):
        if self.type == 'slide_switch' or self.type == 'rotary_switch' or self.type == 'lever' or self.type == 'gear_stick':
            self.positions = 3 if self.type == 'gear_stick' else 2
    
    object_name: bpy.props.StringProperty(name="Select object name", default="")
    bone_name: bpy.props.StringProperty(name="Select bone name", default="")
    
    type: bpy.props.EnumProperty(name="Type", description="Knob type", items=OPTIONS, update=on_type_change)
    positions: bpy.props.IntProperty(name="Position count", description="Count of positions on <min,max> range", default=DEFAULT_VALUES.positions, min=0, max=255,)

    def execute(self, context):
        ctx = context;
        props = ctx.scene.ot_control_element_prop;
        obj = bpy.data.objects[self.object_name];
        if obj is None:
            return {'CANCELLED'}
        bone = obj.data.edit_bones[self.bone_name]
        if bone is None:
            return {'CANCELLED'}
        
        def handle_option_properties(props, option_defaults, knob_defaults):
            props.type = option_defaults['type'];
            props.use_control = True
            props.use_action = False
            props.use_min = option_defaults['min'] != knob_defaults['min']
            props.min = option_defaults['min']
            props.use_max = option_defaults['max'] != knob_defaults['max']
            props.max = option_defaults['max']
            props.use_vel = option_defaults['vel'] != knob_defaults['vel']
            props.vel = option_defaults['vel']
            props.use_acc = option_defaults['acc'] != knob_defaults['acc']
            props.acc = option_defaults['acc']
            props.use_center = option_defaults['center'] != knob_defaults['center']
            props.center = option_defaults['center']
            props.use_positions = self.positions != knob_defaults['positions']
            props.positions = self.positions
            props.use_anim = option_defaults['anim_type'] != knob_defaults['anim_type']
            props.anim_type = option_defaults['anim_type']
            props.use_anim_min = option_defaults['anim_min'] != knob_defaults['anim_min']
            props.anim_min = option_defaults['anim_min']
            props.use_anim_max = option_defaults['anim_max'] != knob_defaults['anim_max']
            props.anim_max = option_defaults['anim_max']
        
        for idx, opt in enumerate(self.OPTIONS):
            if idx > 0 and self.type == self.OPTIONS[idx][0]:
                try:
                    knob_def_idx = [i[0] for i in CONTROL_TYPES].index(self.DEFAULTS[idx - 1]['type'])
                    handle_option_properties(props, self.DEFAULTS[idx - 1], CONTROL_TYPES_DEFAULTS[knob_def_idx])
                    props.set_to_bone(bone)
                except:
                    pass
        
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)
    
    def draw(self, context):
        self.layout.prop(self, "type");
        if self.type == 'slide_switch' or self.type == 'rotary_switch' or self.type == 'lever' or self.type == 'gear_stick':
            self.layout.prop(self, "positions");


# popup window for delete bone knob properties
class OT_WM_OT_delete_knob(bpy.types.Operator):
    """Delete knob definition"""
    bl_idname = "wm.delete_knob"
    bl_label = "Do you really want to do that?"
    
    object_name: bpy.props.StringProperty(name="Select object name", default="")
    bone_name: bpy.props.StringProperty(name="Select bone name", default="")
    
    def execute(self, context):
        ctx = context;
        props = ctx.scene.ot_control_element_prop;
        obj = bpy.data.objects[self.object_name];
        if obj is None:
            return {'CANCELLED'}
        bone = obj.data.edit_bones[self.bone_name]
        if bone is None:
            return {'CANCELLED'}
        
        for prop_name in ALL_PROPERTY_NAMES:
            bone.pop(prop_name, None)
        props.use_control = False
        
        return {'FINISHED'}

    def invoke(self, context, event):
        return context.window_manager.invoke_confirm(self, event)


# additional property editor for bone knob properties
class OT_PT_control_element_panel(bpy.types.Panel):
    bl_label = "Outerra Knob Element"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_options = {'DEFAULT_CLOSED'}
    
    @classmethod
    def poll(cls, context):
        if context.space_data.context != 'BONE':
            return False
        if context.mode == 'OBJECT':
            return True
        if context.mode == 'EDIT_ARMATURE' and context.selected_bones is not None:
            return True
        return False
    
    def draw(self, context):
        if context.mode == 'OBJECT':
            self.layout.label(text="This plugin is usable only in edit mode.")
            return
        if len(context.selected_bones) != 1:
            self.layout.label(text="Multiple selection is not supported for now.")
            return

        def draw_property(layout, props, name, check_prop, check_prop_name, prop_label = None):
            row = layout.row()
            wrap = row.row()
            wrap.enabled = check_prop
            wrap.prop(props, name, text=prop_label)
            row.prop(props, check_prop_name, text="")

        layout = self.layout
        bone = get_bone(context)
        props = context.scene.ot_control_element_prop
        
        if not props.use_control:
            op = layout.operator(OT_WM_OT_create_knob.bl_idname, text="Initialize knob")
            op.object_name = bpy.context.view_layer.objects.active.name
            op.bone_name = bpy.context.view_layer.objects.active.data.edit_bones.active.name
            return
            
        layout.prop(props, "type")
        draw_property(layout, props, "handles", props.use_handles, "use_handles")
        
        layout.separator()
        row = layout.grid_flow()
        
        col = row.box()
        col.label(text="Action data")
        
        draw_property(col, props, "action", props.use_action, "use_action")
        draw_property(col, props, "vel", props.use_vel, "use_vel")
        draw_property(col, props, "acc", props.use_acc, "use_acc")
        draw_property(col, props, "center", props.use_center, "use_center")
        draw_property(col, props, "min", props.use_min, "use_min")
        draw_property(col, props, "max", props.use_max, "use_max")
        draw_property(col, props, "positions", props.use_positions, "use_positions")
        draw_property(col, props, "channel", props.use_channel, "use_channel")
        
        col = row.box()
        col.label(text="Animation data")
        
        draw_property(col, props, "anim_type", props.use_anim, "use_anim");
        
        is_rot = props.anim_type == ANIMATION_TYPES[0][0] or props.anim_type == ANIMATION_TYPES[1][0] or props.anim_type == ANIMATION_TYPES[2][0]
        anim_min_label = 'Minimum ' + ('(deg)' if is_rot else '(cm)')
        anim_max_label = 'Maximum ' + ('(deg)' if is_rot else '(cm)')
        draw_property(col, props, "anim_min", props.use_anim_min, "use_anim_min", anim_min_label)
        draw_property(col, props, "anim_max", props.use_anim_max, "use_anim_max", anim_max_label)

        op = layout.operator(OT_WM_OT_delete_knob.bl_idname, text="Delete knob")
        op.object_name = bpy.context.view_layer.objects.active.name
        op.bone_name = bpy.context.view_layer.objects.active.data.edit_bones.active.name

class OT_OT_list_item_convert_operator(bpy.types.Operator):
    """Convert all old knob definitions to new format"""
    bl_idname = "list_item_convert.operator"
    bl_label = "Convert item from list"
    bl_options = {'REGISTER'}
    
    object_name: bpy.props.StringProperty(name="Select object name", default="")
    
    @classmethod
    def poll(cls, context):
        return True
    
    def execute(self, context):
        obj = bpy.data.objects[self.object_name];
        if obj is not None:
            convert_old_properties_object(obj)
        return {'FINISHED'}
    

class OT_OT_list_item_operator(bpy.types.Operator):
    """Select bone"""
    bl_idname = "list_item.operator"
    bl_label = "Select item from list"
    bl_options = {'REGISTER'}
    
    object_name: bpy.props.StringProperty(name="Select object name", default="")
    bone_name: bpy.props.StringProperty(name="Select bone name", default="")
    
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        props = context.scene.ot_control_element_prop
        
        obj = bpy.data.objects[self.object_name];
        if obj is not None:
            for o in bpy.data.objects:
                o.select_set(False)
                
            obj.select_set(state=True)
            bpy.context.view_layer.objects.active = obj;
            if self.bone_name != "":
                if context.mode.startswith('EDIT'):
                    bpy.ops.armature.select_all(action='DESELECT')
                    bones = obj.data.edit_bones
                    bones.active = bones[self.bone_name]
                else:
                    for b in obj.data.bones:
                        b.select = False
                    bones = obj.data.bones
                    bones.active = bones[self.bone_name]
        return {'FINISHED'}

# 3D view editor with list of defined knobs
class OT_PT_control_element_list_panel(bpy.types.Panel):
    bl_label = "Bone knob definitions"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "OT tools"
    bl_options = {'DEFAULT_CLOSED'}
    
    def draw(self, context):
        props = context.scene.ot_control_element_prop
        
        layout = self.layout
        view_layer = bpy.context.view_layer
        
        for obj in view_layer.objects:
            if obj.type == 'ARMATURE':
                bones = obj.data.edit_bones if context.mode == 'EDIT_ARMATURE' else obj.data.bones
                has_old_knobs = False
                for bone in bones:
                    if "ot_controlbone_type" in bone:
                        layout.label(text="Old knobs properties detected")
                        label = "Convert old knobs"
                        op = layout.operator("list_item_convert.operator", text=label)
                        op.object_name = obj.name
                        has_old_knobs = True
                        break
                    
                list_layout = layout.column()
                list_layout.enabled = not has_old_knobs
                
                if has_old_knobs:
                    for bone in bones:
                        if "ot_controlbone_type" in bone:
                            label = bone.name + " (old format)"
                            list_layout.label(text=label)
                        elif TYPE_PROPERTY_NAME in bone:
                            label = bone.name
                            list_layout.label(text=label)
                else:
                    for bone in bones:
                        if TYPE_PROPERTY_NAME in bone:
                            label = bone.name + " (" + bone[TYPE_PROPERTY_NAME] + ")"
                            op = list_layout.operator("list_item.operator", text=label)
                            op.object_name = obj.name
                            op.bone_name = bone.name
            #elif obj.name.startswith('BONE_'):
                #if TYPE_PROPERTY_NAME in obj:
                    #type = obj[TYPE_PROPERTY_NAME]
                    #values = [x.strip() for x in type.split(';')]
                    #label = obj.name + " (" + values[0] + ")"
                    #op = layout.operator("list_item.operator", text=label)
                    #op.object_name = obj.name
                    #op.bone_name = ''
        

classes = (
    OT_control_element_properties,
    OT_WM_OT_create_knob,
    OT_WM_OT_delete_knob,
    OT_PT_control_element_panel,
    OT_OT_list_item_convert_operator,
    OT_OT_list_item_operator,
    OT_PT_control_element_list_panel
)

# backwards compatibility fix of versions < 0.1.0
def convert_old_properties_bone(bone):
    if "ot_controlbone_type" in bone:
        item = bone["ot_controlbone_type"]
        values = [x.strip() for x in item.split(';')]
        if any(values[0] == i[0] for i in CONTROL_TYPES):
            bone[TYPE_PROPERTY_NAME] = values[0]
            if values[1]:
                bone[HANDLES_PROPERTY_NAME] = values[1]
            bone.pop("ot_controlbone_type", None)
        else:
            if values[0] == 'shaft':
                bone[TYPE_PROPERTY_NAME] = 'stick'
                if values[1]:
                    bone[HANDLES_PROPERTY_NAME] = values[1]
                bone.pop("ot_controlbone_type", None)
            else:
                print(bone.name + " could not be converted (dont have correct type)")
                return
    else:
        return

    knob_def_idx = [i[0] for i in CONTROL_TYPES].index(bone[TYPE_PROPERTY_NAME])
    knob_default_vals = CONTROL_TYPES_DEFAULTS[knob_def_idx]

    if "ot_controlbone_action" in bone:
        item = bone["ot_controlbone_action"]
        values = [x.strip() for x in item.split(';')]
        if len(values[0]) > 0:
            bone[ACTION_NAME_PROPERTY_NAME] = values[0];
        try:
            val = float(values[1])
            if val != knob_default_vals['vel']:
                bone[ACTION_VELOCITY_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = float(values[2])
            if val != knob_default_vals['acc']:
                bone[ACTION_ACCELERATION_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = float(values[3])
            if val != knob_default_vals['center']:
                bone[ACTION_CENTERING_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = float(values[4])
            if val != knob_default_vals['min']:
                bone[ACTION_MIN_VALUE_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = float(values[5])
            if val != knob_default_vals['max']:
                bone[ACTION_MAX_VALUE_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = int(values[6])
            if val != knob_default_vals['positions']:
                bone[ACTION_POSITIONS_PROPERTY_NAME] = val;
        except:
            pass
        try:
            val = int(values[7])
            bone[ACTION_CHANNEL_PROPERTY_NAME] = val;
        except:
            pass
        bone.pop("ot_controlbone_action", None)
        
    TRANSFORM_TYPES = ('rX', 'rY', 'rZ', 'tX', 'tY', 'tZ')
                
    if "ot_controlbone_animation" in bone:
        item = bone["ot_controlbone_animation"]
        values = [x.strip() for x in item.split(';')]
        if len(values[0]) > 0:
            try:
                idx = TRANSFORM_TYPES.index(values[0])
                if values[0] != knob_default_vals['anim_type']:
                    bone[ANIMATION_TYPE_PROPERTY_NAME] = ANIMATION_TYPES[idx][0]
                try:
                    val = float(values[1])
                    print(bone.name + ", anim_min: " + str(val) + " =?= " + str(knob_default_vals['anim_min']))
                    if val != knob_default_vals['anim_min']:
                        bone[ANIMATION_MIN_VALUE_PROPERTY_NAME] = val;
                except Exception as e:
                    print("2 " + str(e));
                    pass
                try:
                    val = float(values[2])
                    print(bone.name + ", anim_max: " + str(val) + " =?= " + str(knob_default_vals['anim_max']))
                    if val != knob_default_vals['anim_max']:
                        bone[ANIMATION_MAX_VALUE_PROPERTY_NAME] = val;
                except Exception as e:
                    print("1 " + str(e));
                    #pass
            except Exception as e:
                print("0 " + str(e));
                #pass
        bone.pop("ot_controlbone_animation", None)
        
def convert_old_properties_object(object):
    if object.type != 'ARMATURE':
        return
    
    bones = None
    if object.mode.startswith('EDIT'):
        bones = object.data.edit_bones
    else:
        bones = object.data.bones
        
    for bone in bones:
        convert_old_properties_bone(bone);

def convert_all_old_properties():
    for arm in bpy.data.armatures:
        for bone in arm.edit_bones:
            convert_old_properties_bone(bone)
                

# on blend file is opened
def load_handler(dummy):
    convert_all_old_properties()


#hack for update of enum property for UI
# 1) bone custom property (string type) cannot be used in UI as enum
# 2) when we use enum property for UI with callback update to custom property,
#    there is no way to register callback on manual custom property change to propagate to enum UI property
#    that is why we use this stupid timed function for update
@persistent
def check_active_bone():
    global s_last_active_bone
    bone = get_bone(bpy.context)
    props = bpy.context.scene.ot_control_element_prop
    
    if bone != s_last_active_bone:
        if bone is not None:
            props.set_from_bone(bone)
        s_last_active_bone = bone;
    #elif bone is not None:
    #    if not props.has_bone_props(bone):
    #        props.use_control = False
    
    return 0.5

#def msgbus_callback(*arg):
    #print("asd")
    # you can do something

# on addon enabled
def register():
    for cls in classes:
        bpy.utils.register_class(cls)
        
    #convert_all_old_properties()
    bpy.app.handlers.load_post.append(load_handler)
    bpy.types.Scene.ot_control_element_prop = bpy.props.PointerProperty(type=OT_control_element_properties)
    bpy.app.timers.register(check_active_bone, first_interval=0)
    
    """bpy.msgbus.subscribe_rna(
        key=bpy.context.object.data.edit_bones.active,
        owner=bpy,
        args=(bpy.context,),
        notify=msgbus_callback
        )"""#this doesnt work, but it looks closest to get bone selection change callback (if it actually works)
    
# on addon disabled
def unregister():
    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except:
            pass
    try:
        bpy.app.handlers.load_post.remove(load_handler)
    except:
        pass
    try:
        del bpy.types.Scene.ot_control_element_prop
    except:
        pass
    try:
        bpy.app.timers.unregister(check_active_bone)
    except:
        pass



# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    unregister()
    register()