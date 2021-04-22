add_executable(helloWorld
        helloWorld.cpp
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(helloWorld
        pico_stdlib
        hardware_i2c # needed for oled
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(helloWorld 1)
pico_enable_stdio_uart(helloWorld 0)

# create map/bin/hex file etc.
#pico_add_extra_outputs(helloWorld)

#todo discover purpose
# add url via pico_set_program_url
#example_auto_set_url(encoder)



