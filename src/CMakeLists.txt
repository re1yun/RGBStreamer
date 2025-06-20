add_executable(RGBStreamer
    main.cpp
    CaptureModule.cpp
)

# Link required libraries
find_package(nlohmann_json CONFIG REQUIRED)

# Windows-specific libraries
if (WIN32)
    target_link_libraries(RGBStreamer PRIVATE nlohmann_json::nlohmann_json d3d11 dxgi ws2_32)
else()
    target_link_libraries(RGBStreamer PRIVATE nlohmann_json::nlohmann_json)
endif()
