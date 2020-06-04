bl_info = {
    "name": "Outerra Bone control element",
    "author": "Outerra",
    "version": (0, 1, 0),
    "blender": (2, 80, 0),
    "location": "Properties > Bone",
    "description": "Helper functionality for control elements defnition on bones.",
    "tracker_url": "",
    "category": "Properties"
}

import bpy
from bpy.app.handlers import persistent

control_types = (
    ('button', 'Button', 'Button.'),
    ('slider', 'Slider', 'Slider.'),
    ('knob', 'Knob', 'Knob.'),
    ('lever', 'Lever', 'Lever.'),
    ('shaft', 'Shaft', 'Shaft.'),
)

transform_types = (
    ('rX', 'rotate X', ''),
    ('rY', 'rotate Y', ''),
    ('rZ', 'rotate Z', ''),
    ('tX', 'translate X', ''),
    ('tY', 'translate Y', ''),
    ('tZ', 'translate Z', ''),
)

class DefaultAction:
    vel = 1.0
    acc = 1000.0
    cen = 0.0
    min = -1.0
    max = 1.0
    steps = 1
    channels = 0

TYPE_PROPERTY_NAME = "ot_controlbone_type"
ACTION_PROPERTY_NAME = "ot_controlbone_action"
ANIMATION_PROPERTY_NAME = "ot_controlbone_animation"


def clear_properties(bone):
    if TYPE_PROPERTY_NAME in bone:
        del bone[TYPE_PROPERTY_NAME]

    if ACTION_PROPERTY_NAME in bone:
        del bone[ACTION_PROPERTY_NAME]

    if ANIMATION_PROPERTY_NAME in bone:
        del bone[ANIMATION_PROPERTY_NAME]

def deserialize_type(props, bone):
    if TYPE_PROPERTY_NAME not in bone:
        return

    item = bone[TYPE_PROPERTY_NAME]
    if (type(item) != str):
        return

    values = [x.strip() for x in item.split(';')]

    for n in range(len(values), 2):
        values.append("")

    el_type = values[0]
    if any(el_type == i[0] for i in control_types):
        props.type = el_type
    else:
        props.type = control_types[0][0];

    props.handles = values[1]

def serialize_type(props, bone):
    if not props.use_control:
        return

    val_str = [""] * 2
    val_str[0] = props.type
    val_str[1] = props.handles;
    bone[TYPE_PROPERTY_NAME] = ";".join(val_str)


def deserialize_action(props, bone):
    if ACTION_PROPERTY_NAME not in bone:
        props.use_vel = False
        props.use_acc = False
        props.use_cen = False
        props.use_min = False
        props.use_max = False
        props.use_steps = False
        props.use_channels = False
        return

    item = bone[ACTION_PROPERTY_NAME]
    if (type(item) != str):
        return

    values = [x.strip() for x in item.split(';')]

    for n in range(len(values), 8):
        values.append("")

    if len(values[0]) > 0:
        props.action = values[0];

    try:
        props.vel = float(values[1])
        props.use_vel = True
    except:
        props.use_vel = False

    try:
        props.acc = float(values[2])
        props.use_acc = True
    except:
        props.use_acc = False

    try:
        props.cen = float(values[3])
        props.use_cen = True
    except:
        props.use_cen = False

    try:
        props.min = float(values[4])
        props.use_min = True
    except:
        props.use_min = False

    try:
        props.max = float(values[5])
        props.use_max = True
    except:
        props.use_max = False

    try:
        props.steps = int(values[6])
        props.use_steps = True
    except:
        props.use_steps = False

    try:
        props.channels = int(values[7])
        props.use_channels = True
    except:
        props.use_channels = False

def serialize_action(props, bone):
    if not props.use_control:
        return

    used = False;
    val_str = [""] * 8

    val_str[0] = props.action;

    if props.use_vel:
        val_str[1] = "{:1.5f}".format(props.vel).rstrip('0').rstrip('.');
        used = True

    if props.use_acc:
        val_str[2] = "{:1.5f}".format(props.acc).rstrip('0').rstrip('.');
        used = True

    if props.use_cen:
        val_str[3] = "{:1.5f}".format(props.cen).rstrip('0').rstrip('.');
        used = True

    if props.use_min:
        val_str[4] = "{:1.5f}".format(props.min).rstrip('0').rstrip('.');
        used = True

    if props.use_max:
        val_str[5] = "{:1.5f}".format(props.max).rstrip('0').rstrip('.');
        used = True

    if props.use_steps:
        val_str[6] = str(props.steps);
        used = True

    if props.use_channels:
        val_str[7] = str(props.channels);
        used = True

    if used:
        bone[ACTION_PROPERTY_NAME] = ";".join(val_str)
    else:
        if ACTION_PROPERTY_NAME in bone:
            del bone[ACTION_PROPERTY_NAME]


