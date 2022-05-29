set(srcs
"app_main.cpp"

"io/fs_utils.cpp"
"io/sd_card_daemon.cpp"
)

set(requires
# System
console
fatfs
nvs_flash
sdmmc
vfs

# Project
cmd_system
scope_guard
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${requires}
)

# compiler flags for the `main` component can be specified here
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++2a -DCONFIG_MOUNT_ROOT="/s")

# compiler flags to be applied to all components can be specified here
# target_compile_options(${COMPONENT_LIB} PUBLIC)
