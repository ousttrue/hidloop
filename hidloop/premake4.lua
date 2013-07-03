------------------------------------------------------------------------------
-- Project
------------------------------------------------------------------------------
project "hidloop"
--language "C"
language "C++"
kind "StaticLib"
--kind "DynamicLib"
--kind "ConsoleApp"
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
}
libdirs {
}
links {
}

