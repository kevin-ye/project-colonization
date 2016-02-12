

set(command "/opt/local/bin/cmake;-P;/Users/Ye-Macbook/Documents/Development/project-colonization/shared/assimp/test/gtest/tmp/gtest-gitclone.cmake")
execute_process(
  COMMAND ${command}
  RESULT_VARIABLE result
  OUTPUT_FILE "/Users/Ye-Macbook/Documents/Development/project-colonization/shared/assimp/test/gtest/src/gtest-stamp/gtest-download-out.log"
  ERROR_FILE "/Users/Ye-Macbook/Documents/Development/project-colonization/shared/assimp/test/gtest/src/gtest-stamp/gtest-download-err.log"
  )
if(result)
  set(msg "Command failed: ${result}\n")
  foreach(arg IN LISTS command)
    set(msg "${msg} '${arg}'")
  endforeach()
  set(msg "${msg}\nSee also\n  /Users/Ye-Macbook/Documents/Development/project-colonization/shared/assimp/test/gtest/src/gtest-stamp/gtest-download-*.log")
  message(FATAL_ERROR "${msg}")
else()
  set(msg "gtest download command succeeded.  See also /Users/Ye-Macbook/Documents/Development/project-colonization/shared/assimp/test/gtest/src/gtest-stamp/gtest-download-*.log")
  message(STATUS "${msg}")
endif()
