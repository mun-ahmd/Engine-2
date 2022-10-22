import itertools
from os import listdir, path
from pprint import pprint
from typing import Dict, List




#goal is to create a c++ class with static varibles of uniforms and ssbos of given list of program

vert = "newDrawVert.glsl"
frag = "newDrawFrag.glsl"

def parse_shader(filedir):
    print(filedir)
    details = {'uniform' : [], 'in' : [], 'out' : []}
    
    content = open(filedir, 'r').read()
    lines = []
    for line in content.splitlines():
        if line.strip().startswith('#'):
            continue
        comment_found = line.find('//')
        if comment_found != -1:
            line = line[:comment_found]
        lines.append(line)
    content = ' '.join(lines)
    blocks = []
    found = content.find("{")
    while(found != -1):
        semicolon = content.rfind(";", 0, found)
        semicolon = semicolon+1 if semicolon != -1 else 0
        matched = content.find("}", found)
        if(matched == -1):
            #error
            raise Exception()
        blocks.append({'content' :content[:semicolon], 'match': False})
        blocks.append({'content' :content[semicolon:matched+1], 'match': True})
        content = content[matched+1::]
        found = content.find("{")
    blocks.append({'content' :content, 'match': False})
    semi_blocks = [block['content'].strip().replace('\t','').replace('\n','') for block in blocks if block['match']]
    other_blocks = [block['content'].strip().replace('\t','').replace('\n','') for block in blocks if not block['match']]
    
    for other_block in other_blocks:
        for block in other_block.split(';'):
            for key in details.keys():
                key_found = block.find(key)
                details[key] += [block.strip()] if key in block.split(" ") else []

    pprint(details)
    return process_details(details)

def get_uniform_type(uniform : str):
    types = {
        'float' : 'float',
        'vec2' : 'float[2]',
        'vec3' : 'float[3]',
        'vec4' : 'float[4]',
        'int' : 'int',
        'uint' : 'uint32_t',
        'mat4' : 'float[16]',
        'sampler2D' : 'int' if 'bindless' not in uniform else 'uint64_t'
    }
    return types.get(uniform[uniform.find('uniform') + len('uniform'):].strip().split(' ')[0], 'any')

def process_details(details : Dict[str, str]):
    processed = {}
    for key, val in details.items():
        processed[key] = []
        if key == 'uniform':
            for uniform in val:
                processed[key] += [{
                    'type' : get_uniform_type(uniform),
                    'name' : uniform.split(' ')[-1]
                }]
    return processed

def create_class(name : str, details) -> str:
    uniforms = ''
    for uni in details['uniform']:
        uniforms += uni['type'] + ' ' + uni['name'] + ';\n\t\t'
    cpp_class = (
    f'\nclass {name}{{\n'
    f'public:\n'
    f'\tstruct{{\n'
    f'\t\t{uniforms}\n'
    f'\t}} uniforms;\n'
    f'}};')
    print(cpp_class.replace('\t','    '))
    return cpp_class

    
for shader in itertools.chain(listdir("."), listdir("Shaders\\")):
    if not path.isfile(shader):
        continue
    create_class(path.basename(shader), parse_shader(shader))
    print("\n\n")
