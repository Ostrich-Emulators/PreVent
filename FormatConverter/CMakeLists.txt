add_executable(formatconverter main.cpp Db.cpp)

target_link_libraries(formatconverter
	formats
	${SQLITE_LIBRARIES}
)

target_compile_options(formatconverter PRIVATE -Wall -O0)

target_include_directories(formatconverter
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}../Formats>
)

install (PROGRAMS formatconverter DESTINATION bin)

