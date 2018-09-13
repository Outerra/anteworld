How to use OT tools

Mirror On/Off
	On - will display mirror modifiers on all selected objects if they have one.
	Off - will hide mirror modifiers on all selected objects if they have one.
	
Array On/Off
	On - will display array modifiers on all selected objects if they have one.
	Off - will hide array modifiers on all selected objects if they have one.
	
OT vertex groups
  Why is it needed?
	Blender can't export vertex groups to FBX so there is needed other way to do that. Even if Blender could export vertex groups, 
	there is still problem with pairing given vertices with faces. It is the same case as with UV's. The same vertex can have more 
	than one UV coordinates in given UVmap, because you can map neighboring faces on different areas of texture. And original 
	Vertex groups are missing this information.
	
  How is it done?
	We use vertex colors which, like UV maps, also contain information about the face vertex is from. There is automatically
	created new vertex color layer called "ot_vertex_groups". Do not delete it and also do not manually paint to it in 
	Blender Vertex Paint mode. Use just tools we provided. You can also find some new Custom Properties under object
	('ot_vertex_groups','ot_vertex_group_counter','ot_vertex_groups_prop'). Do not modify or delete them. The base color
	of vertices is black so when vertex is black, it doesn't belong to any of groups.

  Provided funcionality:
	'Add vertex group' - will add new vertex group to the list of vertex groups.
	
	'Remove vertex group' - will remove vertex group you have selected in list below.
						  - it also paint all vertices it have owned with black color.
	
	'Add faces' - will add selected faces to the vertex group you have selected in the list below.
				- works only in Edit Mode or Vertex Paint mode
	
	'Clear faces' - will remove selected faces from every vertex group they were part of.
				  - works only in Edit Mode or Vertex Paint mode
	
	'Select faces' - will select all faces that belong to the vertex group selected below.
				   - works only in Edit Mode or Vertex Paint mode
				   
	'List of vertex groups' - under all the buttons, there is a list of vertex groups.
							- you can select and also rename vertex groups here.
							
	Hints:
		- when you are not seeing colors in Vertex Paint mode, check if your active vertex color layer is 'ot_vertex_groups'
		- currently Blender FBX supports Custom Properties only in Binary Mode. Also don't forget check 'Custom Properties'
		  checkbox when exporting.
      
Single bone controls
  
	
	