def deserialize_animation(props, bone):
    if ANIMATION_PROPERTY_NAME not in bone:
        props.use_trans_min = False
        props.use_trans_max = False
        return

    item = bone[ANIMATION_PROPERTY_NAME]
    if (type(item) != str):
        props.use_trans_min = False
        props.use_trans_max = False
        return

    values = [x.strip() for x in item.split(';')]

    for n in range(len(values), 3):
        values.append("")

    if len(values[0]) > 0:
        if any(values[0] == i[0] for i in transform_types):
            props.trans_type = values[0]
        else:
            props.trans_type = transform_types[0][0];

    try:
        props.trans_min = float(values[1])
        props.use_trans_min = True
    except:
        props.use_trans_min = False

    try:
        props.trans_max = float(values[2])
        props.use_trans_max = True
    except:
        props.use_trans_max = False

def serialize_animation(props, bone):
    if not props.use_control:
        return

    used = False
    val_str = [""] * 3

    val_str[0] = props.trans_type

    if props.use_trans_min:
        val_str[1] = "{:1.5f}".format(props.trans_min).rstrip('0').rstrip('.');
        used = True

    if props.use_trans_max:
        val_str[2] = "{:1.5f}".format(props.trans_max).rstrip('0').rstrip('.');
        used = True

    if used:
        bone[ANIMATION_PROPERTY_NAME] = ";".join(val_str)
    else:
        if ANIMATION_PROPERTY_NAME in bone:
            del bone[ANIMATION_PROPERTY_NAME]


def update_type(self, context):
    props = context.scene.ot_control_element_prop
    bone = get_active_bone()

    if bone is None:
        return

    serialize_type(props, bone)

def update_action(self, context):
    props = context.scene.ot_control_element_prop
    bone = get_active_bone()

    if bone is None:
        return

    serialize_action(props, bone)

def update_animation(self, context):
    props = context.scene.ot_control_element_prop
    bone = get_active_bone()

    if bone is None:
        return

    serialize_animation(props, bone)

def update_use_control(self, context):
    props = context.scene.ot_control_element_prop
    bone = get_active_bone()

    if bone is None:
        return

    if props.use_control:
        serialize_type(props, bone)
        serialize_action(props, bone)
        serialize_animation(props, bone)
    else:
        clear_properties(bone)


def get_active_bone():
    view_layer = bpy.context.view_layer
    active = view_layer.objects.active

    if active is not None and active.type == 'ARMATURE':
        return active.data.edit_bones.active
    else:
        return None


class OtControlElementSettings(bpy.types.PropertyGroup):
    use_control: bpy.props.BoolProperty(name="Use control element", description="serialize controlbone", default=False, update=update_use_control)

    type: bpy.props.EnumProperty(name="Type", description="Element type", items=control_types, update=update_type)
    handles: bpy.props.StringProperty(name="Handles", description="comma separated list of bone names", default="", maxlen=1024, update=update_type)


    action: bpy.props.StringProperty(name="Action", description="iomap action", default="", maxlen=1024, update=update_action)

    use_vel: bpy.props.BoolProperty(name="Use velocity", description="serialize this field", default=False, update=update_action)
    vel: bpy.props.FloatProperty(name="Velocity", description="velocity", default=DefaultAction.vel, min=0.01, max=1000, update=update_action)

    use_acc: bpy.props.BoolProperty(name="Use acceleration", description="serialize this field", default=False, update=update_action)
    acc: bpy.props.FloatProperty(name="Acceleration", description="acceleration", default=DefaultAction.acc, min=0.01, max=1000, update=update_action)

    use_cen: bpy.props.BoolProperty(name="Use centering", description="serialize this field", default=False, update=update_action)
    cen: bpy.props.FloatProperty(name="Centering", description="centering", default=DefaultAction.cen, min=0.0, max=1000, update=update_action)

    use_min: bpy.props.BoolProperty(name="Use minimum", description="serialize this field", default=False, update=update_action)
    min: bpy.props.FloatProperty(name="Minimum", description="minimum value", default=DefaultAction.min, min=-6.0, max=0.0, update=update_action)

    use_max: bpy.props.BoolProperty(name="Use maximum", description="serialize this field", default=False, update=update_action)
    max: bpy.props.FloatProperty(name="Maximum", description="maximum value", default=DefaultAction.max, min=0.0, max=6.0, update=update_action)

    use_steps: bpy.props.BoolProperty(name="Use steps", description="serialize this field", default=False, update=update_action)
    steps: bpy.props.IntProperty(name="Step count", description="count of steps between 0 and 1", default=DefaultAction.steps, min=1, max=255, update=update_action)

    use_channels: bpy.props.BoolProperty(name="Use channels", description="serialize this field", default=False, update=update_action)
    channels: bpy.props.IntProperty(name="Channel count", description="number of extra channels supported by the handler", default=DefaultAction.channels, min=0, max=7, update=update_action)


    trans_type: bpy.props.EnumProperty(name="Type", description="type of transformation of bone to animate", items=transform_types, update=update_animation)

    use_trans_min: bpy.props.BoolProperty(name="Use transform min", description="serialize this field", default=False, update=update_animation)
    trans_min: bpy.props.FloatProperty(name="Minimum", description="minimum value transformation could be", default=-1.0, min=-1000, max=1000, update=update_animation)

    use_trans_max: bpy.props.BoolProperty(name="Use transform max", description="serialize this field", default=False, update=update_animation)
    trans_max: bpy.props.FloatProperty(name="Maximum", description="maximum value transformation could be", default=1.0, min=-1000, max=1000, update=update_animation)

