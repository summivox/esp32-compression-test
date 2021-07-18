set(srcs
"app_main.cpp"

"io/sd_card_daemon.cpp"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
)

# compiler flags for the `main` component can be specified here
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++2a)

# compiler flags to be applied to all components can be specified here
# target_compile_options(${COMPONENT_LIB} PUBLIC)
