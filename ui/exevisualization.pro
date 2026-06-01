QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h

FORMS += \
    mainwindow.ui \
    $$PWD/../knowledgebook/knowledgebookwidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

OTHER_FILES += \
    icons/arrow_back_24dp_E3E3E3_FILL0_wght400_GRAD0_opsz24.svg \
RESOURCES += \
    res.qrc
INCLUDEPATH += $$PWD/../code_analysis/include
DEPENDPATH += $$PWD/../code_analysis/include

INCLUDEPATH += $$PWD/../knowledgebook
DEPENDPATH += $$PWD/../knowledgebook

# 2. 把队友的所有头文件加进来（用于在 Qt Creator 左侧显示）
HEADERS += \
    $$PWD/../code_analysis/include/analyzer.h \
    $$PWD/../code_analysis/include/ast.h \
    $$PWD/../code_analysis/include/binding.h \
    $$PWD/../code_analysis/include/class_model.h \
    $$PWD/../code_analysis/include/executor.h \
    $$PWD/../code_analysis/include/lexer.h \
    $$PWD/../code_analysis/include/memory.h \
    $$PWD/../code_analysis/include/parser.h \
    $$PWD/../code_analysis/include/symbol_table.h \
    $$PWD/../code_analysis/include/type_system.h \
    $$PWD/../code_analysis/include/types.h \
    $$PWD/../code_analysis/include/value.h \
    $$PWD/../code_analysis/include/visualization_data.h \
    $$PWD/../knowledgebook/knowledgebookwidget.h \
    $$PWD/../knowledgebook/markdownparser.h
SOURCES += \
    $$PWD/../code_analysis/src/analyzer.cpp \
    $$PWD/../code_analysis/src/ast.cpp \
    $$PWD/../code_analysis/src/class_model.cpp \
    $$PWD/../code_analysis/src/executor.cpp \
    $$PWD/../code_analysis/src/lexer.cpp \
    $$PWD/../code_analysis/src/memory.cpp \
    $$PWD/../code_analysis/src/parser.cpp \
    $$PWD/../code_analysis/src/symbol_table.cpp \
    $$PWD/../code_analysis/src/type_system.cpp \
    $$PWD/../code_analysis/src/types.cpp \
    $$PWD/../code_analysis/src/value.cpp \
    $$PWD/../knowledgebook/knowledgebookwidget.cpp \
    $$PWD/../knowledgebook/markdownparser.cpp