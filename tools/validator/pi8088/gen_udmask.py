#!/bin/python

import xml.etree.ElementTree as ET

def create_mask(text):
    res = '0'
    if text.find('c') != -1:
        res += '|VXT_CARRY'
    if text.find('p') != -1:
        res += '|VXT_PARITY'
    if text.find('a') != -1:
        res += '|VXT_AUXILIARY'
    if text.find('z') != -1:
        res += '|VXT_ZERO'
    if text.find('s') != -1:
        res += '|VXT_SIGN'
    if text.find('t') != -1:
        res += '|VXT_TRAP'
    if text.find('i') != -1:
        res += '|VXT_INTERRUPT'
    if text.find('d') != -1:
        res += '|VXT_DIRECTION'
    if text.find('o') != -1:
        res += '|VXT_OVERFLOW'
    return res

def main():      
    root = ET.parse("x86reference.xml").getroot()

    print('const struct {')
    print('\tint opcode;')
    print('\tint ext;')
    print('\tint mask;')
    print('} flag_mask_lookup[] = {')

    for pri_opcd in root.findall('./one-byte/pri_opcd'):
        for entry in pri_opcd.findall('./entry'):
            for undef_f in entry.findall('./undef_f'):
                print('\t{ 0x', pri_opcd.attrib['value'], end='', sep='')

                opcd_ext = entry.find('opcd_ext')
                if opcd_ext == None:
                    print(', -1', end='')
                else:
                    print(',', opcd_ext.text, end='')

                print(',', create_mask(undef_f.text), '},')

    print('\t{-1, -1, -1},\n};')
      
main()