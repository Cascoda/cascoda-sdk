cmake_minimum_required (VERSION 3.11)

#Helper function to generate a binary file for baremetal targets
macro(cascoda_make_binary a_target)
	if(CMAKE_OBJCOPY)
		set(cascoda_made_binary $<TARGET_FILE_DIR:${a_target}>/${a_target}.bin)
		add_custom_target(${a_target}.bin ALL
			COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${a_target}> ${cascoda_made_binary}
			DEPENDS ${a_target}
			BYPRODUCTS ${cascoda_made_binary}
			)
		unset(cascoda_made_binary)
	endif()
endmacro()
