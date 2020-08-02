add_executable(formatconverter main.cpp Db.cpp)

if( MSVC )
  target_compile_options(formatconverter PRIVATE /W4)
else()
  target_compile_options(formatconverter PRIVATE -Wall)
endif()
target_link_libraries(formatconverter
	formats
	${SQLITE_LIBRARIES}
)

target_include_directories(formatconverter
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		PRIVATE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Formats ${CMAKE_CURRENT_SOURCE_DIR}/../Formats/generated>
)

install (TARGETS formatconverter DESTINATION bin)

