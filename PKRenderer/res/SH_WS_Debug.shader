#version 450                                                            
#pragma PROGRAM_VERTEX

layout(set = 0, binding = 0) uniform UniformBufferObject                
{                                                                       
    mat4 model;                                                         
    mat4 viewproj;                                                      
} ubo;                                                                  
                                                                        
layout(location = 0) in vec3 inPosition;                                
layout(location = 1) in vec2 inTexcoord;                                
layout(location = 2) in vec3 inColor;    

layout(location = 0) out vec3 fragColor;                                
layout(location = 1) out vec2 texCoord;                                 
                                                                        
void main()                                                             
{                                                                       
    gl_Position = ubo.viewproj * ubo.model * vec4(inPosition, 1.0);     
    fragColor = inColor;                                                
    texCoord = inTexcoord;                                              
}

#pragma PROGRAM_FRAGMENT
                                                                       
layout(location = 0) in vec3 fragColor;                                
layout(location = 1) in vec2 texCoord;                                 
layout(set = 0, binding = 1) uniform sampler2D tex1;                   
                                                                       
layout(location = 0) out vec4 outColor;                                
                                                                       
void main()                                                            
{                                                                      
    outColor = vec4(texture(tex1,texCoord).xyz * fragColor, 1.0);      
}