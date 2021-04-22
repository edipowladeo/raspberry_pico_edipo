
add_executable(example
        example.cc font.cc oled.cc splash.cc
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(example pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(example)

add_custom_target(flash
    COMMAND cp app.uf2 /media/pi/RPI-RP2
    DEPENDS app
)
