------------------------------------------------------------------------------
-- Solution
------------------------------------------------------------------------------
solution "sample"
configurations { 
    "Debug",
    "Release", 
}
configuration "gmake Debug"
do
    buildoptions { "-g" }
    linkoptions { "-g" }
end

configuration "gmake"
do
    buildoptions { 
        "-Wall", 
    }
end

configuration "gmake windows"
do
    buildoptions { 
        "-U__CYGWIN__", 
    }
end

configuration "vs*"
do
    linkoptions {}
end

configuration "windows*"
do
    defines {
        'WIN32',
        '_WIN32',
        '_WINDOWS',
    }
end

configuration "Debug"
do
    defines { "DEBUG" }
    flags { "Symbols" }
    targetdir "debug"
    links {
    }
end

configuration "Release"
do
    defines { "NDEBUG" }
    flags { "Optimize" }
    targetdir "release"
    links {
    }
end

configuration {}

include "sample"
include "hidloop"
