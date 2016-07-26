include(FindPackageHandleStandardArgs)

set(_programfiles "")
foreach(v "ProgramW6432" "ProgramFiles" "ProgramFiles(x86)")
  if(DEFINED "ENV{${v}}")
    file(TO_CMAKE_PATH "$ENV{${v}}" _env_programfiles)
    list(APPEND _programfiles "${_env_programfiles}/Windows Kits/8.1/bin/x86")
    unset(_env_programfiles)
  endif()
endforeach()

# Can't use "$ENV{ProgramFiles(x86)}" to avoid violating CMP0053.  See
# http://public.kitware.com/pipermail/cmake-developers/2014-October/023190.html
set (ProgramFiles_x86 "ProgramFiles(x86)")
if ("$ENV{${ProgramFiles_x86}}")
	set (ProgramFiles "$ENV{${ProgramFiles_x86}}")
else ()
	set (ProgramFiles "$ENV{ProgramFiles}")
endif ()

MESSAGE("Trying to find ${ProgramFiles}/Windows Kits/8.1/bin/x86 ${ProgramFiles}/Windows Kits/8.0/bin/x86")

find_program (DirectX_FXC_EXECUTABLE fxc
	HINTS ${_programfiles}
	
	DOC "Path to fxc.exe executable."
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(FXC 
    DEFAULT_MSG 
    DirectX_FXC_EXECUTABLE 
)
	
