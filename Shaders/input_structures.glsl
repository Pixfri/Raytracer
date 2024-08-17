layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout (set = 0, binding = 1) uniform GlobalUniform {
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
    vec3 lightPosition;
} globalUniform;