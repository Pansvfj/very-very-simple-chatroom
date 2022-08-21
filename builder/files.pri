#headers
files = $$files($$_PRO_FILE_PWD_/*.h, true)
for (f, files) {
    HEADERS += $$f
}

#source
files = $$files($$_PRO_FILE_PWD_/*.c, true)
for (f, files) {
    SOURCES += $$f
}
