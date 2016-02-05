#!lua

includeDirList = { 
    "shared",
    "shared/boost",
    "shared/gl3w",
    "shared/imgui",
    "shared/include",
    "include",
    "shared/assimp/include",
    "shared/zlib-1.2.8",
    "shared/irrKlang/include",
    "src",
}

libDirectories = { 
    "lib",
    "shared/boost/lib",
    "shared/assimp/lib",
    "shared/zlib-1.2.8",
    "shared/irrKlang/bin/linux-gcc-64"
}

if os.get() == "macosx" then
    linkLibs = {
        "game-framework",
        "imgui",
        "glfw3",
        "assimp"
    }
end

if os.get() == "linux" then
    linkLibs = {
        "game-framework",
        "imgui",
        "glfw3",
        "GL",
        "Xinerama",
        "Xcursor",
        "Xxf86vm",
        "Xi",
        "Xrandr",
        "X11",
        "stdc++",
        "dl",
        "pthread",
        "assimp",
        "z",
        "IrrKlang"
    }
end

-- Build Options:
if os.get() == "macosx" then
    linkOptionList = { "-framework IOKit", "-framework Cocoa", "-framework CoreVideo",
     "-framework OpenGL"}
end
if os.get() == "linux" then
    linkOptionList = {"-pthread", "-lpthread", "-Wl,--no-as-needed"}
end

buildOptions = {"-std=c++11"}

-- Get the current OS platform
PLATFORM = os.get()

-- Build libz static library and copy it into <cs488_root>/lib if it is not
-- already present.
if not os.isfile("shared/zlib-1.2.8/libz.a") then
    os.chdir("shared/zlib-1.2.8")
    os.execute("cmake .")
    os.execute("make clean")
    os.execute("make")
    os.chdir("../../")
end

-- Build assimp static library and copy it into <cs488_root>/lib if it is not
-- already present.
if not os.isfile("shared/assimp/lib/libassimp.a") then
    os.chdir("shared/assimp")
    os.execute("cmake -G \"Unix Makefiles\" -DBUILD_SHARED_LIBS=OFF")
    os.execute("make clean")
    os.execute("make")
    os.chdir("../../")
end

-- Build glfw3 static library and copy it into <cs488_root>/lib if it is not
-- already present.
if not os.isfile("lib/libglfw3.a") then
    os.chdir("shared/glfw-3.1.1")
    os.mkdir("build")
    os.chdir("build")
    os.execute("cmake ../")
    os.execute("make clean")
    os.execute("make")
    os.chdir("../../../")
    os.mkdir("lib")
    os.execute("cp shared/glfw-3.1.1/build/src/libglfw3.a lib/")
end

-- Build lua-5.3.1 library and copyt it into <cs488_root>/lib if it is not
-- already present.
if not os.isfile("lib/liblua.a") then
    os.chdir("shared/lua-5.3.1")
    os.execute("make clean")
    if PLATFORM == "macosx" then
        os.execute("make macosx")
    elseif PLATFORM == "linux" then
        os.execute("make linux")
    elseif PLATFORM == "windows" then
        os.execute("make mingw")
    end

    os.chdir("../../")
    os.execute("cp shared/lua-5.3.1/src/liblua.a lib/")
end

solution "BuildStaticLibs"
    configurations { "Debug", "Release" }

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }

    -- Builds gamewindow-framework static library
    project "game-framework"
        kind "StaticLib"
        language "C++"
        location "build"
        objdir "build"
        targetdir "lib"
        buildoptions (buildOptions)
        includedirs (includeDirList)
        files { "shared/game-framework/*.cpp" }

    -- Build imgui static library
    project "imgui"
        kind "StaticLib"
        language "C++"
        location "build"
        objdir "build"
        targetdir "lib"
        includedirs (includeDirList)
        includedirs {
            "shared/imgui/examples/opengl3_example",
            "shared/imgui/examples/libs/gl3w/",
        }
        files { 
            "shared/imgui/*.cpp",
            "shared/gl3w/GL/gl3w.c"
        }

    project "CS488Project"
        kind "ConsoleApp"
        language "C++"
        location "build"
        objdir "build"
        targetdir "."
        buildoptions (buildOptions)
        libdirs (libDirectories)
        links (linkLibs)
        linkoptions (linkOptionList)
        includedirs (includeDirList)
        files { 
            "src/*.cpp",
            "src/*.c"
        }