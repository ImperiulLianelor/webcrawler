add_executable(webcrawler main.c)
set_property(TARGET webcrawler PROPERTY C_STANDARD 99)
install(TARGETS webcrawler DESTINATION executable PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
