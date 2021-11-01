# Global config ---------------------------------------------------------------
project (ca821x-openthread)

# Main library config ---------------------------------------------------------
add_library(ca821x-openthread-bm-plat OBJECT
	${PROJECT_SOURCE_DIR}/platform/alarm.c
	${PROJECT_SOURCE_DIR}/platform/entropy.c
	${PROJECT_SOURCE_DIR}/platform/logging.c
	${PROJECT_SOURCE_DIR}/platform/misc.c
	${PROJECT_SOURCE_DIR}/platform/radio.c
	${PROJECT_SOURCE_DIR}/platform/serial.c
	${PROJECT_SOURCE_DIR}/platform/settings.c
	${PROJECT_SOURCE_DIR}/platform/sntp_helper.c
	)

target_link_libraries(ca821x-openthread-bm-plat
	PUBLIC
		cascoda-bm
	PRIVATE
		m
		openthread-plat-api
		mbedcrypto
		ca-ot-util
		cascoda-util
	)

# NTP server used by platform/sntp_helper.c
set(CASCODA_NTP_SERVER "time.google.com" CACHE STRING "The domain name of the NTP server to use for updating the RTC")
set(CASCODA_SNTP_INTERVAL 2880 CACHE STRING "The interval beween SNTP requests, in minutes. Default is two days (2880 minutes).")
mark_as_advanced(CASCODA_SNTP_INTERVAL)
target_compile_definitions(ca821x-openthread-bm-plat PRIVATE NTP_SERVER=\"${CASCODA_NTP_SERVER}\")
target_compile_definitions(ca821x-openthread-bm-plat PRIVATE CASCODA_SNTP_INTERVAL=${CASCODA_SNTP_INTERVAL})

target_include_directories(ca821x-openthread-bm-plat
	PRIVATE
		${openthread_SOURCE_DIR}/src/core
		${PROJECT_SOURCE_DIR}/platform
	PUBLIC
		${openthread_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/include
	)

# Inject credentials option
option(CASCODA_OT_FORCE_CREDS "DEVELOPMENT/DEMO ONLY: Inject credentials into the openthread stack to prevent need to commission" OFF)
set(CASCODA_OT_FORCE_CHANNEL 20 CACHE STRING "Force a channel (integer 11-26) to be used to bootstrap an openthread node - used with CASCODA_OT_FORCE_CREDS")
set(CASCODA_OT_FORCE_PANID 0x1234 CACHE STRING "Force a panid (hex 0x0000 - 0xFFFE) to be used to bootstrap an openthread node - used with CASCODA_OT_FORCE_CREDS")
set(CASCODA_OT_FORCE_MASTERKEY 00112233445566778899aabbccddeeff CACHE STRING "Force a masterkey (16 byte hex string) to be used to bootstrap an openthread node - used with CASCODA_OT_FORCE_CREDS")
if(CASCODA_OT_FORCE_CREDS)
	message(WARNING "CASCODA_OT_FORCE_CREDS is not secure or reliable, and is for development purposes only. Should never be used for real applications! Consider using CASCODA_OT_JOINER_CREDENTIAL instead!")
	cascoda_mark_important(CASCODA_OT_FORCE_CHANNEL CASCODA_OT_FORCE_PANID CASCODA_OT_FORCE_MASTERKEY)
	mark_as_advanced(CLEAR CASCODA_OT_FORCE_CHANNEL CASCODA_OT_FORCE_PANID CASCODA_OT_FORCE_MASTERKEY)

	# Check that all credential injections are valid
	if(NOT (CASCODA_OT_FORCE_CHANNEL GREATER_EQUAL 11 AND CASCODA_OT_FORCE_CHANNEL LESS_EQUAL 26))
		message(FATAL_ERROR "CASCODA_OT_FORCE_CHANNEL invalid or out of range 11-26")
	endif()
	if(NOT (CASCODA_OT_FORCE_PANID MATCHES "^0x[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Ea-e]$")) # CMake doesn't support repeated groups regex syntax
		message(FATAL_ERROR "CASCODA_OT_FORCE_PANID invalid format or out of range 0x0000 - 0xFFFE")
	endif()
	STRING(LENGTH ${CASCODA_OT_FORCE_MASTERKEY} CASCODA_OT_FORCE_MASTERKEY_LEN)
	if(NOT (CASCODA_OT_FORCE_MASTERKEY MATCHES "^[0-9A-Fa-f]+$" AND CASCODA_OT_FORCE_MASTERKEY_LEN EQUAL 32))
		message(FATAL_ERROR "CASCODA_OT_FORCE_MASTERKEY invalid format (16 bytes, 32 hex characters)")
	endif()

	# Convert the masterkey string to a C array
	STRING(REGEX REPLACE "([0-9A-Fa-f][0-9A-Fa-f])" "0x\\1," CASCODA_OT_FORCE_MASTERKEY_ARR ${CASCODA_OT_FORCE_MASTERKEY})
	STRING(SUBSTRING ${CASCODA_OT_FORCE_MASTERKEY_ARR} 0 79 CASCODA_OT_FORCE_MASTERKEY_ARR)

	# Add the defines to the misc.c source file to inject them.
	set_property(
		SOURCE ${PROJECT_SOURCE_DIR}/platform/misc.c
		APPEND
		PROPERTY COMPILE_DEFINITIONS
		INJECT_CREDS
		INJECT_MASTERKEY=${CASCODA_OT_FORCE_MASTERKEY_ARR}
		INJECT_PANID=${CASCODA_OT_FORCE_PANID}
		INJECT_CHANNEL=${CASCODA_OT_FORCE_CHANNEL}
	)

else()
	mark_as_advanced(FORCE CASCODA_OT_FORCE_CHANNEL CASCODA_OT_FORCE_PANID CASCODA_OT_FORCE_MASTERKEY)
endif()

#Hardcode Joiner Credentials
set(CASCODA_OT_JOINER_CREDENTIAL "" CACHE STRING "DEVELOPMENT ONLY: Hardcode a Joiner Credential - useful for developing applications with no serial interface.")
if (CASCODA_OT_JOINER_CREDENTIAL)
	message(WARNING "CASCODA_OT_JOINER_CREDENTIAL is not secure, and is for development purposes only. Production applications should use a different credential for every device!")

	#Application joins network with configured CMake value
	set_property(
			SOURCE ${PROJECT_SOURCE_DIR}/platform/misc.c
			APPEND
			PROPERTY COMPILE_DEFINITIONS
			CASCODA_OT_JOINER_CRED=\"${CASCODA_OT_JOINER_CREDENTIAL}\"
	)
endif ()



# Inject platform into openthread library to resolve cyclic dependency
add_library(ca821x-openthread-bm-ftd
	$<TARGET_OBJECTS:ca821x-openthread-bm-plat>
	$<TARGET_OBJECTS:openthread-ftd-obj>
	$<TARGET_OBJECTS:ca-ot-util>
)

target_link_libraries(ca821x-openthread-bm-ftd
	PUBLIC
		ca-ot-util
		openthread-ftd-obj
		ca821x-openthread-bm-plat
	)

add_library(ca821x-openthread-bm-mtd
	$<TARGET_OBJECTS:ca821x-openthread-bm-plat>
	$<TARGET_OBJECTS:openthread-mtd-obj>
	$<TARGET_OBJECTS:ca-ot-util>
)

target_link_libraries(ca821x-openthread-bm-mtd
	PUBLIC
		ca-ot-util
		openthread-mtd-obj
		ca821x-openthread-bm-plat
	)

# Test app config -------------------------------------------------------------
if(CASCODA_OPENTHREAD_FTD AND OT_COAP)
	add_executable(ot-cli
		${PROJECT_SOURCE_DIR}/example/thread_bm.c
		${PROJECT_SOURCE_DIR}/example/sensordemo.c
		)
	target_include_directories(ot-cli
		PRIVATE ${PROJECT_SOURCE_DIR}/platform
		)
	target_link_libraries(ot-cli openthread-cli-ftd ca821x-openthread-bm-ftd tinycbor-master)
	target_compile_definitions(ot-cli PRIVATE OT_CLI=1)
	cascoda_make_binary(ot-cli)

	add_executable(ot-ncp
		${PROJECT_SOURCE_DIR}/example/thread_bm.c
		)
	target_include_directories(ot-ncp
		PRIVATE ${PROJECT_SOURCE_DIR}/platform
		)
	target_link_libraries(ot-ncp ca821x-openthread-bm-ftd openthread-ncp-ftd)
	target_compile_definitions(ot-ncp PRIVATE OT_NCP=1)
	cascoda_make_binary(ot-ncp)
endif()
