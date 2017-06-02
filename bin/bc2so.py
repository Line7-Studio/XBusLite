#! python3

import os
import sys
import struct
import marshal
import hashlib

resource_list = []

resource_list.append({
        'name': ':Extension/Quantitate',
        'file': './src/Extension/Quantitate/main.py',
    })

resource_list.append({
        'name': ':Extension/SpectrumFile/Reader',
        'file': './src/Extension/SpectrumFile/Reader/main.py',
    })

resource_list.append({
        'name': ':Extension/SpectrumFile/Writer',
        'file': './src/Extension/SpectrumFile/Writer/main.py',
    })

resource_list.append({
        'name': ':Extension/SpectrumFile/Converter',
        'file': './src/Extension/SpectrumFile/Converter/main.py',
    })


################################################################################

dir_dot = os.path.dirname(os.path.abspath(__file__))
for target in resource_list:
    if target['file'][0] == '.':
        file_path =  dir_dot + '/' + target['file']
    else:
        file_path = os.path.normpath(file_path)

    target['file'] = os.path.normpath(file_path)


def transfrom_files():
    print()
    print("#include <string>\n")
    print("#include <string_view>\n")
    print("namespace { // anonymous \n")

    out_source_file_dict = {}
    in_source_file_hash_list = []
    print('static const unsigned char resource_data[] = {\n')
    for target in resource_list:
        block_size = 65536
        hash_fun = hashlib.md5()
        in_source_source = b''
        with open(target['file'], 'rb') as f:
            while True:
                block = f.read(block_size)
                if len(block) > 0:
                    hash_fun.update(block)
                    in_source_source += block
                else:
                    break
        in_file_hash = hash_fun.hexdigest().upper()

        if in_file_hash not in out_source_file_dict:
            out_source_file_dict[in_file_hash] = {'len': 0, 'name': []}
        out_source_file_dict[in_file_hash]['name'].append(target['name'])

        if in_file_hash in in_source_file_hash_list:
            continue # alreadly compiled
        else:
            in_source_file_hash_list.append(in_file_hash)

        code_object = compile(in_source_source, '<string>', 'exec', dont_inherit=True)
        code_object_bytes = marshal.dumps(code_object)
        out_source_file_dict[in_file_hash]['len'] = len(code_object_bytes)

        row_count, row_step = 0, 12
        while True:
            row = code_object_bytes[row_count:row_count+row_step]
            if len(row) == 0:
                break
            for char in row:
                print(' 0x%02x,'%char, end='')
            print()
            row_count += row_step
        print()

    print('};\n')


    print('static const unsigned char resource_name[] = {\n')
    for file_hash in in_source_file_hash_list:
        v = out_source_file_dict[file_hash]
        for name in v['name']:
            print(' "', name, '"', sep='')
    print()
    print('};\n')


    print('static const unsigned char resource_struct[] = {\n')
    data_shift, name_shift = 0, 0
    for file_hash in in_source_file_hash_list:
        def print_size_t(size_t_value):
            for char in struct.pack('N', size_t_value):
                print(' 0x%02x,'%char, end='')

        frame = out_source_file_dict[file_hash]
        for name in frame['name']:
            print_size_t(frame['len'])
            print(' // data size')

            print_size_t(data_shift)
            print(' // data shift')

            print_size_t(len(name))
            print(' // name size')

            print_size_t(name_shift)
            print(' // name shift')

            name_shift += len(name)

            print()
        data_shift += frame['len']

    print('};\n')

    print("}// anonymous namespace\n")

    code = r'''

std::string_view get_resource(const std::string& required_resource_name)
{
    const auto resource_name_buffer = (char*)(resource_name);
    const auto resource_data_buffer = (char*)(resource_data);
    const auto resource_struct_buffer = (size_t*)(resource_struct);

    const int item_count = sizeof(resource_struct)/(sizeof(size_t)*4);
    for( int idx = 0; idx < item_count; ++idx )
    {
        auto data_size  = *(resource_struct_buffer + (idx*4+0));
        auto data_shift = *(resource_struct_buffer + (idx*4+1));
        auto name_size  = *(resource_struct_buffer + (idx*4+2));
        auto name_shift = *(resource_struct_buffer + (idx*4+3));

        auto this_resoure_name = \
            std::string_view(resource_name_buffer + name_shift, name_size);
        if( required_resource_name != this_resoure_name ){
            continue;
        }

        return std::string_view{resource_data_buffer + data_shift, data_size};
    }

    return std::string_view();
}

    '''

    print(code)


def enumerate_files():
    for target in resource_list:
        print(target['file'])


if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()

    parser.add_option("-o", "--out",
                      dest="out",
                      default=sys.stdout,
                      help="sys.stdout[default] or a specified file")

    parser.add_option("-m", "--mode",
                      dest="mode",
                      type="string",
                      help="t/transfrom or e/enumerate")

    (options, args) = parser.parse_args()

    if options.out != sys.stdout:
        if type(options.out) == str:
            sys.stdout = open(options.out, 'wt')

    if options.mode in ['transfrom', 't']:
        transfrom_files()
    elif options.mode in ['enumerate', 'e']:
        enumerate_files()
    else:
        parser.print_help()

