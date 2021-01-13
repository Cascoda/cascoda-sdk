# Global config ---------------------------------------------------------------
project (ca-ot-util)

# ca-ot-util is an object library and linked into the platform, so do not
# manually link it into an application, the APIs are already available
# when the cascoda openthread platform layer is linked.

# Main library config ---------------------------------------------------------
add_library(ca-ot-util OBJECT
	${PROJECT_SOURCE_DIR}/src/cascoda_dns.c
	)

target_include_directories(ca-ot-util
	PUBLIC
		${PROJECT_SOURCE_DIR}/include
	)

target_link_libraries(ca-ot-util
	PUBLIC
		ca821x-api
		openthread-plat-api
	)

cascoda_use_warnings(ca-ot-util)
