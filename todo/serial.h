
print(
    string_view message="", /* array of lines separated with \n */
    message_width=0,        /* meaning no target width */
    edge_character="|",     /* char at the ends of the line */
    text_allign="l",             /* l, r, c allign text */
    min_margin_l=0,         /* min margin on left if text does not fit, there will be next line */
    min_margin_r=0,         /* min margin on right if text does not fit, there will be next line */
    end=""
)

println(
    // same params
    end="\n"
)

printf(
    ...// not sure how to do this one
)