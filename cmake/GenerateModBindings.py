# This only finds the names of public methods and constructors since ModBindings.h lets the compiler work out the types
import os
import re
import sys

source_dir, output_file, stub_file = sys.argv[1], sys.argv[2], sys.argv[3]
excluded = re.compile(r"^(server|ui/imgui|mod)/")

access_label = re.compile(r"^\s*(public|private|protected)\s*:(?!:)")
class_head = re.compile(
    r"^(template\s*<(.*)>\s*)?(class|struct)\s+(\w+)(\s+final)?\s*(:(.*))?$", re.S
)
nested_head = re.compile(r"^(enum(\s+class|\s+struct)?|class|struct|union)\s+(\w+)")
alias_head = re.compile(r"^(using\s+(\w+)\s*=|typedef\s.*?(\w+)$)", re.S)
definition = re.compile(r"^(?=[^\s#/])[^;]*?\b(\w+)(?:<[^>]*>)?::(~?\w+)\s*\([^;]*$", re.M)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def strip_conditionals(text):
    # Anything inside #if blocks may not exist on this platform, so we leave it out entirely
    lines = []
    depth = 0
    for line in text.split("\n"):
        directive = line.strip()
        if directive.startswith("#"):
            if re.match(r"#\s*if", directive):
                depth += 1
            elif re.match(r"#\s*endif", directive):
                depth -= 1
            lines.append("")
        else:
            lines.append(line if depth == 0 else "")
    return "\n".join(lines)


def statements(text):
    """Splits text into (head, body) pairs at brace depth 0, body is None for plain ; statements."""
    result = []
    head = []
    i = 0
    while i < len(text):
        c = text[i]
        if c == ";":
            result.append(("".join(head).strip(), None))
            head = []
        elif c == "{":
            depth = 1
            start = i + 1
            while depth > 0 and i + 1 < len(text):
                i += 1
                if text[i] == "{":
                    depth += 1
                elif text[i] == "}":
                    depth -= 1
            result.append(("".join(head).strip(), text[start:i]))
            head = []
            if i + 1 < len(text) and text[i + 1 :].lstrip().startswith(";"):
                i = text.index(";", i + 1)
        else:
            head.append(c)
        i += 1
    return result


def balanced(text):
    depth = 0
    for c in text:
        if c in "<([":
            depth += 1
        elif c in ">)]":
            depth -= 1
        if depth < 0:
            return False
    return depth == 0


def split_parameters(text):
    parts = []
    depth = 0
    current = ""
    for c in text:
        if c in "<([{":
            depth += 1
        elif c in ">)]}":
            depth -= 1
        if c == "," and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += c
    if current.strip():
        parts.append(current)
    return [part.strip() for part in parts]


def parse_function(head, class_name):
    head = " ".join(head.split())
    head = re.sub(r"\[\[.*?\]\]", "", head).strip()
    if re.match(r"^(template|friend|using|typedef)\b", head) or "operator" in head:
        return None

    depth = 0
    open_index = -1
    for i, c in enumerate(head):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "(" and depth == 0:
            open_index = i
            break
    if open_index < 0:
        return None

    depth = 0
    close_index = -1
    for i in range(open_index, len(head)):
        if head[i] == "(":
            depth += 1
        elif head[i] == ")":
            depth -= 1
            if depth == 0:
                close_index = i
                break
    if close_index < 0:
        return None

    match = re.match(r"^(.*?)(~?\w+)\s*$", head[:open_index])
    if not match:
        return None
    prefix, name = match.group(1), match.group(2)
    words = prefix.split()
    is_static = "static" in words
    is_virtual = "virtual" in words
    return_type = " ".join(
        word
        for word in words
        if word
        not in ("static", "virtual", "inline", "constexpr", "explicit", "consteval")
    )

    suffix = head[close_index + 1 :].strip()
    pure = deleted = defaulted = False
    assignment = re.search(r"=\s*(0|default|delete)$", suffix)
    if assignment:
        pure = assignment.group(1) == "0"
        deleted = assignment.group(1) == "delete"
        defaulted = assignment.group(1) == "default"
        suffix = suffix[: assignment.start()].strip()
    qualifiers = suffix.split()
    if any(
        word not in ("const", "noexcept", "override", "final") for word in qualifiers
    ):
        return None

    parameters = head[open_index + 1 : close_index].strip()
    if "..." in parameters or name.startswith("~"):
        return None

    constructor = name == class_name and return_type == ""
    if not constructor and (
        return_type == "" or not balanced(return_type) or "auto" in return_type.split()
    ):
        return None

    return {
        "name": name,
        "return": return_type,
        "parameters": parameters,
        "static": is_static,
        "const": "const" in qualifiers,
        "virtual": is_virtual
        or "override" in qualifiers
        or "final" in qualifiers
        or pure,
        "deleted": deleted,
        "defaulted": defaulted,
        "constructor": constructor,
    }


