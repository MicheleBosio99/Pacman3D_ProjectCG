@echo off
"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" Game/Shader.vert -o Game/ShaderVert.spv
"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" Game/Shader.frag -o Game/ShaderFrag.spv

"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" HUD/HUDShader.vert -o HUD/HUDShaderVert.spv
"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" HUD/HUDShader.frag -o HUD/HUDShaderFrag.spv

"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" Test/Shader.vert -o Test/ShaderVert.spv
"C:\Users\bosio\All_Files\Computer Graphics\VulkanSDK\Bin\glslc.exe" Test/Shader.frag -o Test/ShaderFrag.spv

pause