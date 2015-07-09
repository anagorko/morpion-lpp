configure_file(dot.py . COPYONLY)
configure_file(hull.py . COPYONLY)

add_test(dot dot.py)
add_test(hull hull.py)
