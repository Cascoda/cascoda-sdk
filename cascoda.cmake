cmake_minimum_required (VERSION 3.11)

# Helper function to print binary section sizes - Auto-invoked when cascoda_make_binary is used
macro(cascoda_print_sizes a_target)
	if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
		add_custom_command(TARGET ${a_target} POST_BUILD
			COMMAND arm-none-eabi-size -B $<TARGET_FILE:${a_target}>
			)
	elseif(CMAKE_C_COMPILER MATCHES armclang)
		add_link_options(--info=sizes)
	endif()
endmacro()

# Helper function to generate a binary file for baremetal targets
macro(cascoda_make_binary a_target)
	set(cascoda_made_binary $<TARGET_FILE_DIR:${a_target}>/${a_target}.bin)
	if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc AND CMAKE_OBJCOPY)
		add_custom_command(TARGET ${a_target} POST_BUILD
			COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${a_target}> ${cascoda_made_binary}
			)
	elseif(CMAKE_C_COMPILER MATCHES armclang AND CASCODA_FROMELF)
		add_custom_command(TARGET ${a_target} POST_BUILD
			COMMAND ${CASCODA_FROMELF} --bin --output ${cascoda_made_binary} $<TARGET_FILE:${a_target}>
			)
	endif()
	cascoda_print_sizes(${a_target})
	unset(cascoda_made_binary)
endmacro()

# Helper function to enable compiler warnings for a certain library
option( CASCODA_USE_WARNINGS "Enable compiler warnings for cascoda libraries" ON)
macro(cascoda_use_warnings a_target)
	if(CASCODA_USE_WARNINGS)
		target_compile_options(${a_target} PRIVATE -Wall -Wextra -Wno-format)
	endif()
endmacro()

# Helper function to add a cache variable that is restricted to certain values
# First argument is default
# eg. cascoda_dropdown(CASCODA_CA_VER "CA-xxxx Version number of target part" 8211 8210)
function(cascoda_dropdown var_name var_string)
	if(${ARGC} LESS 3)
		message(FATAL_ERROR "Invalid argument count for cascoda_dropdown call")
	endif()

	set(${var_name} ${ARGV2} CACHE STRING "${var_string} [${ARGN}]")
	set_property(CACHE ${var_name} PROPERTY STRINGS ${ARGN})

	if(NOT ${var_name} IN_LIST ARGN)
	    message(FATAL_ERROR "${var_name} must be one of [${ARGN}]")
	endif()
endfunction()

# Helper function to map one set of values to another
function(cascoda_map var_in var_out)
	list(FIND ARGN ${var_in} INDEX_FOUND)

	if(NOT INDEX_FOUND GREATER_EQUAL 0)
		message(FATAL_ERROR "cascoda_map failed to find mapped value")
	endif()

	math(EXPR INDEX_MOD "${INDEX_FOUND} % 2")

	if(${INDEX_MOD} EQUAL 0)
		math(EXPR INDEX_RES "${INDEX_FOUND} + 1")
	else()
		math(EXPR INDEX_RES "${INDEX_FOUND} - 1")
	endif()

	list(GET ARGN ${INDEX_RES} RESULT)
	set(${var_out} ${RESULT} PARENT_SCOPE)
endfunction()

# Helper function to put a target runtime into a subdirectory of the 'bin' dir
# useful for tidying up tests, eg cascoda_put_subdir(mytarget test)
function(cascoda_put_subdir target_in subdir)
	get_target_property(RUN_DIRECTORY ${target_in} RUNTIME_OUTPUT_DIRECTORY)
	set(RUN_DIRECTORY "${RUN_DIRECTORY}/${subdir}")
	set_target_properties(${target_in} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RUN_DIRECTORY})
endfunction()

# Helper macro to make a target compile as trustzone secure, and generate
# an import library for the non-secure-callable functions. The import library
# has the same name as the target, but with a '-implib' suffix.
#
# The linkerarg arguments are the arguments passed to the linker for config (eg ld script).
# The nonsecure_linkerarg argument is used to set the link INTERFACE of the implib.
# The secure_linkerarg is used to set the linker argument for the secure binary.
macro(cascoda_tz_secure target_in secure_linkerarg nonsecure_linkerarg)
	#TODO: Currently this only works for gcc - armclang would be nice
	set(IMPORT_LIB ${PROJECT_BINARY_DIR}/CMSE_${target_in}_implib.o)
	target_link_options(${target_in}
		PUBLIC
			-Wl,--cmse-implib,--out-implib,${IMPORT_LIB}
			-Wl,${secure_linkerarg}
	)
	# We add a custom NOP command as POST_BUILD so that Ninja doesn't complain about not knowing about the implib
	add_custom_command(TARGET ${target_in} POST_BUILD
		COMMAND ":"
		BYPRODUCTS ${IMPORT_LIB}
	)

	# Create a new target for the import library. You must link this library with the nonsecure
	# target that relies on the API provided by this binary.
	add_library(${target_in}-implib STATIC IMPORTED GLOBAL)
	add_dependencies(${target_in}-implib ${target_in})
	set_property(TARGET ${target_in}-implib PROPERTY IMPORTED_LOCATION ${IMPORT_LIB})

	# Set the nonsecure linkerarg as interface of the import lib
	target_link_options(${target_in}-implib INTERFACE -Wl,${nonsecure_linkerarg})
	unset(IMPORT_LIB)
endmacro()

# Helper function to print information about the configuration at build time.
# This is implemented to help catch common mistakes using the SDK, like building
# with the incorrect interface. The actual printing is implemented in git-version.cmake,
# which is run at every build.
set(CASCODA_IMPORTANT_VARS "" CACHE INTERNAL "")
function(cascoda_mark_important important_var)
	set(CASCODA_IMPORTANT_VARS ${CASCODA_IMPORTANT_VARS} "${important_var}" CACHE INTERNAL "")
endfunction()
