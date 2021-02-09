project(wakaama)

set(LWM2M_VERSION "1.1" CACHE STRING "LWM2M version for client and max LWM2M version for server.")

include(coap/coap.cmake)
include(core/wakaama.cmake)
include(data/data.cmake)

add_library(wakaama-client
	${WAKAAMA_SOURCES}
	${COAP_SOURCES}
	${DATA_SOURCES}
)

target_include_directories(wakaama-client
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	PRIVATE
		${DATA_HEADERS_DIR}
		${COAP_HEADERS_DIR}
		${PROJECT_SOURCE_DIR}/core
)

target_compile_definitions(wakaama-client PUBLIC
	${WAKAAMA_DEFINITIONS}
	LWM2M_CLIENT_MODE
	LWM2M_BOOTSTRAP
)

# Currently we need to enable TLV support, as it is required for bootstrapping (before preferred-content-format is merged)
target_compile_definitions(wakaama-client PUBLIC LWM2M_SUPPORT_TLV)

if(LWM2M_VERSION VERSION_GREATER "1.0")
    target_compile_definitions(wakaama-client PUBLIC LWM2M_SUPPORT_SENML_JSON)
else()
    target_compile_definitions(wakaama-client PUBLIC LWM2M_SUPPORT_JSON)
endif()