def parse_class(name, kind, body):
    access = "public" if kind == "struct" else "private"
    methods = []
    nested = {}
    for head, member_body in statements(body):
        while True:
            label = access_label.match(head)
            if not label:
                break
            access = label.group(1)
            head = head[label.end() :].strip()

        nested_match = nested_head.match(head)
        if nested_match and "(" not in head:
            nested[nested_match.group(3)] = access
            continue

        alias = alias_head.match(head)
        if alias and member_body is None:
            nested[alias.group(2) or alias.group(3)] = access
            continue

        if "(" not in head:
            continue

        function = parse_function(head, name)
        if function:
            function["access"] = access
            function["inline"] = member_body is not None
            methods.append(function)
    return {"name": name, "kind": kind, "methods": methods, "nested": nested}


def parameter_types(text, resolve, replacements):
    """Returns the parameter types without names or default values, or None when we can't spell them."""
    types = []
    for type_name, _ in parameter_list(text, replacements):
        if type_name is None:
            return None
        type_name = resolve(type_name)
        if type_name is None:
            return None
        types.append(type_name)
    return types


def parameter_list(text, replacements):
    """Returns (type, name) pairs with default values removed, type is None when we can't split it."""
    pairs = []
    for parameter in split_parameters(text):
        depth = 0
        for i, c in enumerate(parameter):
            if c in "<([":
                depth += 1
            elif c in ">)]":
                depth -= 1
            elif c == "=" and depth == 0:
                parameter = parameter[:i].strip()
                break
        if parameter == "void":
            continue
        match = re.match(r"^(.*?[\s*&>])(\w+)$", parameter)
        if "(" in parameter or "[" in parameter or not match:
            pairs.append((None, "arg%d" % (len(pairs) + 1)))
            continue
        type_name = match.group(1).strip()
        for name, replacement in replacements.items():
            type_name = re.sub(r"(?<![\w:])" + name + r"\b", replacement, type_name)
        pairs.append((type_name, match.group(2)))
    return pairs


def read_source(path):
    with open(
        os.path.join(source_dir, path), encoding="utf-8", errors="replace"
    ) as stream:
        return strip_conditionals(strip_comments(stream.read()))


sources = []
for root, _, files in os.walk(source_dir):
    for file in files:
        path = os.path.relpath(os.path.join(root, file), source_dir).replace(
            os.sep, "/"
        )
        if (path.endswith(".cpp") or path.endswith(".h")) and not path.startswith(
            "server/"
        ):
            sources.append(path)
sources.sort()

defined = set()
for path in sources:
    for match in definition.finditer(read_source(path)):
        defined.add(match.group(1) + "::" + match.group(2))

builtins = {
    "const",
    "volatile",
    "unsigned",
    "signed",
    "void",
    "bool",
    "char",
    "wchar_t",
    "char8_t",
    "char16_t",
    "char32_t",
    "short",
    "int",
    "long",
    "float",
    "double",
    "size_t",
    "ptrdiff_t",
    "intptr_t",
    "uintptr_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
}
global_types = set()
enums = set()
classes = {}
templates = {}
duplicates = set()
class_headers = {}
for header in sources:
    if not header.endswith(".h") or excluded.match(header):
        continue
    text = read_source(header)
    enums.update(re.findall(r"\benum(?:\s+class|\s+struct)?\s+(\w+)", text))
    for head, body in statements(text):
        flat = " ".join(head.split())
        declared = re.match(
            r"^(template\s*<.*>\s*)?(enum(\s+class|\s+struct)?|class|struct|union)\s+(\w+)",
            flat,
        )
        if declared:
            global_types.add(declared.group(4))
        alias = alias_head.match(flat)
        if alias and body is None:
            global_types.add(alias.group(2) or alias.group(3))
        if body is None:
            continue
        match = class_head.match(flat)
        if not match:
            continue

        name = match.group(4)
        parsed = parse_class(name, match.group(3), body)
        parsed["bases"] = split_parameters(match.group(7) or "")
        if match.group(1):
            parsed["parameters"] = [
                re.sub(r"^(typename|class)\s+", "", p).split("=")[0].strip()
                for p in split_parameters(match.group(2))
            ]
            templates[name] = parsed
        elif name in classes:
            duplicates.add(name)
        else:
            classes[name] = parsed
            class_headers[name] = header

