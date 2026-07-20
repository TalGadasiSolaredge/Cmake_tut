# --- The "adc" module ---
# Library = the conversion logic (adc.c) + the real hardware read (adc_hal.c).
# The app links both; the unit test links only adc.c and mocks the leaf.
add_library(adc
    Src/adc.c
    Src/adc_hal.c
)

target_include_directories(adc PUBLIC Inc)

# adc.c #includes the generated app_config.h (for ADC_VREF_MV). PRIVATE because
# only adc's own .c needs it -- code that merely uses adc.h does not.
target_link_libraries(adc PRIVATE app_config)
