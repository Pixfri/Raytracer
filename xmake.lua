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

add_requires("spdlog v1.9.0")

local outputdir = "$(mode)-$(os)-$(arch)"

target("Raytracer")
    set_kind("binary")

    set_targetdir("build/" .. outputdir .. "/Raytracer/bin")
    set_objectdir("build/" .. outputdir .. "/Raytracer/obj")

    add_files("Source/**.cpp")
    add_headerfiles("Include/**.hpp", "Include/**.inl")
    add_includedirs("Include/")

    set_pcxxheader("Include/Raytracer/rtpch.hpp")
    
    add_packages("spdlog")