for name in duplicates:
    del classes[name]


def base_name(base):
    return re.sub(
        r"<.*$", "", re.sub(r"^((public|protected|private|virtual)\s+)*", "", base)
    ).strip()


def find_nested(class_name, name, seen):
    """Returns (owner, accessible) for a nested type visible from class_name, or None."""
    parsed = classes.get(class_name) or templates.get(class_name)
    if not parsed or class_name in seen:
        return None
    seen.add(class_name)
    if name in parsed["nested"]:
        return class_name, parsed["nested"][name] == "public"
    for base in parsed["bases"]:
        found = find_nested(base_name(base), name, seen)
        if found:
            public = base.startswith("public") or (
                parsed["kind"] == "struct"
                and not re.match(r"^(private|protected)\b", base)
            )
            return found[0], found[1] and public
    return None


def resolver(class_name):
    def resolve(type_name):
        result = ""
        position = 0
        for match in re.finditer(r"(?<![\w:])([A-Za-z_]\w*)", type_name):
            identifier = match.group(1)
            result += type_name[position : match.start()]
            position = match.end()
            if identifier in builtins:
                result += identifier
                continue
            found = find_nested(class_name, identifier, set())
            if found:
                if not found[1]:
                    return None
                result += found[0] + "::" + identifier
            elif identifier in global_types or type_name[
                match.end() :
            ].lstrip().startswith("::"):
                result += identifier
            else:
                return None
        return result + type_name[position:]

    return resolve


def describe(class_name, method, overloaded, replacements):
    if not overloaded:
        return 'ModBind<&%s::%s>::Describe("%s")' % (
            class_name,
            method["name"],
            method["name"],
        )

    types = parameter_types(method["parameters"], resolver(class_name), replacements)
    if types is None:
        return None
    pick = "Static" if method["static"] else "Const" if method["const"] else "Member"
    return 'ModBind<ModPick<%s>::%s(&%s::%s)>::Describe("%s")' % (
        ", ".join(types),
        pick,
        class_name,
        method["name"],
        method["name"],
    )


def bindable(owner, method):
    if method["constructor"] or method["access"] != "public" or method["deleted"]:
        return False
    return (
        method["inline"]
        or method["virtual"]
        or owner + "::" + method["name"] in defined
    )


def overload_counts(parsed):
    counts = {}
    for method in parsed["methods"]:
        counts[method["name"]] = counts.get(method["name"], 0) + 1
    return counts


lua_keywords = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in",
                "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"}


def lua_type(type_name, parameter):
    if type_name is None:
        return "any"
    words = [word for word in re.findall(r"[\w:]+|[*&]+", type_name) if word not in ("const", "volatile")]
    pointers = sum(word.count("*") for word in words)
    reference = any("&" in word for word in words) and "const" not in type_name.split()
    base = " ".join(word for word in words if word[0] not in "*&")
    plain = base.split("::")[-1]
    if base in ("std::string", "std::wstring") and pointers <= 1:
        return "string"
    if plain in ("char", "wchar_t") and pointers == 1:
        return "string"
    if pointers or reference:
        if pointers == 0 and parameter and plain in from_string:
            return plain + "|string"
        if pointers <= 1 and plain in classes:
            return plain
        return "ffi.cdata*"
    if plain == "bool":
        return "boolean"
    if plain in builtins or plain in enums:
        return "number"
    if plain in classes:
        return plain + "|string" if parameter and plain in from_string else plain
    return "ffi.cdata*"


def stub(class_name, method, replacements):
    stub_lines = []
    names = []
    for type_name, parameter in parameter_list(method["parameters"], replacements):
        if parameter in lua_keywords:
            parameter += "Value"
        names.append(parameter)
        stub_lines.append("---@param %s %s" % (parameter, lua_type(type_name, True)))
    result = method["return"]
    for template, replacement in replacements.items():
        result = re.sub(r"(?<![\w:])" + template + r"\b", replacement, result)
    if result != "void":
        stub_lines.append("---@return %s" % lua_type(result, False))
    stub_lines.append(
        "function %s%s%s(%s) end"
        % (class_name, "." if method["static"] else ":", method["name"], ", ".join(names))
    )
    return stub_lines


