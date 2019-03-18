cmake_minimum_required (VERSION 3.11)

# Helper function to print binary section sizes - Auto-invoked when cascoda_make_binary is used
macro(cascoda_print_sizes a_target)
	if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc)
		add_custom_target(${a_target}.size ALL
			COMMAND arm-none-eabi-size -B $<TARGET_FILE:${a_target}>
			DEPENDS ${a_target}
			)
	elseif(CMAKE_C_COMPILER MATCHES armclang)
		add_link_options(--info=sizes)
	endif()
endmacro()

# Helper function to generate a binary file for baremetal targets
macro(cascoda_make_binary a_target)
	set(cascoda_made_binary $<TARGET_FILE_DIR:${a_target}>/${a_target}.bin)
	if(CMAKE_C_COMPILER MATCHES arm-none-eabi-gcc AND CMAKE_OBJCOPY)
		add_custom_target(${a_target}.bin ALL
			COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${a_target}> ${cascoda_made_binary}
			DEPENDS ${a_target}
			BYPRODUCTS ${cascoda_made_binary}
			)
	elseif(CMAKE_C_COMPILER MATCHES armclang AND CASCODA_FROMELF)
		add_custom_target(${a_target}.bin ALL
			COMMAND ${CASCODA_FROMELF} --bin --output ${cascoda_made_binary} $<TARGET_FILE:${a_target}>
			DEPENDS ${a_target}
			BYPRODUCTS ${cascoda_made_binary}
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
