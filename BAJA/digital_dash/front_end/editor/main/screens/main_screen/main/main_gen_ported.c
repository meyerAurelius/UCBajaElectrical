lv_obj_t * main_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);

	lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));

    lv_obj_t * button_1 = lv_button_create(lv_obj_0);
    lv_obj_t * button_1_label = lv_label_create(button_1);
    lv_label_set_text(button_1_label, "Settings");
    lv_obj_center(button_1_label);

	lv_obj_set_x(button_1, 228);
    lv_obj_set_y(button_1, 5);
    
    lv_obj_t * temp_slide = lv_slider_create(lv_obj_0);

	lv_obj_set_x(temp_slide, 7);
    lv_obj_set_y(temp_slide, 213);
    lv_obj_set_width(temp_slide, 131);
    lv_obj_set_height(temp_slide, 18);
    lv_obj_set_style_transform_rotation(temp_slide, -900, 0);
    
    lv_obj_t * h4_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(h4_1, "CVT   Temperature");

	lv_obj_set_x(h4_1, 10);
    lv_obj_set_y(h4_1, 219);
    
    lv_obj_t * arc_1 = lv_arc_create(lv_obj_0);

	lv_obj_set_x(arc_1, 144);
    lv_obj_set_y(arc_1, 75);
    lv_obj_set_width(arc_1, 170);
    lv_obj_set_height(arc_1, 162);
    
    lv_obj_t * h3_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(h3_1, "Speed");

	lv_obj_set_x(h3_1, 200);
    lv_obj_set_y(h3_1, 219);
    
    lv_obj_t * value_large_1 = lv_label_create(lv_obj_0);
    lv_label_set_text(value_large_1, "VALUE");

	lv_obj_set_x(value_large_1, 38);
    lv_obj_set_y(value_large_1, 160);
    lv_obj_set_width(value_large_1, 79);
    lv_obj_set_height(value_large_1, 48);
    
    lv_obj_t * value_large_2 = lv_label_create(lv_obj_0);
    lv_label_set_text(value_large_2, "VALUE");

	lv_obj_set_x(value_large_2, 178);
    lv_obj_set_y(value_large_2, 130);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}
