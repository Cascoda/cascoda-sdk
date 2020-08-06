project (cascoda-docs)

# Find plantuml
set(CASCODA_PLANTUML_JAR "" CACHE FILEPATH "Path to the PlantUML .jar file for generating UML images (including in doxygen)")

configure_file(
	${PROJECT_SOURCE_DIR}/doxyfile.in
	${PROJECT_BINARY_DIR}/doxyfile
	)

if(CASCODA_PLANTUML_JAR AND EXISTS "${CASCODA_PLANTUML_JAR}")
	# Add plantuml target (Yes, we build the results into source, because we want to distribute images as documentation)
	add_custom_target(plantuml
		COMMAND java -Djava.awt.headless=true -jar "${CASCODA_PLANTUML_JAR}" -nbthread auto -checkmetadata -o "png" -config "./cascoda.pu.conf" -r "./**.pu"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
endif()

# Find doxygen
find_package(Doxygen
        COMPONENTS dot
        OPTIONAL_COMPONENTS mscgen dia
        )

if(TARGET Doxygen::doxygen)
	# Add docs target
	add_custom_target(docs
		COMMAND Doxygen::doxygen ${CASCODA_BINARY_DIR}/docs/doxyfile
		WORKING_DIRECTORY ${CASCODA_BINARY_DIR}
		)
endif()

