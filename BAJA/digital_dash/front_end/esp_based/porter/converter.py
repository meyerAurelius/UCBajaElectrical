import os
import re
import curses
import sys

FUNCTIONS_TO_REMOVE = {
    "lv_obj_set_name_static",
    "lv_obj_set_name",
}

EXCLUDED_DIRS = {"lib"}

CREATE_RULES = {
    "button_create": {
        "new_func": "lv_button_create",
        "keep_arg_indices": [0],   # keep only parent
        "post_lines": [
            'lv_obj_t * {var}_label = lv_label_create({var});',
            'lv_label_set_text({var}_label, {arg1});',
            'lv_obj_center({var}_label);',
        ],
    },
    "slider_create": {
        "new_func": "lv_slider_create",
        "keep_arg_indices": [0],
        "post_lines": [],
    },
    "arc_create": {
        "new_func": "lv_arc_create",
        "keep_arg_indices": [0],
        "post_lines": [],
    },
    "h4_create": {
        "new_func": "lv_label_create",
        "keep_arg_indices": [0],
        "post_lines": [
            'lv_label_set_text({var}, {arg1});',
        ],
    },
    "h3_create": {
        "new_func": "lv_label_create",
        "keep_arg_indices": [0],
        "post_lines": [
            'lv_label_set_text({var}, {arg1});',
        ],
    },
    "value_large_create": {
        "new_func": "lv_label_create",
        "keep_arg_indices": [0],
        "post_lines": [
            'lv_label_set_text({var}, "VALUE");',
        ],
    },
}


def remove_functions(source_text, functions):
    """
    Remove full statements that call specific functions,
    replacing them with newline+tab to preserve formatting.
    """
    for func in functions:
        pattern = rf"[ \t]*{re.escape(func)}\s*\([^;]*\);\s*"
        source_text = re.sub(
            pattern,
            "\n\t",
            source_text,
            flags=re.MULTILINE | re.DOTALL
        )
    return source_text

def split_args(arg_string):
    args = []
    current = []
    depth = 0
    in_string = False
    escape = False

    for ch in arg_string:
        if in_string:
            current.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
            continue

        if ch == '"':
            in_string = True
            current.append(ch)
        elif ch == '(':
            depth += 1
            current.append(ch)
        elif ch == ')':
            depth -= 1
            current.append(ch)
        elif ch == ',' and depth == 0:
            args.append(''.join(current).strip())
            current = []
        else:
            current.append(ch)

    if current:
        args.append(''.join(current).strip())

    return args

def transform_create_calls(source_text):
    """
    Transform lines like:
        lv_obj_t * arc_1 = arc_create(lv_obj_0, &subject_test);

    into:
        lv_obj_t * arc_1 = lv_arc_create(lv_obj_0);

    and optionally add post-processing lines like:
        lv_label_set_text(h4_1, "Speed");
    """

    pattern = re.compile(
        r'(?P<indent>[ \t]*)'
        r'lv_obj_t\s*\*\s*(?P<var>\w+)\s*=\s*'
        r'(?P<func>\w+)\s*'
        r'\((?P<args>.*?)\)\s*;',
        re.MULTILINE | re.DOTALL
    )

    def repl(match):
        indent = match.group("indent")
        var_name = match.group("var")
        old_func = match.group("func")
        args_raw = match.group("args")

        if old_func not in CREATE_RULES:
            return match.group(0)

        rule = CREATE_RULES[old_func]
        args = split_args(args_raw)

        kept_args = []
        for idx in rule["keep_arg_indices"]:
            if idx < len(args):
                kept_args.append(args[idx])

        new_call = (
            f'{indent}lv_obj_t * {var_name} = '
            f'{rule["new_func"]}({", ".join(kept_args)});'
        )

        post_lines = []
        for tmpl in rule["post_lines"]:
            line = tmpl.format(
                var=var_name,
                args=", ".join(args),
                **{f"arg{i}": args[i] if i < len(args) else '""' for i in range(len(args))}
            )
            post_lines.append(indent + line)

        if post_lines:
            return new_call + "\n" + "\n".join(post_lines)
        return new_call

    return re.sub(pattern, repl, source_text)

def port_functions(source_text):
    """
    Port supported create calls and remove unwanted calls.
    """
    source_text = remove_functions(source_text, FUNCTIONS_TO_REMOVE)
    source_text = transform_create_calls(source_text)
    return source_text

