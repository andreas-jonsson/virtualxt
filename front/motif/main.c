// Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    Portions Copyright (c) 2019-2023 Andreas T Jonsson <mail@andreasjonsson.se>
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/DrawingA.h>
#include <Xm/RowColumn.h>

static void pushed_button(Widget widget, XtPointer client_data, XtPointer call_data) {
    (void)widget; (void)client_data; (void)call_data;
    printf("Hello to you too!\n");
}

static void drawing_area_callback(Widget widget, XtPointer client_data, XtPointer call_data) {
    (void)widget; (void)client_data; (void)call_data;
}

int main(int argc, char *argv[]) {
    XtAppContext app;
    Widget top_widget = XtVaAppInitialize(&app, "VirtualXT", NULL, 0, &argc, argv, NULL, NULL);
    Widget main_window = XtVaCreateManagedWidget("main_window",
        xmMainWindowWidgetClass,   top_widget,
        XmNscrollBarDisplayPolicy, XmAS_NEEDED,
        XmNscrollingPolicy,        XmAUTOMATIC,
        NULL);

    Widget rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, main_window, XmNorientation, XmVERTICAL, NULL);
    XtManageChild(rowcol);

    XmString file = XmStringCreateLocalized("File");
    XmString edit = XmStringCreateLocalized("Edit");
    XmString help = XmStringCreateLocalized("Help");
    Widget menubar = XmVaCreateSimpleMenuBar(main_window, "menubar",
        XmVaCASCADEBUTTON, file, 'F',
        XmVaCASCADEBUTTON, edit, 'E',
        XmVaCASCADEBUTTON, help, 'H',
        NULL);
    XmStringFree(file);
    XmStringFree(edit);
    XmStringFree(help);
    XtManageChild(menubar);

    Widget button = XmCreatePushButton(rowcol, "Hello World! Push me!", NULL, 0);
    XtAddCallback(button, XmNactivateCallback, &pushed_button, NULL);
    XtManageChild(button);

    Widget drawing_a = XtVaCreateWidget("drawing_a", xmDrawingAreaWidgetClass, rowcol, NULL);
    XtAddCallback(drawing_a, XmNinputCallback, &drawing_area_callback, NULL);
    XtAddCallback(drawing_a, XmNexposeCallback, &drawing_area_callback, NULL);
    XtManageChild(drawing_a);

    XtRealizeWidget(top_widget);
    XtAppMainLoop(app);
    return 0;
}
