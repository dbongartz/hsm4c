# Enables the project's compiler warnings on a target.
# Use CMAKE_COMPILE_WARNING_AS_ERROR=ON to turn them into errors.
function(hsm4c_target_warnings target)
  target_compile_options(
    ${target} PRIVATE $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
                      $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>)
endfunction()
