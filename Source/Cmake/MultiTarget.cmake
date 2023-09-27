# Multitarget fixes
macro(MultiTargetFix PROJECT NAME OUT)
    set(CONFIS DEBUG RELEASE MINSIZEREL RELWITHDEBINFO)

    foreach (p IN LISTS CONFIS)
        string(TOLOWER ${p} P_LOWER)
        set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${p} ${OUT} RUNTIME_OUTPUT_NAME_${p} ${NAME}${P_LOWER})
    endforeach()
endmacro()
