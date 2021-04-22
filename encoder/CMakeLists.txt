add_executable(encoder
        encoder.cpp
        ../oled/font.cc ../oled/oled.cc ../oled/splash.cc #oled
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(encoder
        pico_stdlib
        pico_multicore
        hardware_i2c # needed for oled
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(encoder 1)
pico_enable_stdio_uart(encoder 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(encoder)

#todo discover purpose
# add url via pico_set_program_url
#example_auto_set_url(encoder)



