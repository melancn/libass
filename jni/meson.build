if host_machine.system() != 'android'
    if get_option('jni').enabled()
        error('The jni option is only supported when building for Android.')
    endif
    subdir_done()
endif

static_library(
    'ass_jni',
    'ass_jni.c',
    include_directories: incs,
    dependencies: libass_dep,
)
