Andrea Tortato - andrea.tortato01@estudiant.upf.edu - NIA:231603
Edgar Espinos - edgar.espinos01@estudiant.upf.edu - NIA:

[README of Assigment 1, update before delivery]

# GTR Framework
The GTR Framework is an OpenGL C++ framework used for teaching real-time graphics at Universitat Pompeu Fabra. This framework allows you to move around the scene using the WASD, Q and E keys and render and manipulate elements of a scene such as meshes with specific textures and lights, altering the illumination of the scene in an intuitive way using a graphical user interface (GUI).

To execute the application open the GTR_application.exe file.

### User Interface
The framework provides a graphical user interface that lets you select specific objects in the scene and change their state, such as position, color, or render the entire scene in different modes. You can also select prefab objects and change their position, rotation, and scale, as well as alter the different parameters of the lights, such as intensity, color, shadow bias, etc.

### Lights
It also supports different types of lights, including point lights, directional lights, and spotlights. Each type of light affects the scene in a different way, and you can adjust the different parameters of each light using the GUI.

### Rendering Modes
Apart from just rendering the elements of the scene you can also render their bounding boxes or view them by their wireframes. You can also choose to view the shadow maps of the lights which cast shadows.
All of the above can be done choosing firm three rendering modes:
- TEXTURE mode: Renders the scene with just the objects themselves and their textures.
- LIGHTS mode: Like texture mode but also renders the different types of lights, how they contribute to the illumination of the objects in the scene and cast shadows.
- NORMAL_MAP mode: Like lights mode but also renders the objects with a normal map to produce a 3D effect on the textures based on the different lights.

### Known issues
Due to a bug, if you view the normal map mode after having rendered the lights mode, the normal maps and metallic attributes of the objects will not render correctly.
Also the directional light produces zig-zagged shadows due to an incorrect shadow mapping (easy to see when the shadow map option is toggled ON).