class OtControlElementPanel(bpy.types.Panel):
    bl_idname = "PANEL_PT_OtControlElement"
    bl_label = "Outerra Control Element"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "bone"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(cls, context):
        return (context.object is not None)

    def draw_header(self, context):
        props = context.scene.ot_control_element_prop
        self.layout.prop(props, "use_control", text="")

    def draw(self, context):
        def draw_type(layout, props, name, enabled):
            row = layout.row()
            row.enabled = enabled
            row.prop(props, name)

        def draw_property(layout, props, name, check_name, check_prop):
            row = layout.row()
            wrap = row.row()
            wrap.enabled = check_prop
            wrap.prop(props, name)
            row.prop(props, check_name, text="")

        layout = self.layout

        if context.mode != 'EDIT_ARMATURE':
            return

        props = context.scene.ot_control_element_prop

        layout.enabled = props.use_control

        layout.prop(props, "type")
        layout.prop(props, "handles")

        layout.separator()
        row = layout.grid_flow()

        col = row.box()
        col.label(text="Action data")
        used = props.use_vel or props.use_acc or props.use_cen or props.use_min or props.use_max or props.use_steps or props.use_channels
        draw_type(col, props, "action", used)
        draw_property(col, props, "vel", "use_vel", props.use_vel)
        draw_property(col, props, "acc", "use_acc", props.use_acc)
        draw_property(col, props, "cen", "use_cen", props.use_cen)
        draw_property(col, props, "min", "use_min", props.use_min)
        draw_property(col, props, "max", "use_max", props.use_max)
        draw_property(col, props, "steps", "use_steps", props.use_steps)
        draw_property(col, props, "channels", "use_channels", props.use_channels)

        col = row.box()
        col.label(text="Animation data")
        used = props.use_trans_min or props.use_trans_max
        draw_type(col, props, "trans_type", used)
        draw_property(col, props, "trans_min", "use_trans_min", props.use_trans_min)
        draw_property(col, props, "trans_max", "use_trans_max", props.use_trans_max)


class OtControlElementListPanel(bpy.types.Panel):
    bl_idname = "PANEL_PT_OtControlElementListPanel"
    bl_label = "Control elements"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "OT tools"
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        layout = self.layout
        view_layer = bpy.context.view_layer
        active = view_layer.objects.active
        
        if active is not None and active.type == 'ARMATURE':
            bones = active.data.edit_bones
            for bone in bones:
                if TYPE_PROPERTY_NAME in bone:
                    type = bone[TYPE_PROPERTY_NAME]
                    values = [x.strip() for x in type.split(';')]
                    layout.label(text=bone.name + " (" + values[0] + ")")


ot_current_bone = None
@persistent
def check_active_bone():
    global ot_current_bone

    bone = get_active_bone()

    if bone is None:
        return 0.3

    props = bpy.context.scene.ot_control_element_prop

    if ot_current_bone != bone:
        deserialize_type(props, bone)
        deserialize_action(props, bone)
        deserialize_animation(props, bone)
        props.use_control = TYPE_PROPERTY_NAME in bone or ACTION_PROPERTY_NAME in bone or ANIMATION_PROPERTY_NAME in bone
        ot_current_bone = bone
    return 0.3


classes = (OtControlElementSettings, OtControlElementPanel, OtControlElementListPanel)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.Scene.ot_control_element_prop = bpy.props.PointerProperty(type=OtControlElementSettings)

    bpy.app.timers.register(check_active_bone, first_interval=0, persistent=True)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

    bpy.app.timers.unregister(check_active_bone)

    del bpy.types.Scene.ot_control_element_prop


if __name__ == "__main__":
    register()