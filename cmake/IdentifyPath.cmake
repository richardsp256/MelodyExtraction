# The following function checks if a given variable (passed as PATH_VAR) has
# been initialized and that it's value refers an existing directory. If the
# specified path is not a relative path the value of the variable is modified
# to be an absolute path. If the specified path cannot be found, an error is
# raised.
#
# If the variable has not been initialized, then the function attempts to
# assign it a value matched with the pattern passed to DEFAULT_GLOB.
#
# The value passed to DEFAULT_GLOB must actually be a value, and not a variable
function(IDENTIFY_PATH PATH_VAR DEFAULT_GLOB)

  if(${PATH_VAR})
    # PATH_VAR had an initial value. Get the absolute path

    if( NOT IS_ABSOLUTE ${${PATH_VAR}})
      get_filename_component(ABSPATH ${${PATH_VAR}} ABSOLUTE)
    else()
      SET(ABSPATH ${${PATH_VAR}})
    endif()

    # check that the path exists and then ensure that PATH_VAR points to the
    # absolute path
    if (IS_DIRECTORY ${ABSPATH})
      SET(${PATH_VAR} ${ABSPATH} PARENT_SCOPE)
    else()
      message(FATAL_ERROR
	"The path set for ${PATH_VAR}, ${${PATH_VAR}}, doesn't exist")
    endif()

  else ()
    # PATH_VAR is not defined, try to set it by matching DEFAULT_GLOB
    FILE(GLOB NEW_PATH ${DEFAULT_GLOB} ONLYDIR)

    # Check if the file matching was a success:
    if (IS_DIRECTORY ${NEW_PATH})
      # set ${PATH_VAR} equal to the identified path
      SET(${PATH_VAR} ${NEW_PATH} PARENT_SCOPE)
    endif()

  endif()

endfunction(IDENTIFY_PATH)
