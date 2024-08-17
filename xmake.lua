set_xmakever("2.9.3")

set_project("Raytracer")
set_version("1.0.0")

set_allowedplats("windows")
set_allowedarchs("windows|x64")

add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_optimize("fastest")

if (is_mode("debug")) then
    add_defines("RT_DEBUG")
end

includes("xmake/**.lua") 

add_repositories("pixfri https://github.com/Pixfri/xmake-repo.git")

add_requires("spdlog v1.9.0", "glfw 3.4", "vulkan-loader 1.3.290+0", "vk-bootstrap v1.3.290", 
             "vulkan-memory-allocator v3.1.0", "vulkan-utility-libraries v1.3.290", "glm 1.0.1")
add_requires("glslang 1.3.290+0", {configs = {binaryonly = true}})
add_requires("imgui v1.91.0", {configs = {glfw = true, vulkan = true, debug = is_mode("debug")}})
             
add_defines("GLFW_INCLUDE_VULKAN")

local outputdir = "$(mode)-$(os)-$(arch)"

rule("cp-resources")
  after_build(function (target)
    os.cp("Resources", "build/" .. outputdir .. "/" .. target:name() .. "/bin")
  end)

rule("cp-imgui-layout")
  after_build(function(target)
    os.cp("Resources/imgui.ini", "build/" .. outputdir .. "/" .. target:name() .. "/bin")    
  end)
  

target("Raytracer")
    set_kind("binary")
    add_rules("utils.glsl2spv", {outputdir = "build/" .. outputdir .. "/Raytracer/bin/Shaders"})
    add_rules("cp-resources", "cp-imgui-layout")

    set_targetdir("build/" .. outputdir .. "/Raytracer/bin")
    set_objectdir("build/" .. outputdir .. "/Raytracer/obj")
    
    add_files("Source/**.cpp")
    add_headerfiles("Include/**.hpp", "Include/**.inl")
    add_includedirs("Include/")
    
    add_files("Shaders/**.vert", "Shaders/**.frag") -- Tell glsl2spv to compile the files.
    add_headerfiles("Shaders/**") -- A trick to make them show up in VS/Rider solutions.

    add_headerfiles("Resources/**") -- A trick to make them show up in VS/Rider solutions.

    set_pcxxheader("Include/Raytracer/rtpch.hpp")
    
    add_packages("spdlog", "glfw", "vulkan-loader", "vk-bootstrap", "vulkan-memory-allocator", "vulkan-utility-libraries", 
                 "glm", "imgui")
    