def extract_main_create_function(source_text):
    """
    Extract only:
        lv_obj_t * main_create(void) { ... }

    Returns the full function text, or None if not found.
    """
    signature_pattern = r"lv_obj_t\s*\*\s*main_create\s*\(\s*void\s*\)"
    match = re.search(signature_pattern, source_text)

    if not match:
        return None

    sig_start = match.start()
    search_pos = match.end()

    # Find the first opening brace after the signature
    brace_start = source_text.find("{", search_pos)
    if brace_start == -1:
        return None

    depth = 0
    i = brace_start

    while i < len(source_text):
        ch = source_text[i]

        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source_text[sig_start:i + 1]

        i += 1

    return None


def port_file(input_path, output_path):
    with open(input_path, "r", encoding="utf-8") as f:
        source = f.read()

    main_create_text = extract_main_create_function(source)
    if main_create_text is None:
        raise ValueError("Could not find function: lv_obj_t * main_create(void)")

    new_source = port_functions(main_create_text)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(new_source + "\n")


def find_c_files(root_dir):
    """
    Recursively find all .c files under root_dir,
    excluding anything inside a lib/ directory.
    """
    matches = []

    for dirpath, dirnames, filenames in os.walk(root_dir):
        dirnames[:] = [d for d in dirnames if d not in EXCLUDED_DIRS]

        for filename in filenames:
            if filename.endswith(".c"):
                full_path = os.path.join(dirpath, filename)
                rel_path = os.path.relpath(full_path, root_dir)
                matches.append(rel_path)

    matches.sort()
    return matches


def draw_menu(stdscr, files, selected_idx, top_idx, root_dir):
    stdscr.clear()
    height, width = stdscr.getmaxyx()

    title = f"Select a .c file to port from: {root_dir}"
    help_line = "Use UP/DOWN arrows to move, ENTER to select, q to quit"

    stdscr.addnstr(0, 0, title, width - 1)
    stdscr.addnstr(1, 0, help_line, width - 1)

    visible_rows = height - 4
    for row in range(visible_rows):
        file_idx = top_idx + row
        if file_idx >= len(files):
            break

        file_path = files[file_idx]
        y = row + 3

        if file_idx == selected_idx:
            stdscr.attron(curses.A_REVERSE)
            stdscr.addnstr(y, 0, file_path, width - 1)
            stdscr.attroff(curses.A_REVERSE)
        else:
            stdscr.addnstr(y, 0, file_path, width - 1)

    status = f"{selected_idx + 1}/{len(files)}"
    stdscr.addnstr(height - 1, max(0, width - len(status) - 1), status, len(status))
    stdscr.refresh()


def select_file_curses(stdscr, files, root_dir):
    curses.curs_set(0)
    stdscr.keypad(True)

    selected_idx = 0
    top_idx = 0

    while True:
        height, _ = stdscr.getmaxyx()
        visible_rows = max(1, height - 4)

        if selected_idx < top_idx:
            top_idx = selected_idx
        elif selected_idx >= top_idx + visible_rows:
            top_idx = selected_idx - visible_rows + 1

        draw_menu(stdscr, files, selected_idx, top_idx, root_dir)

        key = stdscr.getch()

        if key in (curses.KEY_UP, ord("k")):
            if selected_idx > 0:
                selected_idx -= 1
        elif key in (curses.KEY_DOWN, ord("j")):
            if selected_idx < len(files) - 1:
                selected_idx += 1
        elif key in (10, 13, curses.KEY_ENTER):
            return files[selected_idx]
        elif key in (ord("q"), 27):
            return None


def choose_file(root_dir):
    files = find_c_files(root_dir)

    if not files:
        print(f"No .c files found under: {root_dir}")
        return None

    return curses.wrapper(select_file_curses, files, root_dir)


def make_output_path(input_path):
    base, ext = os.path.splitext(input_path)
    return f"{base}_ported{ext}"


def main():
    if len(sys.argv) > 1:
        root_dir = sys.argv[1]
    else:
        root_dir = input("Enter directory to search for .c files: ").strip()

    if not root_dir:
        print("No directory specified.")
        sys.exit(1)

    root_dir = os.path.abspath(root_dir)

    if not os.path.isdir(root_dir):
        print(f"Not a valid directory: {root_dir}")
        sys.exit(1)

    selected_rel_path = choose_file(root_dir)

    if selected_rel_path is None:
        print("No file selected.")
        sys.exit(0)

    input_path = os.path.join(root_dir, selected_rel_path)
    output_path = make_output_path(input_path)

    print(f"Selected: {selected_rel_path}")

    try:
        port_file(input_path, output_path)
    except ValueError as e:
        print(f"Error: {e}")
        sys.exit(1)

    print(f"Ported file written to: {os.path.relpath(output_path, root_dir)}")


if __name__ == "__main__":
    main()