#version 450
#extension GL_ARB_separate_shader_objects : enable



layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int meshID;
layout(location = 3) in vec3 eyePos;
layout(location = 4) in vec3 fragPos;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D texSampler2;

vec3 objectColor = vec3(0.98, 0.83, 0.73);
vec3 ambient = vec3(0.2,0.2,0.2);
vec3 lightPos = vec3(0, 1, 0);
vec3 lightColor = vec3(0.25, 0.25, 0.25);



void main() {
	if(meshID == 0)
	{
		//vec3 normal = normalize(fragColor);
		//vec3 vertexPos = fragPos;
		//vec3 vertexNormal = normalize(normal);
	

		//float scaler = max(0.0, dot(normalize(vertexNormal), vec3(0.02, 0.25, 0.0)));

		//vec3 diffuseIntensity = (vec3(0.25, 0.25, 0.25) * scaler);
		
		vec3 mat = vec3(texture(texSampler, fragTexCoord).rgb);

		 //mat += diffuseIntensity;

		outColor = vec4(mat, 1.0);
	}
	else
	{
		vec3 mat = vec3(texture(texSampler2, fragTexCoord).rgb);

		 //mat += diffuseIntensity;

		outColor = vec4(mat, 1.0);

		// Diffuse 
		//vec3 norm = normalize(fragColor);
		//vec3 lightDir = normalize(lightPos - fragPos);
		//float diff = max(dot(norm, lightDir), 0.0);
		//vec3 diffuse = lightColor * diff;// * vec3(texture(material.diffuse, TexCoords));  
    
		//// Specular
		//vec3 viewDir = normalize(eyePos - fragPos);
		//vec3 reflectDir = reflect(-lightDir, norm);  
		//float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.25);
		//vec3 specular = vec3(0.85, 0.85, 0.85) * spec;// * vec3(texture(material.specular, TexCoords));
    
		//// Attenuation
		//float distance    = length(lightPos - fragPos);
		//float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * (distance * distance));    

		//ambient  *= attenuation;  
		//diffuse  *= attenuation;
		//specular *= attenuation;   
            
		//outColor = vec4(ambient + diffuse + specular, 1.0f);  
	}
}