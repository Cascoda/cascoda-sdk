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
	if(NOT CASCODA_BUILD_BINARIES)
		set_target_properties(${a_target} PROPERTIES EXCLUDE_FROM_ALL ON)
	endif()
	unset(cascoda_made_binary)
endmacro()

# Helper function to enable compiler warnings and linting for a certain library
option(CASCODA_USE_WARNINGS "Enable compiler warnings for cascoda libraries" ON)
option(CASCODA_USE_CLANG_TIDY "Enable clang-tidy warnings for cascoda libraries" OFF)
function(cascoda_use_warnings a_target)
	if(CASCODA_USE_WARNINGS)
		target_compile_options(${a_target} PRIVATE -Wall -Wextra -Wno-format)
	endif()

	if(CASCODA_USE_CLANG_TIDY)
		# Find clang-tidy
		find_program(
			CLANG_TIDY_EXE
			NAMES "clang-tidy-8"
			      "clang-tidy"
			DOC "Path to clang-tidy executable"
			)
		if(CLANG_TIDY_EXE)
			set(CLANG_TIDY_ARGS ${CLANG_TIDY_EXE} "-checks=bugprone-*,clang-analyzer-*,portability-*,-clang-diagnostic-unused-function")
			set_target_properties(${a_target} PROPERTIES C_CLANG_TIDY "${CLANG_TIDY_ARGS}" CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")
		else()
			message(FATAL_ERROR "CASCODA_USE_CLANG_TIDY enabled but clang-tidy not found.")
		endif()
	endif()
endfunction()

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
# useful for tidying up tests, eg cascoda_put_subdir(test mytarget mytarget2 ...)
function(cascoda_put_subdir subdir)
	foreach(target_in IN LISTS ARGN)
		get_target_property(RUN_DIRECTORY ${target_in} RUNTIME_OUTPUT_DIRECTORY)
		set(RUN_DIRECTORY "${RUN_DIRECTORY}/${subdir}")
		set_target_properties(${target_in} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RUN_DIRECTORY})
	endforeach()
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
	cascoda_tz_build(${target_in})
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

# Helper macro to build with csme args
# This doesn't create any implibs, so should only be used for sub-libs
# which are included in the main libs. If any entry functions are included
# then OBJECT libraries should be used, or else they won't be exported.
macro(cascoda_tz_build target_in)
	# O2 is enabled here as Os has fatal errors in code generation for csme entry functions (priv issue #259)
	target_compile_options(${target_in} PUBLIC -mcmse -O2)
	target_link_options(${target_in} PUBLIC -mcmse)
endmacro()

# Helper function to print information about the configuration at build time.
# This is implemented to help catch common mistakes using the SDK, like building
# with the incorrect interface. The actual printing is implemented in git-version.cmake,
# which is run at every build.
set(CASCODA_IMPORTANT_VARS "" CACHE INTERNAL "")
function(cascoda_mark_important important_var)
	set(CASCODA_IMPORTANT_VARS ${CASCODA_IMPORTANT_VARS} "${ARGV}" CACHE INTERNAL "")
endfunction()

# Helper function to change the memory configuration for a given application.
# This allows individual applications to have tweaked stack/heap sizes
function(cascoda_configure_memory target_in stack_size heap_size)
	target_sources(${target_in} PRIVATE ${CASCODA_STARTUP_ASM})
	target_compile_definitions(${target_in} PRIVATE ${CASCODA_STACK_SIZE}=${stack_size})
	target_compile_definitions(${target_in} PRIVATE ${CASCODA_HEAP_SIZE}=${heap_size})
endfunction()

file(WRITE "${CMAKE_BINARY_DIR}/longtests.csv.in" "")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/longtests.csv" INPUT "${CMAKE_BINARY_DIR}/longtests.csv.in")
# Helper function to add a long test to the 'longtest' target.
# target_name is the target name of the test to run
# timeout_val is the timeout in seconds to apply to the test
# Additional parameters are arguments to pass to the test and "--gtest_list_tests" is a special arg which will expand all tests of a gtest enabled executable
function(cascoda_add_longtest target_name timeout_val)
	set(ARGLIST "")
	foreach(argument IN LISTS ARGN)
		set(ARGLIST "${ARGLIST},${argument}")
	endforeach()
	file(APPEND "${CMAKE_BINARY_DIR}/longtests.csv.in" "$<TARGET_FILE:${target_name}>,${timeout_val}${ARGLIST}\n")
endfunction()

# Track whether a cache variable has changed since the last configure.
# var_name is the name of the variable to track
# has_changed is the name of the variable to set to true if the variable has changed
macro(cascoda_has_changed var_name has_changed)
	if("${${var_name}}" STREQUAL "${${var_name}_internal_value}")
		set("${has_changed}" OFF)
	else()
		set("${has_changed}" ON)
		set("${var_name}_internal_value" "${${var_name}}" CACHE INTERNAL "")
	endif()
endmacro()