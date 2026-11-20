# Toolchain for bare-metal ARM Cortex-M with the GNU Arm Embedded toolchain (arm-none-eabi-gcc).
# The CPU is set with -mcpu in CMAKE_C_FLAGS, see the cortex-m presets.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
# No OS to link test programs against
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
