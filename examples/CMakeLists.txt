# Examples for LuminLogger

# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE lumin::logger)

# Set output directory
set_target_properties(basic_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install examples if requested
option(LUMIN_LOGGER_INSTALL_EXAMPLES "Install LuminLogger examples" OFF)
if(LUMIN_LOGGER_INSTALL_EXAMPLES)
    install(TARGETS basic_example
        DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )
    
    install(FILES
        basic_example.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    )
endif() 