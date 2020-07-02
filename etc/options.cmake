# Cmake file containing all of the high-level configure options and parameters for the cascoda build
# This file is processed before anything else, allowing options that affect the whole build. Options
# only affect certain modules should be kept in their respective projects.


option(CASCODA_BUILD_LWIP "Build the LwIP stack into the Cascoda SDK" ON)

option(CASCODA_BUILD_DUMMY "Build the dummy baremetal layer for posix instead of the posix SDK. This replaces standard posix behaviour. Only for unit testing baremetal." OFF)
mark_as_advanced(FORCE CASCODA_BUILD_DUMMY)
