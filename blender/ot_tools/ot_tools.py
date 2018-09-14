bl_info = {
    "name": "Outerra tools",
    "author": "Outerra",
    "version": (0, 0, 0),
    "blender": (2, 7, 0),
    "location": "Toolshelf",
    "description": "Tiny tools that can be usefull!",
    "tracker_url": "",
    "category": "3D View"}

import bpy
import colorsys
import bmesh
import random
import math
import binascii
import mathutils
import os


rnd_colors = -1

def gen_rnd_colors(count = 512):
    global rnd_colors
    random.seed(5489)
    rnd_colors = []
    for i in range(count):
        r = 0.0;
        g = 0.0;
        b = 0.0;
        while r + g + b < 0.1:
            r = random.random();
            g = random.random();
            b = random.random();
        
        rnd_colors.append((r,g,b));
    
    for i in range(count):
        rnd_colors[i] = list(rnd_colors[i])
    

class OTVtxGroupUI(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        layout.prop(data=item,property='ot_name',emboss=False,icon='GROUP_VERTEX',text='')
    
def set_vtx_group_name_callback(self,value):
    if self.name == value:
        return
    
    if value == '':
        value = self.default
    
    unique_name = value;    
    ncount = 0
    keys = bpy.context.object.ot_vertex_groups.keys()
    keys.remove(self.name)
    
    while keys.count(unique_name):
        ncount += 1;
        unique_name = "%s.%03d"%(value,ncount)
    
    self.name = unique_name
    set_ot_vertex_group_custom_prop(bpy.context.object)

def get_vtx_group_name_callback(self):
    return self.name
            
class OtVtxGroupItem(bpy.types.PropertyGroup):
    ot_name = bpy.props.StringProperty(name="ot_name", default="ot_vtx_group", set=set_vtx_group_name_callback, get = get_vtx_group_name_callback)
    ot_value = bpy.props.StringProperty(name="ot_color", default="000000")
    
    
class OtVertexGroups(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "OT vertex groups"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        
        row = col.row()
        row.operator("view3d.ot_add_new_vertex_group")
        
        row = col.row()
        row.operator("view3d.ot_remove_selected_vertex_group")
        
        row = col.row()
        row.operator("view3d.ot_add_selected_faces_to_active_vertex_group")
        
        row = col.row()
        row.operator("view3d.ot_clear_selected_faces_to_active_vertex_group")
        
        row = col.row()
        row.operator("view3d.ot_select_group_faces")
        
        obj = context.selected_objects[0]
        if hasattr(obj,"ot_vertex_groups"):
            row = col.row()
            row.template_list("OTVtxGroupUI","",obj, "ot_vertex_groups", obj, "ot_vertex_group_idx")
        

class OtAddNewVertexGroup(bpy.types.Operator):
    '''Add new vertex group'''
    bl_idname = "view3d.ot_add_new_vertex_group"
    bl_label = "Add vertex group"
    bl_options = {'REGISTER'}
        
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        return add_vertex_group(context)

class OtRemoveSelectedVertexGroup(bpy.types.Operator):
    '''Remove selected vertex group'''
    bl_idname = "view3d.ot_remove_selected_vertex_group"
    bl_label = "Remove vertex group"
    bl_options = {'REGISTER'}
        
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        return remove_vertex_group(context)

        
class OtAddSelectedFaces(bpy.types.Operator):
    '''Add selected faces to active vertex group'''
    bl_idname = "view3d.ot_add_selected_faces_to_active_vertex_group"
    bl_label = "Add faces"
    bl_options = {'REGISTER'}
        
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        return add_faces_to_vertex_group(context)

class OtRemoveSelectedFaces(bpy.types.Operator):
    '''Remove selected faces from any vertex group'''
    bl_idname = "view3d.ot_clear_selected_faces_to_active_vertex_group"
    bl_label = "Clear faces"
    bl_options = {'REGISTER'}
        
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        return remove_faces_from_vertex_group(context)

class OtSelectGroupFaces(bpy.types.Operator):
    '''Select faces of active vertex group'''
    bl_idname = "view3d.ot_select_group_faces"
    bl_label = "Select faces"
    bl_options = {'REGISTER'}
        
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        return select_faces_from_vertex_group(context)

        
class MirrorOnOff(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Mirror on/off"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.display_mirror_modifiers_on")
        row.operator("view3d.display_mirror_modifiers_off")
        
class ArrayOnOff(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Array on/off"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_display_array_modifiers_on")
        row.operator("view3d.ot_display_array_modifiers_off")
      
class SolidifyOnOff(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Solidify on/off"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_display_solidify_modifiers_on")
        row.operator("view3d.ot_display_solidify_modifiers_off")
    
class DisplayMirrorModifiersOn(bpy.types.Operator):
    '''Display all mirror modifiers'''
    bl_idname = "view3d.display_mirror_modifiers_on"
    bl_label = "On"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        mirror_modifiers_on(context)
        return {'FINISHED'}
    
class DisplayMirrorModifiersOff(bpy.types.Operator):
    '''Hide all mirror modifiers'''
    bl_idname = "view3d.display_mirror_modifiers_off"
    bl_label = "Off"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        mirror_modifiers_off(context)
        return {'FINISHED'}

class DisplayArrayModifiersOn(bpy.types.Operator):
    '''Display all array modifiers'''
    bl_idname = "view3d.ot_display_array_modifiers_on"
    bl_label = "On"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        array_modifiers_on(context)
        return {'FINISHED'}
    
class DisplayArrayModifiersOff(bpy.types.Operator):
    '''Hide all array modifiers'''
    bl_idname = "view3d.ot_display_array_modifiers_off"
    bl_label = "Off"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        array_modifiers_off(context)
        return {'FINISHED'}
    
class DisplaySolidifyModifiersOn(bpy.types.Operator):
    '''Display all solidify modifiers'''
    bl_idname = "view3d.ot_display_solidify_modifiers_on"
    bl_label = "On"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        solidify_modifiers_on(context)
        return {'FINISHED'}
    
class DisplaySolidifyModifiersOff(bpy.types.Operator):
    '''Hide all solidify modifiers'''
    bl_idname = "view3d.ot_display_solidify_modifiers_off"
    bl_label = "Off"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"

    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        solidify_modifiers_off(context)
        return {'FINISHED'}

class SingleBoneControls(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Single bone controls"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_add_control_properties")
        
class ObjectControls(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Object custom properties"
    
    def draw(self, context):
        layout = self.layout
        
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_add_lod_curve_properties")

class AddControlProperties(bpy.types.Operator):
    '''Add single bone control custom properties to all selected bones'''
    bl_idname = "view3d.ot_add_control_properties"
    bl_label = "Add control properties"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        add_control_properties_to_selected_bones(context)
        return {'FINISHED'}
        
class AddLodCurveProperties(bpy.types.Operator):
    '''Add lod curve custom properties to all selected object root nodes'''
    bl_idname = "view3d.ot_add_lod_curve_properties"
    bl_label = "Add lod curve properties"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        add_lod_curve_properties_to_selected_nodes(context)
        return {'FINISHED'}
    
class VegetationPanel(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Vegetations tools"
    
    def draw(self, context):
        layout = self.layout
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_compute_vegetation_weights")

class VegetationComputeWeights(bpy.types.Operator):
    '''Compute weights for vegetation on selected object'''
    bl_idname = "view3d.ot_compute_vegetation_weights"
    bl_label = "Compute weights"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        process_plant(context)
        return {'FINISHED'}

        
class OtherToolsPanel(bpy.types.Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    bl_label = "Other tools"
    
    def draw(self, context):
        layout = self.layout
        col = layout.column()
        col.alignment = 'EXPAND'
        row = col.row()
        row.operator("view3d.ot_save_selected_obj_names")
        
class OtherToolsSaveSelectedObjectNames(bpy.types.Operator):
    '''Export selected object names to file (names.txt)'''
    bl_idname = "view3d.ot_save_selected_obj_names"
    bl_label = "Export selected names"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    bl_category = "OT tools"
    
    @classmethod
    def poll(cls, context):
        return True
        
    def execute(self, context):
        save_selected_obj_names(context)
        return {'FINISHED'}        
       
'''    
///    
///        FUNCTIONS
///    
'''    

def init_ot_vertex_groups(obj):
    mesh = obj.data        
        
    bm = create_bmesh(obj)
    ot_lay_idx = 0;
    
    for key in bm.loops.layers.color.keys():
        if bm.loops.layers.color[key].name == 'ot_vertex_groups':
            break;
        ot_lay_idx += 1
    
    ot_lay = None
    if ot_lay_idx == len(bm.loops.layers.color.keys()):
        ot_lay = bm.loops.layers.color.new('ot_vertex_groups');
            
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        for l in f.loops:
            l[ot_lay] = [0,0,0]        
    
    free_bmesh(obj,bm)
    mesh.vertex_colors.active_index = ot_lay_idx
        
def add_vertex_group(context):
    if len(context.selected_objects) == 0 or context.selected_objects[0].type != 'MESH':
        return {'CANCELLED'}
        
    obj = context.selected_objects[0]
    m = obj.data
    ot_lay = m.vertex_colors.find('ot_vertex_groups')
    
    if not hasattr(obj,'ot_vertex_groups') or not hasattr(obj,'ot_vertex_group_idx') or ot_lay == -1:
        init_ot_vertex_groups(obj)
        
    create_new_group(obj,'ot_vertex_group')
    return {'FINISHED'}

def remove_vertex_group(context):
    if len(context.selected_objects) == 0 or context.selected_objects[0].type != 'MESH':
        return {'CANCELLED'}
        
    obj = context.selected_objects[0]
    
    if not hasattr(obj,'ot_vertex_group_idx') or not hasattr(obj,'ot_vertex_groups'):    
        return {'CANCELLED'}
        
    group_idx = obj.ot_vertex_group_idx;
    if group_idx >= len(obj.ot_vertex_groups):
        return {'CANCELLED'}
        
    color = list(binascii.unhexlify(obj.ot_vertex_groups[group_idx].ot_value))
    
    m = obj.data
    bm = create_bmesh(obj)
    ot_lay = bm.loops.layers.color['ot_vertex_groups'];
    
    for f in bm.faces:
        for l in f.loops:
            r = int(l[ot_lay].r*255)
            g = int(l[ot_lay].g*255)
            b = int(l[ot_lay].b*255)
            if r == color[0] and g == color[1] and b == color[2]:
                l[ot_lay] = [0,0,0]

    free_bmesh(obj,bm)
    obj.ot_vertex_groups.remove(group_idx)
    set_ot_vertex_group_custom_prop(obj)
    return {'FINISHED'}
        
def add_faces_to_vertex_group(context):
    if len(context.selected_objects) == 0 or context.selected_objects[0].type != 'MESH':
        return {'CANCELLED'}
        
    obj = context.selected_objects[0]
    
    if not hasattr(obj,'ot_vertex_group_idx') or not hasattr(obj,'ot_vertex_groups'):    
        return {'CANCELLED'}
        
    group_idx = obj.ot_vertex_group_idx;
    if group_idx >= len(obj.ot_vertex_groups):
        return {'CANCELLED'}
        
    color = list(binascii.unhexlify(obj.ot_vertex_groups[group_idx].ot_value))
    color[0] /= 255; 
    color[1] /= 255;
    color[2] /= 255;
    
    cur_mode = obj.mode;
    
    if cur_mode != 'EDIT' and cur_mode != 'VERTEX_PAINT':
        print('Object is not in edit or paint mode!')
        return {'CANCELLED'}
    
    m = obj.data
    bm = create_bmesh(obj)
    ot_lay = bm.loops.layers.color['ot_vertex_groups'];
    
    for f in bm.faces:
        if f.select:
            for l in f.loops:
                l[ot_lay] = color;
                
    free_bmesh(obj,bm)
    set_ot_vertex_group_custom_prop(obj)
    return {'FINISHED'}
        
def remove_faces_from_vertex_group(context):
    if len(context.selected_objects) == 0 or context.selected_objects[0].type != 'MESH':
        return {'CANCELLED'}
        
    obj = context.selected_objects[0]
    
    if not hasattr(obj,'ot_vertex_group_idx') or not hasattr(obj,'ot_vertex_groups'):    
        return {'CANCELLED'}
        
    cur_mode = obj.mode;    
    
    if cur_mode != 'EDIT' and cur_mode != 'VERTEX_PAINT':
        print('Object is not in edit or paint mode!')
        return {'CANCELLED'}

    m = obj.data
    bm = create_bmesh(obj)
    ot_lay = bm.loops.layers.color['ot_vertex_groups'];
        
    for f in bm.faces:
        if f.select:
            for l in f.loops:
                l[ot_lay] = [0,0,0];
                
    free_bmesh(obj,bm)
    set_ot_vertex_group_custom_prop(obj)
    return {'FINISHED'}

def select_faces_from_vertex_group(context):
    if len(context.selected_objects) == 0 or context.selected_objects[0].type != 'MESH':
        return {'CANCELLED'}
        
    obj = context.selected_objects[0]
    
    if not hasattr(obj,'ot_vertex_group_idx') or not hasattr(obj,'ot_vertex_groups'):    
        return {'CANCELLED'}
        
    group_idx = obj.ot_vertex_group_idx;
    if group_idx >= len(obj.ot_vertex_groups):
        return {'CANCELLED'}
        
    color = list(binascii.unhexlify(obj.ot_vertex_groups[group_idx].ot_value))
    
    m = obj.data
    bm = create_bmesh(obj)
    ot_lay = bm.loops.layers.color['ot_vertex_groups'];
    
    for f in bm.faces:
        for l in f.loops:
            r = int(l[ot_lay].r*255)
            g = int(l[ot_lay].g*255)
            b = int(l[ot_lay].b*255)
            if r == color[0] and g == color[1] and b == color[2]:
                f.select = True
            else:
                f.select = False
            break
                
    free_bmesh(obj,bm)
    
    return {'FINISHED'}
    
def create_new_group(obj,name):
    global rnd_colors
    if rnd_colors == -1:
        gen_rnd_colors()
    
        
    color = rnd_colors[obj.ot_vertex_group_counter];
    
    r = int(color[0]*255);
    g = int(color[1]*255);
    b = int(color[2]*255);
        
    ngroup = obj.ot_vertex_groups.add()
    ngroup.ot_name = name;
    ngroup.ot_value = "%02x%02x%02x"%(r,g,b)
    obj.ot_vertex_group_counter += 1


def set_ot_vertex_group_custom_prop(obj):
    bm = create_bmesh(obj)
    
    ot_lay = bm.loops.layers.color['ot_vertex_groups']
    result = '';
    for grp in obj.ot_vertex_groups:
        color = list(binascii.unhexlify(grp.ot_value))
        for f in bm.faces:
            if len(f.loops):
                r = int(f.loops[0][ot_lay].r * 255)
                g = int(f.loops[0][ot_lay].g * 255)
                b = int(f.loops[0][ot_lay].b * 255)
                if r == color[0] and g == color[1] and b == color[2]:
                    if result == '':
                        result = "%s:%s"%(grp.ot_name,grp.ot_value)
                    else:
                        result += ";%s:%s"%(grp.ot_name,grp.ot_value)
                    break;
                    
    obj.data['ot_vertex_groups_prop'] = result;                
    free_bmesh(obj,bm)
    

def create_bmesh(obj):
    if obj.mode == 'EDIT':
        return bmesh.from_edit_mesh(obj.data)
    else:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        return bm
        
def free_bmesh(obj,bm):
    if obj.mode == 'EDIT':
        bmesh.update_edit_mesh(obj.data)
    else:
        bm.to_mesh(obj.data)
        obj.data.update()
        bm.free()
        
def mirror_modifiers_on(context):
    sel = bpy.context.selected_objects 
    
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'MIRROR':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'MIRROR':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    
def mirror_modifiers_off(context):
    sel = bpy.context.selected_objects 
      
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'MIRROR':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'MIRROR':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False
                    
def array_modifiers_on(context):
    sel = bpy.context.selected_objects 
    
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'ARRAY':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'ARRAY':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    
def array_modifiers_off(context):
    sel = bpy.context.selected_objects 
      
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'ARRAY':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'ARRAY':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False
                    
def solidify_modifiers_on(context):
    sel = bpy.context.selected_objects 
    
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'SOLIDIFY':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'SOLIDIFY':
                    m.show_render = True
                    m.show_viewport = True
                    m.show_in_editmode = True
                    m.show_on_cage = True
    
def solidify_modifiers_off(context):
    sel = bpy.context.selected_objects 
      
    if not(sel):  
        for o in bpy.data.objects:
            for m in o.modifiers:
                if m.type == 'SOLIDIFY':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False
    else:
        for o in sel:
            for m in o.modifiers:
                if m.type == 'SOLIDIFY':
                    m.show_render = False
                    m.show_viewport = False
                    m.show_in_editmode = False
                    m.show_on_cage = False

def add_control_properties_to_selected_bones(context):
    if not context.selected_objects:
        return
    
    obj = context.object
    
    if obj.type != 'ARMATURE' or obj.mode != 'EDIT':
        return
     
    for b in context.selected_bones:
        b['single_bone_control_type'] = 0
        b['single_bone_vel'] = 0.0
        b['single_bone_acc'] = 0.0
        b['single_bone_cen'] = 0.0
        b['single_bone_min'] = 0.0
        b['single_bone_max'] = 0.0
        b['single_bone_step'] = 0.0
        b['single_bone_channel'] = 0
        b['single_bone_handles'] = ''
        b['single_bone_action'] = ''
        
def add_lod_curve_properties_to_selected_nodes(context):
    if not context.selected_objects:
        return
    
    obj = context.object
    
    if obj.type != 'EMPTY':
        return
     
    for o in context.selected_objects:
        o['lod_curve'] = 850.0
        o['lod_curve1'] = 300.0
        
# VEGETATION FUNCTIONS 
##########################################################################################

def delete_all_color_layers():
    for o in bpy.data.objects:
        if o.type != 'MESH':
            continue
        bm = bmesh.new()
        bm.from_mesh(o.data)
        for cl_key in bm.loops.layers.color.keys():
            cl = bm.loops.layers.color[cl_key]
            bm.loops.layers.color.remove(cl)
        bm.to_mesh(o.data)
        bm.free()
        
def clear_all_vegetation_color_layers(obj):
    if obj.type != 'MESH':
        return
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    ids_cl = None;
    lon_w_cl = None;
    lat_w_cl = None;
    
    if "veg_ids" in bm.loops.layers.color.keys():   
        ids_cl = bm.loops.layers.color["veg_ids"]
    else:
        ids_cl = bm.loops.layers.color.new("veg_ids")
        
    if "veg_lon_weights" in bm.loops.layers.color.keys():   
        lon_w_cl = bm.loops.layers.color["veg_lon_weights"]
    else:
        lon_w_cl = bm.loops.layers.color.new("veg_lon_weights")
        
    if "veg_lat_weights" in bm.loops.layers.color.keys():   
        lat_w_cl = bm.loops.layers.color["veg_lat_weights"]
    else:
        lat_w_cl = bm.loops.layers.color.new("veg_lat_weights")
    
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        for l in f.loops:
            l[ids_cl] = (0.0,0.0,0.0)
            l[lon_w_cl] = (0.0,0.0,0.0)
            l[lat_w_cl] = (0.0,0.0,0.0)
            
    bm.to_mesh(obj.data)
    bm.free()
    
    for ch in obj.children:
        clear_all_vegetation_color_layers(ch)
    
def show_color_layer(lay):
    root_obj = bpy.context.object;
    for o in root_obj.children:
        if o.type != 'MESH':
            continue
        bpy.context.scene.objects.active = o
        bpy.ops.object.mode_set(mode='VERTEX_PAINT')
        act_idx = 0
        for vc in o.data.vertex_colors.keys():
            if vc == lay:
                break;
            act_idx += 1;
        o.data.vertex_colors.active_index = act_idx
                
def all_to_object_mode():
    root_obj = bpy.context.object
    for o in root_obj.children:
        if o.type != 'MESH' and o.hide:
            continue
        bpy.context.scene.objects.active = o
        bpy.ops.object.mode_set(mode='OBJECT')
    
        
def process_plant(context):
    root_obj = context.object
    if root_obj.type != 'EMPTY':
        return
    
    for ch in root_obj.children:
        clear_all_vegetation_color_layers(ch)
    
    b_id = 0;
    for ch0 in root_obj.children:
        if len(ch0.children) == 0:
            continue
        b_id = process_branch(ch0,b_id,0)
        ch_b_id = 0
        for ch1 in ch0.children:
            if len(ch1.children) == 0:
                continue
            ch_b_id = process_branch(ch1,ch_b_id,1)
            
    leaf_id = 1;
    for ch in root_obj.children:
        if len(ch.children) != 0:
            continue
        process_leaf(ch,leaf_id)
        leaf_id += 1

def generate_envelope(ch,step,steps_count,lat_data):
    bm = bmesh.new()
    for idx in range(0,steps_count):
        v = (0,lat_data[idx][0],step*idx)
        bm.verts.new(v)   
    for idx in range(0,steps_count):
        v = (0,lat_data[steps_count-idx-1][1],step*(steps_count-idx-1))
        bm.verts.new(v)
    bm.verts.ensure_lookup_table()
    bm.faces.new(bm.verts)
    bm.faces.ensure_lookup_table()
    m = bpy.data.meshes.new('leaf_envelope')
    bm.to_mesh(m)
    bm.free()
    o = bpy.data.objects.new('leaf_envelope',m)
    o.matrix_basis = ch.matrix_local;
    bpy.context.scene.objects.link(o)
    
def process_branch(obj,start_b_id,level):
    start_b_id += 1
    b_id = start_b_id;
   
    lon_weights = []
    lat_weights = []
    
    # Gather data
    process_object_pass1(lon_weights,lat_weights,obj)    
    for ch0 in obj.children:
        process_object_pass1(lon_weights,lat_weights,ch0,obj)
        for ch1 in ch0.children:
            process_object_pass1(lon_weights,lat_weights,ch1,obj)
            
    max_lon = max(lon_weights)
    max_lat = max(lat_weights)      
    
    lon_weights.reverse()
    lat_weights.reverse()  
   
    process_object_pass2(max_lon,max_lat,lon_weights,lat_weights,b_id,obj,level)    
    for ch0 in obj.children:
        process_object_pass2(max_lon,max_lat,lon_weights,lat_weights,b_id,ch0,level)       
        for ch1 in ch0.children:
            process_object_pass2(max_lon,max_lat,lon_weights,lat_weights,b_id,ch1,level)
       
    leaf_id = 0
    for ch in obj.children:
        if len(ch.children) == 0:
            process_leaf(ch,leaf_id)
            leaf_id += 1
        
    return start_b_id

def process_leaf(obj,l_id):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    
    step = 0.0
    num_seg = 256
    grow_v = mathutils.Vector((0,0,1))
    lat_v = mathutils.Vector((0,1,0))
    n = lat_v.cross(grow_v).normalized()
    
    bm.verts.ensure_lookup_table()
    for v in bm.verts:
        proj = v.co.dot(grow_v)
        if(proj > step):
            step = proj
    
    step = step / (num_seg-1)
    seg_vals = [[0]*2 for _ in range(num_seg)]
    
    bm.edges.ensure_lookup_table()
    def calculate_seg_vals(v0,v1):
        v0_proj_len = v0.dot(grow_v)
        v1_proj_len = v1.dot(grow_v)
        
        if v0_proj_len > v1_proj_len:
            tmp = v0;
            v0 = v1
            v1 = tmp
            
            tmp = v0_proj_len
            v0_proj_len = v1_proj_len;
            v1_proj_len = tmp  
        
        on_left = n0.dot(n) < 0
                
        v0_seg_idx = math.ceil(v0_proj_len/step)
        v1_seg_idx = math.ceil(v1_proj_len/step)
        
        if v1_seg_idx == (num_seg-1):
            v1_seg_idx += 1
            
        if v0_seg_idx == v1_seg_idx:
            return;
        
        for idx in range(v0_seg_idx,v1_seg_idx):
            if v1_proj_len - v0_proj_len != 0:
                vt_lon_len = idx*step;
                vt_lon = grow_v * vt_lon_len;
                t = (vt_lon_len - v0_proj_len)/(v1_proj_len - v0_proj_len)
                vt = (1.0 - t)*v0 + t*v1
                lat_vt = vt - vt_lon
                
                if on_left:
                    if(seg_vals[idx][1] > -lat_vt.length):
                        seg_vals[idx][1] = -lat_vt.length
                else:    
                    if(seg_vals[idx][0] < lat_vt.length):
                        seg_vals[idx][0] = lat_vt.length

        
    for e in bm.edges:
        v0 = e.verts[0].co
        v1 = e.verts[1].co
        n0 = v0.cross(grow_v).normalized()
        n1 = v1.cross(grow_v).normalized()
        
        if n0.dot(n1) < 0:
            v0_lon = grow_v * v0.dot(grow_v)
            v1_lon = grow_v * v1.dot(grow_v)
            v0_lat = v0 - v0_lon
            v1_lat = v1 - v1_lon
            t = v0_lat.length / (v0_lat.length + v1_lat.length)
            v3 = (1.0 - t)*v0 + t*v1
            calculate_seg_vals(v0,v3)
            calculate_seg_vals(v1,v3)
        else:
            calculate_seg_vals(v0,v1)
    
    #generate_envelope(obj,step,num_seg,seg_vals)
                        
    ids_cl = bm.loops.layers.color["veg_ids"]
    lon_w_cl = bm.loops.layers.color["veg_lon_weights"]
    lat_w_cl = bm.loops.layers.color["veg_lat_weights"]
    
    max_lon = 0.0
    max_lat = 0.0
    
    for val in seg_vals:
        if max_lat < abs(val[0]):
              max_lat = abs(val[0])
        if max_lat < abs(val[1]):
              max_lat = abs(val[1])
        
    for f in bm.faces:
        for l in f.loops:
            lv = l.vert.co
            lv_lon_len = lv.dot(grow_v)
            if(lv_lon_len > max_lon):
                max_lon = lv_lon_len
    
    lat_treshold = max_lat * 0.1
    max_lat *= 0.5
    front_max_lon = max_lon - max_lat
    
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        for l in f.loops:
            lv = l.vert.co
            on_left = lv.cross(grow_v).normalized().dot(n) < 0
            
            lv_lon_len = lv.dot(grow_v)
            lv_lon =  grow_v * lv.dot(grow_v)
            
            t = lv_lon_len / step
            i = math.floor(t)
            t -= i;
            max_lat_r = 0.0
            max_lat_l = 0.0
            
            if(i == num_seg - 1):
                max_lat_r = (1.0-t)*seg_vals[i][0]
                max_lat_l = (1.0-t)*seg_vals[i][1]
            else:    
                max_lat_r = (1.0-t)*seg_vals[i][0] + t*seg_vals[i+1][0]
                max_lat_l = (1.0-t)*seg_vals[i][1] + t*seg_vals[i+1][1]
            
            lv_lat = lv - lv_lon
            lat_val = lv_lat.length;
            
            if lat_val < lat_treshold:
                lat_val = 0.0
            else:
                if on_left:
                    lat_val = -lat_val;
                lat_val = abs(((lat_val - max_lat_l) / (max_lat_r - max_lat_l))*2.0 - 1.0)
            
            if lv_lon_len > front_max_lon:
                lat_val2 = (lv_lon_len - front_max_lon)/max_lat
                lat_val =  max(lat_val,lat_val2)
            
            l[ids_cl] = (l[ids_cl][0],l[ids_cl][1],l_id/255.0)
            l[lon_w_cl] = (l[lon_w_cl][0],l[lon_w_cl][1],lv_lon_len/max_lon)
            l[lat_w_cl] = (l[lat_w_cl][0],l[lat_w_cl][1],lat_val)
    
    bm.to_mesh(obj.data)
    bm.free()

def process_object_pass1(lon_dists,lat_dists,obj,par = None):
    trans = mathutils.Matrix()
    
    if par != None:
        if par != obj.parent:
            trans = obj.parent.matrix_local * obj.matrix_local 
        else:
            trans = obj.matrix_local
        
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    
    ids_cl = bm.loops.layers.color["veg_ids"]
    lon_w_cl = bm.loops.layers.color["veg_lon_weights"]
    lat_w_cl = bm.loops.layers.color["veg_lat_weights"]
    
    grow_v = mathutils.Vector((0,0,1))
    
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        for l in f.loops:
            v = trans*l.vert.co;
            lon = v.dot(grow_v)
            lon_v = lon * grow_v
            lat_v = v - lon_v
            lat = lat_v.length
            
            lon_dists.append(lon)
            lat_dists.append(lat)
            
    bm.to_mesh(obj.data)
    bm.free()

def process_object_pass2(max_lon,max_lat,lon_dists,lat_dists,id,obj,level):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    
    ids_cl = bm.loops.layers.color["veg_ids"]
    lon_w_cl = bm.loops.layers.color["veg_lon_weights"]
    lat_w_cl = bm.loops.layers.color["veg_lat_weights"]

    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        for l in f.loops:
            if level==1:
                l[ids_cl] = (l[ids_cl][0],id/255.0,0)
                l[lon_w_cl] = (l[lon_w_cl][0],lon_dists.pop()/max_lon,0)
                l[lat_w_cl] = (l[lat_w_cl][0],lat_dists.pop()/max_lat,0)
            else:
                l[ids_cl] = (id/255.0,0,0)
                l[lon_w_cl] = (lon_dists.pop()/max_lon,0,0)
                l[lat_w_cl] = (lat_dists.pop()/max_lat,0,0)    

    bm.to_mesh(obj.data)
    bm.free()
    
##########################################################################################
        
def save_selected_obj_names(context):
    blend_file = bpy.data.filepath
    f_path = os.path.dirname(blend_file) + "/names.txt"
    obj_names = [];
    
    for o in bpy.context.selected_objects:
        obj_names.append(o.name)
        
    obj_names.sort()
    
    with open(f_path,"w") as f:
        for on in obj_names:
            f.write("%s\n" % on)


##########################################################################################
        
def register():
    bpy.utils.register_class(MirrorOnOff)
    bpy.utils.register_class(ArrayOnOff)
    bpy.utils.register_class(SolidifyOnOff)
    bpy.utils.register_class(OtVertexGroups)
    bpy.utils.register_class(OtRemoveSelectedVertexGroup)
    bpy.utils.register_class(OtAddNewVertexGroup)
    bpy.utils.register_class(OtAddSelectedFaces)
    bpy.utils.register_class(OtRemoveSelectedFaces)
    bpy.utils.register_class(OtSelectGroupFaces)
    bpy.utils.register_class(DisplayMirrorModifiersOn)
    bpy.utils.register_class(DisplayMirrorModifiersOff)
    bpy.utils.register_class(DisplayArrayModifiersOn)
    bpy.utils.register_class(DisplayArrayModifiersOff)
    bpy.utils.register_class(DisplaySolidifyModifiersOn)
    bpy.utils.register_class(DisplaySolidifyModifiersOff)
    bpy.utils.register_class(OtVtxGroupItem)
    bpy.utils.register_class(OTVtxGroupUI)
    bpy.utils.register_class(SingleBoneControls)
    bpy.utils.register_class(AddControlProperties)
    bpy.utils.register_class(VegetationPanel)
    bpy.utils.register_class(VegetationComputeWeights)
    bpy.utils.register_class(ObjectControls)
    bpy.utils.register_class(AddLodCurveProperties)
    bpy.utils.register_class(OtherToolsPanel)
    bpy.utils.register_class(OtherToolsSaveSelectedObjectNames)
    
    if not hasattr(bpy.types.Object,'ot_vertex_groups') or not hasattr(bpy.types.Object,'ot_vertex_group_idx'):
        bpy.types.Object.ot_vertex_groups = bpy.props.CollectionProperty(type=OtVtxGroupItem,name='Outerra vertex groups');
        bpy.types.Object.ot_vertex_group_idx = bpy.props.IntProperty(name='Active Outerra vertex group index',default=0,min = 0, max = 9999999);
        bpy.types.Object.ot_vertex_group_counter = bpy.props.IntProperty(name='Active Outerra vertex group index',default=0,min = 0, max = 9999999);


##########################################################################################
        
def unregister():
    bpy.utils.unregister_class(MirrorOnOff)
    bpy.utils.unregister_class(ArrayOnOff)
    bpy.utils.unregister_class(SolidifyOnOff)
    bpy.utils.unregister_class(OtVertexGroups)
    bpy.utils.unregister_class(OtRemoveSelectedVertexGroup)
    bpy.utils.unregister_class(OtAddNewVertexGroup)
    bpy.utils.unregister_class(OtAddSelectedFaces)
    bpy.utils.unregister_class(OtRemoveSelectedFaces)
    bpy.utils.unregister_class(OtSelectGroupFaces)
    bpy.utils.unregister_class(DisplayMirrorModifiersOn)
    bpy.utils.unregister_class(DisplayMirrorModifiersOff)
    bpy.utils.unregister_class(DisplayArrayModifiersOn)
    bpy.utils.unregister_class(DisplayArrayModifiersOff)
    bpy.utils.unregister_class(DisplaySolidifyModifiersOn)
    bpy.utils.unregister_class(DisplaySolidifyModifiersOff)
    bpy.utils.unregister_class(OtVtxGroupItem)
    bpy.utils.unregister_class(OTVtxGroupUI)
    bpy.utils.unregister_class(SingleBoneControls)
    bpy.utils.unregister_class(AddControlProperties)
    bpy.utils.unregister_class(VegetationPanel)
    bpy.utils.unregister_class(VegetationComputeWeights)
    bpy.utils.unregister_class(ObjectControls)
    bpy.utils.unregister_class(AddLodCurveProperties)
    bpy.utils.unregister_class(OtherToolsPanel)
    bpy.utils.unregister_class(OtherToolsSaveSelectedObjectNames)
    
if __name__ == "__main__":
    register()
