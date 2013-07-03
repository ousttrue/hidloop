------------------------------------------------------------------------------
-- Project
------------------------------------------------------------------------------
project "sample"
--language "C"
language "C++"
--kind "StaticLib"
--kind "DynamicLib"
kind "ConsoleApp"
--kind "WindowedApp"

files {
    "*.cpp", "*.h",
}
flags {
}
buildoptions {
}
defines {
}
includedirs {
    "../hidloop",
}
libdirs {
}
links {
    "hidloop",
}

