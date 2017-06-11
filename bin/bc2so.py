#! python3

import os
import sys
import struct
import marshal
import hashlib


def resource_file_to_embeded_data_EXEC(resource_file: str):
    block_size = 65536
    hash_fun = hashlib.md5()
    in_source_source = b''
    with open(resource_file, 'rb') as f:
        while True:
            block = f.read(block_size)
            if len(block) > 0:
                hash_fun.update(block)
                in_source_source += block
            else:
                break
    in_file_hash = hash_fun.hexdigest().upper()

    # code_object = compile(in_source_source, '<string>', 'exec', dont_inherit=False)
    code_object = compile(in_source_source, resource_file, 'exec', dont_inherit=False)

    return marshal.dumps(code_object)


def resource_file_to_embeded_data_RAW(resource_file: str):
    block_size = 65536
    hash_fun = hashlib.md5()
    in_source_source = b''
    with open(resource_file, 'rb') as f:
        while True:
            block = f.read(block_size)
            if len(block) > 0:
                hash_fun.update(block)
                in_source_source += block
            else:
                break
    in_file_hash = hash_fun.hexdigest().upper()

    return in_source_source



def process(file_full_path, resource_name):
    print()
    print("namespace XBusLite { ")
    print("  void EmbededSourceInitialize(const unsigned char* resource_struct,")
    print("                               const unsigned char* resource_name,")
    print("                               const unsigned char* resource_data);")
    print("  void EmbededSourceFinalize(const unsigned char* resource_struct,")
    print("                             const unsigned char* resource_name,")
    print("                             const unsigned char* resource_data);")
    print("} // namespace XBusLite ")
    print()

    print("namespace { // anonymous \n")

    print('static const unsigned char resource_data[] = {\n')

    embeded_data = resource_file_to_embeded_data_EXEC(file_full_path)
    # embeded_data = resource_file_to_embeded_data_RAW(file_full_path)

    row_count, row_step = 0, 12
    while True:
        row = embeded_data[row_count:row_count+row_step]
        if len(row) == 0:
            break
        for char in row:
            print(' 0x%02x,'%char, end='')
        print()
        row_count += row_step
    print()

    print('};\n')


    print('static const unsigned char resource_name[] = {\n')
    print(' "', resource_name, '"', sep='')
    print()
    print('};\n')


    def print_size_t(size_t_value):
        for char in struct.pack('N', size_t_value):
            print(' 0x%02x,'%char, end='')

    print('static const unsigned char resource_struct[] = {\n')
    print_size_t(len(resource_name))
    print(' // name size')
    print_size_t(len(embeded_data))
    print(' // data size')
    print()
    print('};\n')

    print('struct initializer {')
    print('  initializer() {')
    print('     XBusLite::EmbededSourceInitialize(resource_struct, resource_name, resource_data);')
    print('   }')
    print('  ~initializer() {')
    print('     XBusLite::EmbededSourceFinalize(resource_struct, resource_name, resource_data);')
    print('   }')
    print('} dummy;')

    print()

    print("}// anonymous namespace\n")

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()

    parser.add_option("--file",
                      dest="file",
                      type="string",
                      help="resource file")

    parser.add_option("--name",
                      dest="name",
                      type="string",
                      help="resource name")

    parser.add_option("--output",
                      dest="output",
                      default=sys.stdout,
                      help="sys.stdout[default] or a specified file")


    (options, args) = parser.parse_args()

    if options.output != sys.stdout:
        if type(options.output) == str:
            sys.stdout = open(options.output, 'wt')

    process(options.file, options.name)

