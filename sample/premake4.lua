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

BOOST_DIR=os.getenv("BOOST_DIR")

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
    BOOST_DIR,
}
libdirs {
    BOOST_DIR.."/lib",
    BOOST_DIR.."/stage/lib",
}
links {
    "hidloop",
    "winmm",
}

