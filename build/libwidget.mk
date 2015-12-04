WIDGET_SOURCES = \
	$(SRC)/Widget/Widget.cpp \
	$(SRC)/Widget/ActionWidget.cpp \
	$(SRC)/Widget/CallbackWidget.cpp \
	$(SRC)/Widget/WindowWidget.cpp \
	$(SRC)/Widget/CreateWindowWidget.cpp \
	$(SRC)/Widget/ListWidget.cpp \
	$(SRC)/Widget/TextListWidget.cpp \
	$(SRC)/Widget/ContainerWidget.cpp \
	$(SRC)/Widget/SolidWidget.cpp \
	$(SRC)/Widget/PanelWidget.cpp \
	$(SRC)/Widget/TabWidget.cpp \
	$(SRC)/Widget/TextWidget.cpp \
	$(SRC)/Widget/LargeTextWidget.cpp \
	$(SRC)/Widget/OverlappedWidget.cpp \
	$(SRC)/Widget/TwoWidgets.cpp \
	$(SRC)/Widget/RowFormWidget.cpp \
	$(SRC)/Widget/EditRowFormWidget.cpp \
	$(SRC)/Widget/ProfileRowFormWidget.cpp \
	$(SRC)/Widget/UnitRowFormWidget.cpp \
	$(SRC)/Widget/ManagedWidget.cpp \
	$(SRC)/Widget/PagerWidget.cpp \
	$(SRC)/Widget/ArrowPagerWidget.cpp \
	$(SRC)/Widget/OffsetButtonsWidget.cpp \
	$(SRC)/Widget/ButtonPanelWidget.cpp \
	$(SRC)/Widget/ButtonWidget.cpp \
	$(SRC)/Widget/QuestionWidget.cpp \
	$(SRC)/Widget/KeyboardWidget.cpp \
	$(SRC)/Widget/ViewImageWidget.cpp \
	$(SRC)/Widget/DockWindow.cpp

WIDGET_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,libwidget,WIDGET))
