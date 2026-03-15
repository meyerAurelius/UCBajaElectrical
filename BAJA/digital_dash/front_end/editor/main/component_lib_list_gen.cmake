

set(LVGL_COMPONENT_LIB_NAME "LIB")
add_subdirectory(lib)
list(
  APPEND
  LV_EDITOR_COMPONENT_LIB_LIST
  LIB
)