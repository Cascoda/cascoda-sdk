# Global config ---------------------------------------------------------------
project (cascoda-bm-core)

add_library(cascoda-bm-core
	${PROJECT_SOURCE_DIR}/source/cascoda_dispatch.c
	${PROJECT_SOURCE_DIR}/source/cascoda_log.c
	${PROJECT_SOURCE_DIR}/source/cascoda_spi.c
	${PROJECT_SOURCE_DIR}/source/cascoda_time.c
	${PROJECT_SOURCE_DIR}/source/cascoda_wait.c
)

target_link_libraries(cascoda-bm-core 
	PUBLIC
		cascoda-util
		cascoda-bm-plat
	)

target_include_directories(cascoda-bm-core PUBLIC ${PROJECT_SOURCE_DIR}/include)

cascoda_use_warnings(cascoda-bm-core)
