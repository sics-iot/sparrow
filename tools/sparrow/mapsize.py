import sys, operator

class SourceFile:
    name = "unknown"
    ram = 0
    flash = 0

    def __init__(self, name):
        self.name = name
    def add_flash(self, size):
        self.flash = self.flash + int(size, 16)
    def add_ram(self, size):
        self.ram = self.ram + int(size, 16)


source_files = {}


def add_flash(fname, size):
    if fname not in source_files:
        source_files[fname] = SourceFile(fname)
    source_files[fname].add_flash(size)

def add_ram(fname, size):
    if fname not in source_files:
        source_files[fname] = SourceFile(fname)
    source_files[fname].add_ram(size)

INIT = 0
COMMON_SYMBOLS = 1
DISCARDED_SECTIONS = 2
TEXT_SECTIONS = 3
RAM_SECTIONS = 4



fname = sys.argv[1]
with open(fname) as f:
    content = f.readlines()
content = [line.rstrip('\n') for line in content]

ram_size = 0
state = INIT
text_size = 0
name_only = 0
for line in content:
    if "Common symbol" in line:
        print "Common symbols..."
        state = COMMON_SYMBOLS
        continue
    if "Discarded" in line:
        state = DISCARDED_SECTIONS
    if ".isr_vector" in line:
        state = TEXT_SECTIONS
    if ".boot_data" in line:
        state = RAM_SECTIONS
    if line != "" and state == COMMON_SYMBOLS:
        parts = line.split()
        if len(parts) > 1:
            if name_only:
                size = parts[0]
                fname = parts[1]
            else:
                name = parts[0]
                size = parts[1]
                fname = parts[2]
            print "Symbol ", name, " RAM:", size, "File: ", fname
            add_ram(fname, size)
            ram_size = ram_size + int(size, 16)
            name_only = 0
        else:
            name_only = 1
            name = parts[0]
    if line != "" and state == TEXT_SECTIONS and (" .text" in line or name_only == 1):
        parts = line.split()
        if len(parts) > 1:
            if name_only:
                size = parts[1]
                fname = parts[2]
            else:
                name = parts[0]
                size = parts[2]
                fname = parts[3]
            print "Symbol ", name, " TEXT:", size
            add_flash(fname, size)
            text_size = text_size + int(size, 16)
            name_only = 0
        else:
            name_only = 1
            name = parts[0]

    if line != "" and state == TEXT_SECTIONS and (" .rodata" in line or name_only == 1):
        parts = line.split()
        if len(parts) > 1:
            if name_only:
                size = parts[1]
                fname = parts[2]
            else:
                name = parts[0]
                size = parts[2]
                fname = parts[3]
            print "Symbol ", name, " rodata:", size, " file:", parts[3]
            text_size = text_size + int(size, 16)
            add_flash(fname, size)
            name_only = 0
        else:
            name_only = 1
            name = parts[0]

    if line != "" and " *fill*" in line:
        parts = line.split()
        if len(parts) > 1:
            name = parts[0]
            size = parts[2]
            print "Symbol ", name, " fill:", size
            if state == TEXT_SECTIONS:
                add_flash("*fill*", size)
                text_size = text_size + int(size, 16)
            else:
                add_ram("*fill*", size)
                ram_size = ram_size + int(size, 16)
            name_only = 0

    if line != "" and (" .data" in line or " .bss" in line):
        parts = line.split()
        if len(parts) > 1:
            name = parts[0]
            size = parts[2]
            fname = parts[3]
            print "Symbol ", name, " RAM:", size
            add_ram(fname, size)
            ram_size = ram_size + int(size, 16)
            name_only = 0



    print line


print "---- Summary ----"
print "RAM: ", ram_size
print "TEXT:", text_size

src_list = source_files.values()

src_list.sort(key=lambda x:x.flash, reverse=True)

for src in src_list:
    print '%04x' % (src.flash), '%04x' % (src.ram), src.name