# Lua strings convert to any class with a public constructor taking one string
from_string = set()
for name, parsed in classes.items():
    for method in parsed["methods"]:
        pairs = parameter_list(method["parameters"], {})
        if (
            method["constructor"]
            and method["access"] == "public"
            and len(pairs) == 1
            and lua_type(pairs[0][0], False) == "string"
        ):
            from_string.add(name)

entries = []
stub_entries = []
used_headers = set()
for name in sorted(classes):
    parsed = classes[name]
    counts = overload_counts(parsed)
    lines = []
    stubs = []
    for method in parsed["methods"]:
        if bindable(name, method):
            line = describe(name, method, counts[method["name"]] > 1, {})
            if line:
                lines.append(line)
                stubs.append(stub(name, method, {}))

    constructors = [method for method in parsed["methods"] if method["constructor"]]
    for method in constructors:
        if method["access"] != "public" or method["deleted"]:
            continue
        if (
            not method["inline"]
            and not method["defaulted"]
            and name + "::" + name not in defined
        ):
            continue
        types = parameter_types(method["parameters"], resolver(name), {})
        if types is None or (
            len(types) == 1 and re.match(r"^(const\s+)?" + name + r"\s*&&?$", types[0])
        ):
            continue
        lines.append("ModConstruct<%s>::Describe()" % ", ".join([name] + types))
    if not constructors:
        lines.append("ModConstruct<%s>::Describe()" % name)

    # We only follow the first base since a pointer to the object is only a valid pointer to that one
    bases = []
    if parsed["bases"]:
        first = parsed["bases"][0]
        public = first.startswith("public") or (
            parsed["kind"] == "struct"
            and not re.match(r"^(private|protected)\b", first)
        )
        base = re.sub(r"^((public|protected|private|virtual)\s+)*", "", first).strip()
        template = re.match(r"^(\w+)\s*<(.*)>$", base)
        if public and base in classes:
            bases.append(base)
        elif public and template and template.group(1) in templates:
            parent = templates[template.group(1)]
            arguments = split_parameters(template.group(2))
            if len(arguments) == len(parent["parameters"]):
                replacements = dict(zip(parent["parameters"], arguments))
                parent_counts = overload_counts(parent)
                for method in parent["methods"]:
                    if method["name"] not in counts and bindable(
                        parent["name"], method
                    ):
                        line = describe(
                            name,
                            method,
                            parent_counts[method["name"]] > 1,
                            replacements,
                        )
                        if line:
                            lines.append(line)
                            stubs.append(stub(name, method, replacements))

    used_headers.add(class_headers[name])
    entries.append((name, ",".join(bases), lines))
    stub_entries.append((name, bases, stubs))

output = ['#include "mod/ModBindings.h"', "", "#include <iterator>", ""]
output += ['#include "%s"' % header for header in sorted(used_headers)]
output.append("")
output += [
    'template<> constexpr const char *kModName<%s> = "%s";' % (name, name)
    for name, _, _ in entries
]
output.append("")
for name, _, lines in entries:
    if lines:
        output.append("static const ModMethod k%sMethods[] = {" % name)
        output += ["    %s," % line for line in lines]
        output.append("};")
output.append("")
output.append("const ModClass g_modClasses[] = {")
for name, bases, lines in entries:
    methods = (
        "k%sMethods, std::size(k%sMethods)" % (name, name) if lines else "nullptr, 0"
    )
    output.append(
        '    {"%s", "%s", %s, ModDestroyOf<%s>()},' % (name, bases, methods, name)
    )
output.append("};")
output.append("")
output.append("const size_t g_modClassCount = std::size(g_modClasses);")
output.append("")

text = "\n".join(output)
if (
    not os.path.exists(output_file)
    or open(output_file, encoding="utf-8").read() != text
):
    with open(output_file, "w", encoding="utf-8") as stream:
        stream.write(text)

stub_output = ["---@meta", ""]
for name, bases, stubs in stub_entries:
    stub_output.append("---@class %s%s" % (name, " : " + bases[0] if bases else ""))
    stub_output.append("%s = {}" % name)
    stub_output.append("")
    for lines in stubs:
        stub_output += lines
        stub_output.append("")

stub_text = "\n".join(stub_output)
os.makedirs(os.path.dirname(stub_file), exist_ok=True)
if not os.path.exists(stub_file) or open(stub_file, encoding="utf-8").read() != stub_text:
    with open(stub_file, "w", encoding="utf-8") as stream:
        stream.write(stub_text)
