# --- Unit tests (cmocka) ---

# Where cmocka is installed on this machine (a mingw/gcc build).
set(CMOCKA_DIR "C:/MinGW/cmocka")

# --- math module test ---
# Now that math_utils is a library, we LINK it instead of recompiling its
# source. Cleaner, and it tests the exact same object the app uses.
add_executable(test_math test_math.c)

# cmocka.h path only -- math_utils.h comes from the math_utils PUBLIC include dir.
target_include_directories(test_math PRIVATE ${CMOCKA_DIR}/include)

# Link math_utils (the code under test) and cmocka.
target_link_libraries(test_math PRIVATE math_utils ${CMOCKA_DIR}/lib/libcmocka.dll.a)

# cmocka is a DLL: copy it next to the test .exe so the test runs standalone.
add_custom_command(TARGET test_math POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_math>
)

# Register the exe with CTest so "ctest" runs it.
add_test(NAME math_tests COMMAND test_math)


# --- menus module test ---
# menus is already a library target, so we just link it (no source files here).
add_executable(test_menus test_menus.c)

target_include_directories(test_menus PRIVATE ${CMOCKA_DIR}/include)

# Link the menus library (brings its Inc/ headers along, since they're PUBLIC)
# and cmocka.
target_link_libraries(test_menus PRIVATE menus ${CMOCKA_DIR}/lib/libcmocka.dll.a)

add_custom_command(TARGET test_menus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_menus>
)

add_test(NAME menus_tests COMMAND test_menus)


# --- adc module test (demonstrates MOCKING) ---
# Compile ONLY adc.c (the logic). We deliberately do NOT include adc_hal.c,
# so the mock adc_read_raw() inside test_adc.c is the definition that links.
add_executable(test_adc
    test_adc.c
    ${CMAKE_SOURCE_DIR}/adc/Src/adc.c
)

target_include_directories(test_adc PRIVATE
    ${CMAKE_SOURCE_DIR}/adc/Inc
    ${CMOCKA_DIR}/include
)

# Links app_config too, because the adc.c we compile in now needs app_config.h.
target_link_libraries(test_adc PRIVATE app_config ${CMOCKA_DIR}/lib/libcmocka.dll.a)

add_custom_command(TARGET test_adc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMOCKA_DIR}/bin/cmocka.dll $<TARGET_FILE_DIR:test_adc>
)

add_test(NAME adc_tests COMMAND test_adc)
