#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#ifndef EDITOR_X_PERCENT
#define EDITOR_X_PERCENT (0.25f)
#endif

typedef s32 Editor_Tab_Type;
enum
{
    Editor_Tab_Type_Settings,
    Editor_Tab_Type_Brushes
};

Editor_Tab_Type editor_ui_do_tabs();
void editor_ui_do_settings();
void editor_ui_do_brushes();
void editor_ui_do_footer();

void editor_ui_do_main();

#endif //EDITOR_UI_H
